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

#pragma once

#include <mosquittopp.h>
#include "packet.h"
#include "config.h"

using namespace mosqpp;

class MQTTClient : public mosquittopp
{
public:
	MQTTClient(const char* id) : mosquittopp(id)
	{
		lib_init();
		m_Connected=false;
	}

	~MQTTClient()
	{

	}

    enum TYPES
    {
        TYPE_temperature=0,
        TYPE_humidity=1,
        TYPE_battery=2,
        TYPE_quality=3
    };

	int Connect(const char* host, int port);

	void SensorUpdate(uint16_t id, const Packet& packet);

	int SensorStatus(uint16_t id, bool online);

	void SensorDiscover(uint16_t id);

	void OnlineStatus(bool online);

	bool IsConnected()
	{
		return m_Connected;
	}
protected:
	void Substitute(uint16_t& id);
	std::string GetName( MQTTClient::TYPES type, uint16_t id );

	void on_connect(int rc);
	void on_disconnect(int rc);
	const char* const m_StateTopic = NEXUS433 "/sensor/%04x/state";
	const char* const m_AvailTopic = NEXUS433 "/sensor/%04x/connection";
	bool m_Connected;
};
