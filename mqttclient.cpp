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

#include "diag.h"
#include "mqttclient.h"
#include "stdio.h"
#include "config.h"
#include "color.h"
#include <string.h>
#include "version.h"

#define DEVICE_MANUFACTURER "aquaticus"
#define DEVICE_NAME "Nexus433 433MHz gateway"
#define DEVICE_MODEL "nexus433"

#define MAX_TOPIC_LEN 256
#define MAX_PAYLOAD_LEN 1024

static int get_pem_password(char *buf, int size, int rwflag, void *userdata)
{
  if( size < (int)Config::mqtt::keypass.size() )
  {
    return 0; //buffer too small to store pass
  }

  Config::mqtt::keypass.copy(buf, Config::mqtt::keypass.size() );

  return Config::mqtt::keypass.size();
}

int MQTTClient::Connect(const char* host, int port)
{
    if( Config::mqtt::user.size() > 0)
    {
        username_pw_set(Config::mqtt::user.c_str(), Config::mqtt::password.c_str());
    }

    char avail_topic[MAX_TOPIC_LEN];
    snprintf(avail_topic, sizeof(avail_topic), m_GatewayAvailTopicFormat, Config::receiver::mac.c_str());

    will_set(avail_topic, strlen(m_Offline), m_Offline, 0, true );

    if( !Config::mqtt::cafile.empty() || !Config::mqtt::capath.empty() )
    {
      tls_set(
              Config::mqtt::cafile.empty() ? NULL : Config::mqtt::cafile.c_str(),
              Config::mqtt::capath.empty() ? NULL : Config::mqtt::capath.c_str(),
              Config::mqtt::certfile.empty() ? NULL : Config::mqtt::certfile.c_str(),
              Config::mqtt::keyfile.empty() ? NULL : Config::mqtt::keyfile.c_str(),
              get_pem_password
            );
    }

    return connect(host, port, 60);
}

void MQTTClient::on_connect(int rc)
{
    if( rc == 0 )
    {
        printf("Connected to MQTT broker.\n");
        m_Connected = true;
        if( Config::transmitter::discovery )
        {
            std::string status = Config::transmitter::discovery_prefix + "/status";

            DEBUG_PRINTF("DEBUG: Subscribing to MQTT topic: %s\n", status.c_str());

            GatewayDiscover();

            // subscribe only when discovery enabled
            subscribe(NULL, status.c_str());
        }
    }
    else
    {
        m_Connected = false;
    }
}

void MQTTClient::on_disconnect(int rc)
{
    printf("MQTT Disconnected. %s\n", rc == 0 ? "" : mosquitto_connack_string(rc) );
    m_Connected = false;
}

void MQTTClient::on_message(const struct mosquitto_message * message)
{
    DEBUG_PRINTF("MQTT message: %s (%d)\n", message->topic, message->payloadlen);
    int online_s = strlen(m_Online);

    if( m_StatusTopic.compare(message->topic)
        && message->payloadlen == online_s
        && 0 == strncmp(m_Online,(const char*)message->payload, online_s)
        )
    {
        DEBUG_PRINTF("DEBUG: Home Assistant online\n");
        
        GatewayDiscover();

        //send discovery messages again
        std::list<uint16_t> transmitters = m_Storage.GetActiveTransmitters();
        for (auto& item : transmitters)
        {
           SensorDiscover(item);
        }
    }
}

void MQTTClient::SensorUpdate(uint16_t id, const Packet& packet)
{
    Substitute(id);

    const char* const payload_fmt = "{ "
            "\"temperature\": %.1f, "
            "\"humidity\": %d, "
            "\"battery\": \"%s\", "
            "\"quality\": %d "
            "}";

    char payload[MAX_PAYLOAD_LEN];
    snprintf(payload, sizeof(payload),
            payload_fmt,
            packet.GetTemperature(),
            packet.GetHumidity(),
            packet.GetBattery() ? Config::transmitter::battery_normal.c_str() : Config::transmitter::battery_low.c_str(),
            packet.GetQualityPercent()
    );

    char state_topic[MAX_TOPIC_LEN];
    snprintf(state_topic, sizeof(state_topic), m_SensorStateTopicFormat, Config::receiver::mac.c_str(), id);

    // publish
    publish(NULL, state_topic, strlen(payload), payload);

}

