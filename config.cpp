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

#include <iostream>
#include "config.h"
#include "xini.h"
#include "board.h"

//default values
const char* Config::default_config_location=INI_FILEPATH;

string Config::mqtt::host = "127.0.0.1";
int Config::mqtt::port = 1883;
string Config::mqtt::password="";
string Config::mqtt::user="";
string Config::mqtt::clientid=NEXUS433;
std::string Config::mqtt::cafile;
std::string Config::mqtt::capath;
std::string Config::mqtt::certfile;
std::string Config::mqtt::keyfile;
std::string Config::mqtt::keypass;

string Config::receiver::chip=GPIOD_DEFAULT_DEVICE;
int Config::receiver::pin=GPIOD_DEFAULT_PIN;
int Config::receiver::resolution_us=1;
int Config::receiver::tolerance_us=300;
std::string Config::receiver::internal_led="";

int Config::transmitter::silent_timeout_sec=90;
int Config::transmitter::minimum_frames=2;
string Config::transmitter::battery_low="0";
string Config::transmitter::battery_normal="100";
bool Config::transmitter::discovery=false;
string Config::transmitter::discovery_prefix="homeassistant";

std::set<uint16_t> Config::ignored;
std::map<uint16_t, uint16_t> Config::substitutes;

std::map<uint16_t, std::string> Config::temperature;
std::map<uint16_t, std::string> Config::humidity;
std::map<uint16_t, std::string> Config::battery;
std::map<uint16_t, std::string> Config::quality;

bool Config::Load(const char* filename)
{
  //INIReader ini(filename);
    XIni ini(filename);

  if (ini.ParseError() < 0)
  {
      return false;
  }

  Config::mqtt::host = ini.Get("mqtt", "host", Config::mqtt::host);
  Config::mqtt::port = ini.GetInteger("mqtt", "port", Config::mqtt::port);
  Config::mqtt::password = ini.Get("mqtt", "password", Config::mqtt::password);
  Config::mqtt::user=ini.Get("mqtt", "user", Config::mqtt::user);
  Config::mqtt::clientid=ini.Get("mqtt", "clientid", Config::mqtt::clientid);

  Config::mqtt::cafile=ini.Get("mqtt", "cafile", "");
  Config::mqtt::capath=ini.Get("mqtt", "capath", "");
  Config::mqtt::certfile=ini.Get("mqtt", "certfile", "");
  Config::mqtt::keyfile=ini.Get("mqtt", "keyfile", "");
  Config::mqtt::keypass=ini.Get("mqtt", "keypass", "");

  Config::receiver::chip=ini.Get("receiver", "chip", Config::receiver::chip);
  Config::receiver::pin=ini.GetInteger("receiver", "pin", Config::receiver::pin);
  Config::receiver::resolution_us=ini.GetInteger("receiver", "resolution_us", Config::receiver::resolution_us);
  Config::receiver::tolerance_us=ini.GetInteger("receiver", "tolerance_us", Config::receiver::tolerance_us);
  Config::receiver::internal_led=ini.Get("receiver", "internal_led", Config::receiver::internal_led);

  Config::transmitter::silent_timeout_sec=ini.GetInteger("transmitter", "silent_timeout_ms", Config::transmitter::silent_timeout_sec);
  Config::transmitter::minimum_frames=ini.GetInteger("transmitter", "minimum_frames", Config::transmitter::minimum_frames);
  Config::transmitter::discovery=ini.GetBoolean("transmitter", "discovery", Config::transmitter::discovery);
  Config::transmitter::discovery_prefix = ini.Get("transmitter", "discovery_prefix", Config::transmitter::discovery_prefix);
  Config::transmitter::battery_low = ini.Get("transmitter", "battery_low", Config::transmitter::battery_low);
  Config::transmitter::battery_normal = ini.Get("transmitter", "battery_normal", Config::transmitter::battery_normal);

  ini.GetIgnored(ignored);
  ini.GetSubstitutes(substitutes);

  ini.GetNames("temperature", Config::temperature);
  ini.GetNames("humidity", Config::humidity);
  ini.GetNames("battery", Config::battery);
  ini.GetNames("quality", Config::quality);

  return true;
}
