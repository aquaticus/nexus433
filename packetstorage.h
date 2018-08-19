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

#include "diag.h"
#include "packet.h"
#include "istorage.h"
#include <map>
#include <list>
#include <mutex>
#include <ctime>
#include <inttypes.h>
#include <set>

class StorageItem
{
public:
    StorageItem(Packet p)
    {
        m_Packet = p;
        m_Timestamp = std::chrono::system_clock::now();
        m_NewTransmitter = true;
    }

    bool IsNewTransmitter()
    {
        return m_NewTransmitter;
    }

    void ClearNewTransmitter()
    {
        m_NewTransmitter = false;
    }

    void UpdateTimestamp()
    {
        m_Timestamp = std::chrono::system_clock::now();
    }

    std::chrono::system_clock::time_point GetTimestamp()
    {
        return m_Timestamp;
    }

    void NewPacket(Packet& packet)
    {
        m_Packet.SetRaw(packet.GetRaw());
        UpdateTimestamp();
        m_Packet.ResetFrameCounter();
        m_Packet.IncreaseFrameCounter();
    }

    Packet& GetPacket()
    {
        return m_Packet;
    }

#ifdef DEBUG
    void Dump()
    {
        DEBUG_PRINTF("DEBUG: 0x%" PRIx64 "; FrameCounter=%d; m_NewSender=%d\n", m_Packet.GetRaw(),
                m_Packet.GetFrameCounter(), m_NewTransmitter);
    }
#endif

protected:
    Packet m_Packet;
    std::chrono::system_clock::time_point m_Timestamp;
    bool m_NewTransmitter;
};

class PacketStorage: public IStorage
{
public:
    PacketStorage()
    {
    }

    virtual ~PacketStorage(){}

    void Add(uint64_t packet);
    void UpdateStatus(std::set<Packet>& NewReadings, std::set<uint16_t>& RemovedTransmitters, std::set<uint16_t>& NewTransmitters);
    std::list<uint16_t> GetActiveTransmitters();

    static uint32_t GetSilentTimeoutSec()
    {
        return m_SilentTimeoutSec;
    }

    static void SetSilentTimeoutSec(uint32_t ms)
    {
        m_SilentTimeoutSec = ms;
    }

protected:
    std::map<uint16_t, StorageItem> m_Items;
    std::mutex m_Lock;
    static uint32_t m_SilentTimeoutSec; //depending on sensor brand, this may be ~60 or ~80 secs
    const uint32_t m_ReadingMaxAgeMs = 1300; // [ms] everything that comes after 1s is treated as new reading ~(36 * (2500us+500us) * 12)
};
