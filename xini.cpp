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

#include "xini.h"

void XIni::GetIgnored(std::set<uint16_t>& ignored)
{
    const char prefix[] ="ignore=";

    for(auto& item: _values )
    {
        if( 0 == item.first.find(prefix, 0) )
        {
            string low(item.second);
            std::transform(low.begin(), low.end(), low.begin(), ::tolower);

            if( low == "yes" || low == "true" || low == "1")
            {
                string val = item.first.substr(sizeof(prefix)-1);
                uint16_t numeric = (uint16_t) std::stoul(val.c_str(), nullptr, 16);
                ignored.insert(numeric);
            }
        }
    }
}


void XIni::GetSubstitutes(std::map<uint16_t, uint16_t>& subs)
{
    const char prefix[] ="substitute=";

    for(auto& item: _values )
    {
        if( 0 == item.first.find(prefix, 0) )
        {
            string val = item.first.substr(sizeof(prefix)-1);
            uint16_t from = (uint16_t) std::stoul(val.c_str(), nullptr, 16);
            uint16_t to = (uint16_t) std::stoul(item.second.c_str(), nullptr, 16);
            subs.insert(std::pair<uint16_t, uint16_t>(from, to));
        }
    }
}


void XIni::GetNames(const char* section, std::map<uint16_t, std::string>& names)
{
    string prefix(section);
    prefix += '=';

    for(auto& item: _values )
    {
        if( 0 == item.first.find(prefix, 0) )
        {
            string val = item.first.substr(prefix.size());
            uint16_t id = (uint16_t) std::stoul(val.c_str(), nullptr, 16);
            names.insert(std::pair<uint16_t, string>(id, item.second));
        }
    }
}
