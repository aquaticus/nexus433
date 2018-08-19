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
#include "packetstorage.h"
#include <string.h>

int MQTTClient::Connect(const char* host, int port)
{
    const char offline[]="offline";
    if( Config::mqtt::user.size() > 0)
    {
        username_pw_set(Config::mqtt::user.c_str(), Config::mqtt::password.c_str());
    }
    will_set(NEXUS433 "/connection", sizeof(offline)-1, "offline", 0, true );

    return connect(host, port, 60);
}

void MQTTClient::on_connect(int rc)
{
    if( rc == 0 )
    {
        printf("Connected to MQTT broker.\n");
        m_Connected = true;
    }
    else
    {
        m_Connected = false;
    }
}

void MQTTClient::on_disconnect(int rc)
{
    DEBUG_PRINTF("MQTT Disconnected\n");
    m_Connected = false;
}

void MQTTClient::SensorUpdate(uint16_t id, const Packet& packet)
{
    Substitute(id);

    const char* payload_fmt = "{ "
            "\"temperature\": %d.%d, "
            "\"humidity\": %d, "
            "\"battery\": \"%s\", "
            "\"quality\": %d "
            "}";

    char payload[256];
    snprintf(payload, sizeof(payload),
            payload_fmt,
            packet.GetTemperature() / 10,
            packet.GetTemperature() % 10,
            packet.GetHumidity(),
            packet.GetBattery() ? Config::transmitter::battery_normal.c_str() : Config::transmitter::battery_low.c_str(),
            packet.GetQualityPercent()
    );

    char topic[256];
    snprintf(topic, sizeof(topic), m_StateTopic, id);

    // publish
    publish(NULL, topic, strlen(payload), payload);

}

int MQTTClient::SensorStatus(uint16_t id, bool online)
{
    Substitute(id);

    const char* const payload = online ? "online" : "offline";
    char topic[256];
    snprintf(topic, sizeof(topic), m_AvailTopic, id);
    publish(NULL, topic, strlen(payload), payload);

    return 0;
}

void MQTTClient::SensorDiscover(uint16_t id)
{
    Substitute(id);

    const char* topic_fmt[4] =
    {
            "%s/sensor/" NEXUS433 "_%04x_temperature/config",
            "%s/sensor/" NEXUS433 "_%04x_humidity/config",
            "%s/sensor/" NEXUS433 "_%04x_battery/config",
            "%s/sensor/" NEXUS433 "_%04x_quality/config"
    };

    const char* units[4] = { "°C", "%", "%", "%" };
    const char* value[4] = { "temperature", "humidity", "battery", "quality" };
    const char* device_class[4] = { "temperature", "humidity", "battery", "None" };

    std::string name;
    char topic[256];
    char payload[512];
    char state_topic[256];

    for (int i = 0; i < 4; i++)
    {
        snprintf(topic, sizeof(topic), topic_fmt[i], Config::transmitter::discovery_prefix.c_str(), id);
        name = GetName((TYPES)i, id);
        snprintf(state_topic, sizeof(state_topic), m_StateTopic, id);

        snprintf(payload, sizeof(payload),
                "{"
                "\"name\": \"%s\", "
                "\"device_class\": \"%s\", "
                "\"state_topic\": \"%s\", "
                "\"availability_topic\": \"" NEXUS433 "/connection\", "
                "\"unit_of_measurement\": \"%s\", "
                "\"value_template\": \"{{ value_json.%s }}\", "
                "\"expire_after\": %d"
                "}",
                name.c_str(),
                device_class[i],
                state_topic,
                units[i],
                value[i],
                PacketStorage::GetSilentTimeoutSec());

        // publish
        publish(NULL, topic, strlen(payload), payload);
    }
}

void MQTTClient::OnlineStatus(bool online)
{
    const char* payload = online ? "online" : "offline";

    publish(NULL, NEXUS433 "/connection", strlen(payload), payload, 0, true);
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
            "433MHz Sensor Id:%02X channel %d Temperature",
            "433MHz Sensor Id:%02X channel %d Humidity",
            "433MHz Sensor Id:%02X channel %d Battery",
            "433MHz Sensor Id:%02X channel %d Quality"
    };

    if( name.size() == 0 )
    {
        char standard[64];
        snprintf(standard, sizeof(standard), standard_name_fmt[(size_t)type], id >> 8, (id & 0xFF) + 1 );
        name = standard;
    }

    return name;
}
