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

#include <chrono>
#include <ctime>
#include <map>
#include <iostream>
#include <iterator>
#include <unistd.h>
#include <inttypes.h>
#include "color.h"
#include "packetstorage.h"
#include "diag.h"
#include "packet.h"

uint32_t PacketStorage::m_SilentTimeoutSec = 90; //[secs]

void PacketStorage::Add(uint64_t raw)
{
    Packet packet(raw);

    // check packet. 0 is not valid.
    if (!packet.IsValid())
    {
        DEBUG_PRINTF("DEBUG: Received invalid packet. %d\n", 0);
        return;
    }

    // id + channel number
    uint16_t id = packet.GetSenderId();

    m_Lock.lock();

    auto pos = m_Items.find(id);
    if (pos == m_Items.end())
    {
        //not found

        DEBUG_PRINTF("DEBUG: New transmitter 0x%02x\n", id);
        DEBUG_PRINTF("DEBUG: New reading 0x%" PRIx64 " from transmitter 0x%02x\n", packet.GetRaw(), id);

        packet.IncreaseFrameCounter();
        StorageItem n(packet);
        m_Items.insert(std::pair<uint16_t, StorageItem>(id, n));
    }
    else
    {
        //found
        StorageItem& n = pos->second;
        if (n.GetPacket().GetRaw() == packet.GetRaw())
        {
            n.GetPacket().IncreaseFrameCounter();
            n.UpdateTimestamp();
#ifdef DEBUG_MORE
            DEBUG_PRINTF("DEBUG: New frame #%d 0x%" PRIx64 " from transmitter 0x%02x\n", n.GetPacket().GetFrameCounter(), packet.GetRaw(), id);
#endif
        }
        else
        {
            DEBUG_PRINTF("DEBUG: New reading 0x%" PRIx64 " from transmitter 0x%02x\n", packet.GetRaw(), id);
            n.NewPacket(packet);
        }

    }

    m_Lock.unlock();
}

void PacketStorage::UpdateStatus(std::set<Packet>& NewReadings, std::set<uint16_t>& RemovedTransmitters,
        std::set<uint16_t>& NewTransmitters)
{
    auto now = std::chrono::system_clock::now();
    Packet subst;

    m_Lock.lock();

    for (auto& i : m_Items)
    {
        auto &item = i.second;

        uint32_t age = std::chrono::duration_cast<std::chrono::milliseconds>(now - item.GetTimestamp()).count();

        // check if new station is available
        if (item.IsNewTransmitter())
        {
            NewTransmitters.insert(item.GetPacket().GetSenderId());
            item.ClearNewTransmitter();

            DEBUG_PRINTF("DEBUG: UpdateStatus: New transmitter is available 0x%02x.\n", item.GetPacket().GetSenderId());
        }

        //if no new readings within 1 minute treat station as dead
        if (age > GetSilentTimeoutSec()*1000)
        {
            DEBUG_PRINTF("DEBUG: Transmitter 0x%02x become silent.\n", i.first);
            RemovedTransmitters.insert(i.first);
        }
        else if (age >= m_ReadingMaxAgeMs && item.GetPacket().GetRaw() != 0) // if reading comes after 1s from the last one is treated as a new reading
        {
            NewReadings.insert(item.GetPacket());
            item.GetPacket().SetRaw(0); //set to invalid value. Next reading will update entry as new but won't indicate new sender
        }
    }

    for (auto item : RemovedTransmitters)
    {
        DEBUG_PRINTF("DEBUG: Removing transmitter %02x from list of active stations\n", item);
        m_Items.erase(item);
    }

    m_Lock.unlock();
}

std::list<uint16_t> PacketStorage::GetActiveTransmitters()
{
    std::list<uint16_t> transmitters;

    m_Lock.lock(); //iterator is const, so it should be thread safe but just in case

    for (const auto& it : m_Items)
    {
        transmitters.push_back(it.first);
    }

    m_Lock.unlock();

    return transmitters;
}

int PacketStorage::GetActiveTransmittersCount()
{
    return m_Items.size();
}