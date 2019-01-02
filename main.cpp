/*
nexus433
Copyright (C) 2018 aquaticus

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <list>
#include <iostream>
#include <inttypes.h>
#include <unistd.h>
#include <getopt.h>
#include <csignal>
#include <set>
#include <sys/types.h>
#include <sys/stat.h>
#include <syslog.h>
#include "decoder.h"
#include "config.h"
#include "color.h"
#include "diag.h"
#include "packet.h"
#include "packetstorage.h"
#include "mqttclient.h"
#include "_VERSION"
#include "led.h"

Decoder* g_pDecoder = NULL; //for signal handler function
#ifdef DEBUG
int g_Verbose = 1;
#else
int g_Verbose = 0;
#endif

static int ProcessLoop(MQTTClient& mqtt, IDecoder& decoder, PacketStorage& storage, Led& led)
{
    std::set<Packet> NewReadings;
    std::set<uint16_t> RemovedTransmitters;
    std::set<uint16_t> NewTransmitters;

    mqtt.OnlineStatus(true);

    decoder.Start();

    while ( !decoder.WaitForTermination(500) )
    {
        led.Control(false);
        storage.UpdateStatus(NewReadings, RemovedTransmitters, NewTransmitters);

        // Sends all new Transmitters when MQTT connection is established.
        // That way when new readings are coming sensor was already properly created in Home Assistant
        if (mqtt.IsConnected() && NewTransmitters.size() > 0)
        {
            std::set<uint16_t>::iterator it = NewTransmitters.begin();
            while (it != NewTransmitters.end())
            {
                if (mqtt.IsConnected())
                {
                    VERBOSE_PRINTF(ANSI_COLOR_YELLOW "New transmitter 0x%02x channel %d\n" ANSI_COLOR_RESET, *it >> 8, (*it & 0xFF) + 1);

                    if( Config::transmitter::discovery )
                    {
                        mqtt.SensorDiscover(*it);
                    }
                    mqtt.SensorStatus(*it, true);
                    it = NewTransmitters.erase(it);
                }
                else
                {
                    it++;
                }
            }
        }

        for (auto id : RemovedTransmitters)
        {
            VERBOSE_PRINTF(ANSI_COLOR_RED "Transmitter become silent: 0x%02x channel %d. No data for %u sec.\n" ANSI_COLOR_RESET, id >> 8,
                    (id & 0xFF) + 1, storage.GetSilentTimeoutSec());
            mqtt.SensorStatus(id, false);
        }
        RemovedTransmitters.clear();

        // New readings. Do not accumulate data when MQTT conn is lost. If MQTT connection is
        // re-established it sends only the most recent reading.
        if (NewReadings.size() > 0)
        {
            led.Control(true);
            std::set<Packet>::iterator it = NewReadings.begin();
            while (it != NewReadings.end())
            {
                VERBOSE_PRINTF(ANSI_COLOR_BRIGHT_GREEN "0x%" PRIx64 " Id:0x%02x Channel:%d Temperature: %.1f°C Humidity: %d%% Battery:%d Frames:%d (%d%%)\n" ANSI_COLOR_RESET,
                        it->GetRaw(), it->GetId(), it->GetChannel() + 1, it->GetTemperature(),
                        it->GetHumidity(), it->GetBattery(), it->GetFrameCounter(), it->GetQualityPercent());

                if (mqtt.IsConnected())
                {
                    if( Config::ignored.find( it->GetSenderId() ) != Config::ignored.end() )
                    {
                        VERBOSE_PRINTF(ANSI_COLOR_MAGENTA "0x%" PRIx64 " Transmitter 0x%04x is ignored.\n" ANSI_COLOR_RESET,
                                it->GetRaw(),
                                it->GetSenderId() );
                    }
                    else
                    {
                        if( it->GetFrameCounter() >= Config::transmitter::minimum_frames )
                        {
                            mqtt.SensorUpdate(it->GetSenderId(), *it);
                        }
                        else
                        {
                            VERBOSE_PRINTF(ANSI_COLOR_MAGENTA "0x%" PRIx64 " Only %d frame received. Ignored.\n" ANSI_COLOR_RESET,
                                    it->GetRaw(),
                                    it->GetFrameCounter() );
                        }
                    }
                }

                it = NewReadings.erase(it);
            }

        }

        if (mqtt.loop(1000) != MOSQ_ERR_SUCCESS)
        {
            puts("MQTT Reconnecting");
            mqtt.reconnect();
        }

    }

    DEBUG_PRINTF("Processing loop exit\n");

    // send offline
    std::list<uint16_t> transmitters = storage.GetActiveTransmitters();
    for (auto& item : transmitters)
    {
        mqtt.SensorStatus(item, false);
    }

    mqtt.OnlineStatus(false);

    return decoder.IsErrorStop() ? -6 : EXIT_SUCCESS;
}

static void terminate_handler(int signal)
{
    DEBUG_PRINTF("DEBUG: RECEIVED TERMINATE SIGNAL %d\n", signal);
    if (g_pDecoder)
    {
        g_pDecoder->Terminate();
    }
    else
    {
        abort();
    }
}

static void print_help()
{
    puts("nexus433 - reads data from 433 MHz temperature and humidity sensors.\n"
            "It requires 433 MHz receiver connected to one of GPIO pins.\n"
            "Version: " VERSION "\n"
            "Parameters (all optional):\n"
            "\t--verbose - enable verbose output\n"
            "\t--daemon - run in daemon mode\n"
            "\t-g/--config - configuration file; default /etc/nexus433.ini\n"
            "\t-c/--chip - gpio device to use; default /dev/gpiochip0\n"
            "\t-n/--pin - pin number where 433MHz receiver is connected; default 1\n"
            "\t-a/--address - MQTT broker address\n"
            "\t-p/--port - MQTT broker port; default 1883\n"
            "\t-h/--help - shows this message\n"
            "\t-v/--version - shows version\n"
            "Pin numbering schema is using block GPIO API, not sysfs.\n\n"
            "Source code: https://github.com/aquaticus/nexus433\n"
            "License: GNU GPLv3\n"
         );
}

int main(int argc, char** argv)
{
    int c;
    int option_index = 0;
    int i = 0;
    int daemon_flag = 0;
    int rv;
    const char* config = NULL;

    struct option long_options[] =
    {
    { "verbose", no_argument, &g_Verbose, 1 },
    { "daemon", no_argument, &daemon_flag, 1 },
    { "config", required_argument, 0, 'g' },
    { "chip", required_argument, 0, 'c' },
    { "pin", required_argument, 0, 'n' },
    { "port", required_argument, 0, 'p' },
    { "address", required_argument, 0, 'a' },
    { "help", no_argument, 0, 'h' },
    { "version", no_argument, 0, 'v' },
    { 0, 0, 0, 0 } };

    while ((c = getopt_long(argc, argv, "g:c:n:p:a:h:v", long_options, &option_index)) != -1)
    {
        switch (c)
        {
        case 0:
            break;

        case 'g':
            config = optarg;
            break;

        case 'c':
            Config::receiver::chip = optarg;
            break;

        case 'n':
            i = 0;
            while (optarg[i])
            {
                if (!isdigit(optarg[i]))
                {
                    puts("-n/--pin accepts numbers only");
                    return EXIT_FAILURE;
                }
                i++;
            }
            Config::receiver::pin = atoi(optarg);
            break;

        case 'a':
            Config::mqtt::host = optarg;
            break;

        case 'p':
            i = 0;
            while (optarg[i])
            {
                if (!isdigit(optarg[i]))
                {
                    puts("-p/--port accepts numbers only");
                    return EXIT_FAILURE;
                }
                i++;
            }
            Config::mqtt::port = atoi(optarg);
            break;

        case '?':
            print_help();
            return EXIT_FAILURE;
            break;

        case 'h':
            print_help();
            return EXIT_SUCCESS;
            break;

        case 'v':
            puts(VERSION);
            return EXIT_SUCCESS;
            break;

        default:
            abort();
        }
    }

    if( daemon_flag )
    {
        DEBUG_PRINTF("Daemon start\n");
        daemon(0, g_Verbose);
        openlog (NEXUS433, LOG_PID, LOG_DAEMON);
        syslog (LOG_NOTICE, NEXUS433 " daemon started.");
    }

    // if config was specified check if file exists
    // in daemon mode path are relative to root, so better pass absolute path
    if (config && access(config, F_OK) == -1)
    {
        std::cerr << "No access to configuration file " << config << std::endl;
        return -4;
    }

    // if default file does not exists -- ignore
    if (!config && access(Config::default_config_location, F_OK) != -1)
    {
        config = Config::default_config_location;
    }

    if (config)
    {
        std::cout << "Loading configuration from " << config << std::endl;

        if (!Config::Load(config))
        {
            std::cerr << "Error parsing configuration file " << config << std::endl;
            return -5;
        }
    }

    struct gpiod_chip *chip;
    struct gpiod_line *line;

    chip = gpiod_chip_open(Config::receiver::chip.c_str());
    if (!chip)
    {
        std::cerr << "Cannot open GPIO device " << Config::receiver::chip << " Are you root?" << std::endl;
        return -1;
    }

    line = gpiod_chip_get_line(chip, Config::receiver::pin);
    if (!line)
    {
        gpiod_chip_close(chip);
        std::cerr << "Cannot open line. Verify line exists and is not used." << std::endl;
        return -2;
    }

    rv = gpiod_line_request_input(line, NEXUS433);
    if (-1 == rv)
    {
        gpiod_chip_close(chip);
        std::cerr << "Request for line input failed." << std::endl;
        return -3;
    }


    Led led;
    if( Config::receiver::internal_led.size() > 0)
    {
      if( !led.Open(Config::receiver::internal_led) )
      {
        std::cerr << "Cannot open internal LED device " << Config::receiver::internal_led << std::endl;
        gpiod_line_close_chip(line);
        return -12;
      }
    }

    MQTTClient mqtt(NEXUS433);

    if (int rc = mqtt.Connect(Config::mqtt::host.c_str(), Config::mqtt::port))
    {
        std::cerr << "Unable to connect to MQTT server." << std::endl;
        std::cerr << mosquitto_connack_string(rc) << std::endl;
        gpiod_line_close_chip(line);
        return -7;
    }

    PacketStorage storage;
    Decoder decoder(storage, line, Config::receiver::tolerance_us, Config::receiver::resolution_us);
    PacketStorage::SetSilentTimeoutSec( Config::transmitter::silent_timeout_sec );

    g_pDecoder = &decoder;
    std::signal(SIGINT, terminate_handler);
    std::signal(SIGHUP, terminate_handler);
    std::signal(SIGTERM, terminate_handler);

    std::cout << "Reading data from 433MHz receiver on " << Config::receiver::chip << " pin " << Config::receiver::pin
            << "." << std::endl;

    rv = ProcessLoop(mqtt, decoder, storage, led);

    gpiod_line_close_chip(line);
    mqtt.disconnect();
    led.Close();

    if( daemon_flag )
    {
        syslog (LOG_NOTICE, NEXUS433 " daemon terminated");
        closelog();
    }
    else
    {
        std::cout << "Program terminated" << std::endl;
    }

    return rv;
}