void MQTTClient::GatewayUpdate(int number_of_active_sensors)
{
    const char* const payload_fmt = "{ "
            "\"active\": %d"
            " }";

    char payload[MAX_PAYLOAD_LEN];
    char state_topic[MAX_PAYLOAD_LEN];
    snprintf(payload, sizeof(payload),
            payload_fmt,
            number_of_active_sensors
    );

    snprintf(state_topic, sizeof(state_topic), m_GatewayStateTopicFormat, Config::receiver::mac.c_str());

    publish(NULL, state_topic, strlen(payload), payload);
}

int MQTTClient::SensorStatus(uint16_t id, bool online)
{
    Substitute(id);

    const char* const payload = online ? m_Online : m_Offline;
    char topic[MAX_TOPIC_LEN];

    snprintf(topic, sizeof(topic), m_SensorAvailTopicFormat, Config::receiver::mac.c_str(), id);
    publish(NULL, topic, strlen(payload), payload);
    DEBUG_PRINTF("DEBUG: Sensor statatus: %s\n", payload);

    return 0;
}

void MQTTClient::GatewayDiscover()
{
    char state_topic[MAX_TOPIC_LEN];
    char avail_topic[MAX_TOPIC_LEN];
    char payload[MAX_PAYLOAD_LEN];
    char discovery_topic[MAX_TOPIC_LEN];
    const char* discovery_topic_fmt="%s/sensor/" NEXUS433 "_%s/config";
    
    snprintf(discovery_topic, sizeof(discovery_topic), discovery_topic_fmt,   Config::transmitter::discovery_prefix.c_str(),
                                                Config::receiver::mac.c_str());
    snprintf(state_topic, sizeof(state_topic), m_GatewayStateTopicFormat, Config::receiver::mac.c_str());
    snprintf(avail_topic, sizeof(avail_topic), m_GatewayAvailTopicFormat, Config::receiver::mac.c_str());

    snprintf(payload, sizeof(payload),
        "{"
        "\"state_topic\": \"%s\","
        "\"unique_id\": \"" NEXUS433 "_%s_active\","
        "\"name\": \"Number of active 433 MHz transmitters\","
        "\"value_template\": \"{{value_json.active}}\","
        "\"enabled_by_default\": \"true\","
        "\"availability_topic\": \"%s\", "
        "\"device\": "
            "{"
            "\"manufacturer\": " "\"" DEVICE_MANUFACTURER "\","
            "\"name\": \"" DEVICE_NAME "\","
            "\"model\": \"" DEVICE_MODEL "\","
            "\"identifiers\": [\"%s\"],"
            "\"sw_version\": \"" VERSION "\""
            "}"
        "}",
        state_topic,
        Config::receiver::mac.c_str(),
        avail_topic,
        Config::receiver::mac.c_str()
    );

    DEBUG_PRINTF("DEBUG: Sending device discovery packet on: %s\n", discovery_topic);

    // publish
    publish(NULL, discovery_topic, strlen(payload), payload);
}

