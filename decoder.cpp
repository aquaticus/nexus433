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

#include "decoder.h"
#include <atomic>
#include <thread>
#include <chrono>
#include <iostream>
#ifdef ENABLE_FAKE_TRANSMITTER
#include <stdlib.h>
#endif

//tolerance +/- microseconds
#define LENGTH(x, len) (x<len+m_ToleranceUs && x>len-m_ToleranceUs)

// microseconds
#define PULSE_HIGH_LEN 500
#define PULSE_LOW_ZERO 1000
#define PULSE_LOW_ONE 2000
#define SYNC_GAP_LEN 4000

// Packet consist of 36 bits
#define PACKET_BITS_COUNT 36

void Decoder::Start()
{
    VERBOSE_PRINTF("Decoder resolution: %d µs; tolerance: %d µs\n", m_ResolutionUs, m_ToleranceUs);
    m_ErrorStop = false;
    m_Future = std::move(m_StopFlag.get_future());
    m_pThread = new std::thread([this]	{ this->ThreadFunc();} );
}

#if ENABLE_FAKE_TRANSMITTER

void Decoder::ThreadFunc()
{
    const size_t SENSORCOUNT=3;
    int TimeTable[SENSORCOUNT] = {5, 20, 120};
    int SecCounter[SENSORCOUNT] = {0,0,0};

    std::cout << "TRANSMITTERS ARE SIMULATED. READINGS ARE NOT REAL." << std::endl;

    do
    {
        for(size_t n=0; n<SENSORCOUNT;n++)
        {
            if( SecCounter[n] >= TimeTable[n] )
            {
                SecCounter[n]=0;

                auto temp = rand()%300;
                auto hum = rand()%100;

                uint64_t sensor_bits = (0xABLL+n) << 28; //id
                sensor_bits |= (n%3) << 24; //ch
                sensor_bits |= temp << 12; //temp
                sensor_bits |= 0xF << 8; //1111
                sensor_bits |= hum; //humidity
                sensor_bits |= 1 << 27; //battery

                for(auto i=0; i<rand()%10+2; i++)
                {
                    m_Storage.Add(sensor_bits); 
                }
            }
            else
            {
                SecCounter[n]++;
            }
        }
    } while (std::future_status::timeout == m_Future.wait_for(std::chrono::seconds(1)));
}

#else

void Decoder::ThreadFunc()
{
    DEBUG_PRINTF("DEBUG: Decoder thread started\n");

    auto prev = std::chrono::high_resolution_clock::now();
    auto now=prev;
    int value;
    int prev_value=0;

    do
    {
        value = gpiod_line_get_value(m_Line);
        if( -1 == value )
        {
            printf("Error reading line. errno: %d\n", errno);
            m_ErrorStop = true;
            return;
        }

        if( value != prev_value )
        {
            now = std::chrono::high_resolution_clock::now();
            long delta = std::chrono::duration_cast<std::chrono::microseconds>(now-prev).count();
            prev=now;

            Decode(value==1, delta);
        }

        prev_value = value;

    } while (std::future_status::timeout == m_Future.wait_for(std::chrono::microseconds(m_ResolutionUs)));

    DEBUG_PRINTF("DEBUG: Thread function finished\n");
}

#endif

void Decoder::Decode(bool risingEdge, long long delta)
{
    bool fallingEdge = !risingEdge;

    switch(m_State)
    {
    case STATE_UNKNOWN:
        if( risingEdge )
        {
            m_State=STATE_PULSE_START;
        }
        break;

    case STATE_PULSE_START:
        if( fallingEdge && LENGTH(delta, PULSE_HIGH_LEN) )
        {
            m_State = STATE_PULSE_END;
        }
        else
        {
            m_State = STATE_UNKNOWN;
        }
        break;

    case STATE_PULSE_END:
        if(risingEdge)
        {
            if( LENGTH(delta, PULSE_LOW_ONE ) )
            {
                // "1"
                m_Bits |= (1ULL << (PACKET_BITS_COUNT-1-m_BitCount));
                m_BitCount++;

                m_State=STATE_PULSE_START;
            }
            else if( LENGTH(delta, PULSE_LOW_ZERO ) )
            {
                // "0" (no need to clear bit -- all bits are initially 0)
                m_BitCount++;
                m_State=STATE_PULSE_START;
            }
            else
            {
                m_BitCount=0;
                m_Bits=0;
                m_State=STATE_PULSE_START;
            }
        }
        else
        {
            m_State=STATE_UNKNOWN;
        }
    break;
    }

    if( PACKET_BITS_COUNT == m_BitCount )
    {
        m_Storage.Add(m_Bits);
        m_BitCount=0;
        m_Bits=0;
    }

    if( STATE_UNKNOWN == m_State )
    {
        m_BitCount=0;
        m_Bits=0;
    }
}

void Decoder::Terminate()
{
    if (m_pThread)
    {
        DEBUG_PRINTF("DEBUG: Stopping decoder thread\n");
        m_StopFlag.set_value();
    }
}

bool Decoder::WaitForTermination( uint16_t milliseconds)
{
    auto rv = std::future_status::timeout != m_Future.wait_for(std::chrono::milliseconds(milliseconds));
    if( rv == true )
    {
        m_pThread->join();
        delete m_pThread;
        m_pThread=NULL;
    }
    return rv;
}
