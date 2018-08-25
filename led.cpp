#include "led.h"
#include <fcntl.h>
#include <string.h>
#include <unistd.h>

void Led::Control(bool on)
{
  const char* const von="255";
  const char* const voff="0";
  if( m_InternalLed > 0 )
  {
    const char* val = on ? von : voff;
    write(m_InternalLed, val, strlen(val) );
  }
}

bool Led::Open(std::string device)
{
  if( m_InternalLed < 0 )
  {
    std::string path("/sys/class/leds/");
    std::string dev =  path + device + "/brightness";

    m_InternalLed = open(dev.c_str(), O_WRONLY);
  }

  if( m_InternalLed < 0 )
    return false;

  return true;
}

void Led::Close()
{
  if( m_InternalLed < 0)
  {
    close(m_InternalLed);
    m_InternalLed=-1;
  }
}