void MQTTClient::SensorDiscover(uint16_t id)
{
    DEBUG_PRINTF("DEBUG: Sending MQTT discovery packet for id %02x\n", id);

    const int TOPIC_COUNT = 4;

    Substitute(id);

    const char* discovery_topic_fmt[TOPIC_COUNT] =
    {
            "%s/sensor/" NEXUS433 "_%s_%04x_temperature/config",
            "%s/sensor/" NEXUS433 "_%s_%04x_humidity/config",
            "%s/sensor/" NEXUS433 "_%s_%04x_battery/config",
            "%s/sensor/" NEXUS433 "_%s_%04x_quality/config"
    };

    const char* units[TOPIC_COUNT] = { "°C", "%", "%", "%" };
    const char* value[TOPIC_COUNT] = { "temperature", "humidity", "battery", "quality" };
    const char* device_class[TOPIC_COUNT] = { "temperature", "humidity", "battery", "signal_strength" };

    std::string name;
    char discovery_topic[MAX_TOPIC_LEN];
    char payload[MAX_PAYLOAD_LEN];
    char state_topic[MAX_TOPIC_LEN];
    char avail_topic[MAX_TOPIC_LEN];

    for (int i = 0; i < TOPIC_COUNT; i++)
    {
        snprintf(discovery_topic, sizeof(discovery_topic), discovery_topic_fmt[i], Config::transmitter::discovery_prefix.c_str(), Config::receiver::mac.c_str(), id);
        name = GetName((TYPES)i, id);
        snprintf(state_topic, sizeof(state_topic), m_SensorStateTopicFormat, Config::receiver::mac.c_str(), id);
        snprintf(avail_topic, sizeof(avail_topic), m_SensorAvailTopicFormat, Config::receiver::mac.c_str(), id);


        snprintf(payload, sizeof(payload),
                "{"
                "\"name\": \"%s\", "
                "\"device_class\": \"%s\", "
                "\"state_topic\": \"%s\", "
                "\"availability_topic\": \"%s\", "
                "\"unit_of_measurement\": \"%s\", "
                "\"value_template\": \"{{ value_json.%s }}\", "
                "\"expire_after\": %d, "
                "\"unique_id\": \"" NEXUS433 "_%s_%04x_%s\","
                "\"device\":"
                    "{"
                    "\"name\": \"Temperatue Sensor Id:%02X ch %d\","
                    "\"model\": \"433 MHz\","
                    "\"identifiers\": [\"%s.%04x\"],"
                    "\"via_device\": \"%s\""
                    "}"
                "}",
                name.c_str(),
                device_class[i],
                state_topic,
                avail_topic,
                units[i],
                value[i],
                PacketStorage::GetSilentTimeoutSec(),
                Config::receiver::mac.c_str(),
                id,
                value[i],
                id >> 8, (id & 0xFF) + 1,
                Config::receiver::mac.c_str(),
                id,
                Config::receiver::mac.c_str()
                );

        publish(NULL, discovery_topic, strlen(payload), payload);
    }
}

void MQTTClient::GatewayStatus(bool online)
{
    char status_topic[MAX_TOPIC_LEN];
    snprintf(status_topic, sizeof(status_topic), m_GatewayAvailTopicFormat, Config::receiver::mac.c_str());

    const char* payload = online ? m_Online : m_Offline;

    publish(NULL, status_topic, strlen(payload), payload, 0, true);
}


void MQTTClient::Substitute(uint16_t& id)
{
    auto it = Config::substitutes.find(id);
    if( it != Config::substitutes.end() )
    {
        VERBOSE_PRINTF(ANSI_COLOR_GREEN "Replace transmitter id %04x to %04x\n" ANSI_COLOR_RESET, id, it->second );
        id = it->second; //change id
    }
    // do nothing
}

std::string MQTTClient::GetName( MQTTClient::TYPES type, uint16_t id )
{
    std::string name;
    static std::map<uint16_t, std::string>* pMap=NULL;

    switch( type )
    {
    case TYPE_temperature:
        pMap=&Config::temperature;
        break;

    case TYPE_humidity:
        pMap=&Config::humidity;
        break;

    case TYPE_battery:
        pMap=&Config::battery;
        break;

    case TYPE_quality:
        pMap=&Config::quality;
        break;

    default:
        return name;
    }

    auto it = pMap->find(id);
    if( it != pMap->end() )
    {
        name = it->second;

    }

    const char* standard_name_fmt[4] =
    {
            "Temperature Id:%02X ch %d",
            "Humidity Id:%02X ch %d",
            "Battery Id:%02X ch %d",
            "Quality Id:%02X ch %d"
    };

    if( name.size() == 0 )
    {
        char standard[64];
        snprintf(standard, sizeof(standard), standard_name_fmt[(size_t)type], id >> 8, (id & 0xFF) + 1 );
        name = standard;
    }

    return name;
}
