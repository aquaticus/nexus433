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

#include <stdint.h>
#include <stdio.h>
#include <cstddef>

/*
* the data is grouped in 9 nibbles
* [0x9(4)] [id(8)] [lowBatt(1)] [test(1)] [channel(2)] [temp(12)] [0x0(1)] [humi(7)] [0x0(1)]
*
* The 8-bit id changes when the battery is changed in the sensor.
* test button pressed
* channel: 0=CH1, 1=CH2, 2=CH3
* temp is 12 bit signed scaled by 10
* humidity is 7 bits  10==LL 110=HH
* not sure about fixed constant in packet, they are not changing, may be extended resolution for future use?
*/

class Packet
{
public:
  Packet(uint64_t raw)
  {
    m_Raw = raw;
    m_FrameCounter = 0;
  }

  Packet()
  {
    m_Raw=0;
    m_FrameCounter=0;
  }

  uint16_t GetSenderId() const
  {
    return GetId() << 8 | GetChannel();
  }

  uint8_t GetId() const
  {
    return (m_Raw >> 25) & 0xFF;
  }

  uint8_t GetChannel() const
  {
    return (m_Raw >> 21) & 0x03;
  }

  // Returns temperature * 10
  int16_t GetTemperature10() const
  {
    int16_t t = m_Raw >> 9 & 0x0FFF;
    return 0x0800 & t ? 0xF000 | t  : t;
  }

  // Returns temperature as double
  double GetTemperature() const
  {
    return GetTemperature10() / 10.0;
  }

  uint8_t GetHumidity() const
  {
    return m_Raw>>1 & 0x7F;
  }

  uint8_t GetConst() const
  {
    return 0x0F;
  }

  uint8_t GetBattery() const
  {
    return ~(m_Raw >> 24) & 0x01;
  }

  bool IsValid() const
  {
    return
      GetConst() == 0x0F &&
      GetHumidity() <= 110 &&
      GetTemperature10() > -400 &&
      GetTemperature10() < 600; //arbitrary chosen valid range
  }

  uint64_t GetRaw() const
  {
    return m_Raw;
  }

  void SetRaw(uint64_t packet)
  {
    m_Raw = packet;
  }

  uint8_t IncreaseFrameCounter()
  {
      if( m_FrameCounter < 0xFF )
      {
          m_FrameCounter++;
      }

      return m_FrameCounter;
  }

    void ResetFrameCounter()
    {
        m_FrameCounter=0;
    }

  uint8_t GetFrameCounter() const
  {
      return m_FrameCounter;
  }

  uint8_t GetQualityPercent() const
  {
      return ((m_FrameCounter * 100) / 7);
  }

  void Substitute(uint16_t newId)
  {
      uint64_t id = newId >> 8;
      uint64_t ch = newId & 0x0003;

      m_Raw &= ~(0xFFLLU << 25);
      m_Raw |= (id << 25 );

      m_Raw &= ~(0x03LLU << 21);
      m_Raw |= (ch << 21);
  }

  inline bool operator< (const Packet& rhs) const { return m_Raw < rhs.GetRaw(); }
  inline bool operator== (const Packet& rhs) const { return m_Raw == rhs.GetRaw(); }
  inline operator uint64_t() const { return m_Raw; }

protected:
  uint64_t m_Raw;
  uint8_t m_FrameCounter;
};
