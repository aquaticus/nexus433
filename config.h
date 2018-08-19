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

#include <set>
#include <map>
#include <string>

#define NEXUS433 "nexus433"

class Config
{
public:
  static bool Load(const char* filename);
  static const char* default_config_location;

  struct mqtt
  {
	static std::string host;
	static int port;
	static std::string password;
	static std::string user;
  };

  struct receiver
  {
    static std::string chip;
    static int pin;
    static int resolution_us;
    static int tolerance_us;
  };

  struct transmitter
  {
    static int silent_timeout_sec;
    static int minimum_frames;
	static bool discovery;
    static std::string battery_low;
	static std::string battery_normal;
	static std::string discovery_prefix;
  };

  static std::set<uint16_t> ignored;
  static std::map<uint16_t, uint16_t> substitutes;

  static std::map<uint16_t, std::string> temperature;
  static std::map<uint16_t, std::string> humidity;
  static std::map<uint16_t, std::string> battery;
  static std::map<uint16_t, std::string> quality;
};
