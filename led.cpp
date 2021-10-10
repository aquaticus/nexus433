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
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-result"
    write(m_InternalLed, val, strlen(val) );
#pragma GCC diagnostic pop
  }
}

bool Led::Open(std::string device)
{
  const std::string path("/sys/class/leds/");
  if( m_InternalLed < 0 )
  {
    std::string trigger =  path + device + "/trigger";
    int fd = open(trigger.c_str(), O_WRONLY);
    if( fd < 0 )
    {
      return false;
    }
    else
    {
      const char none[] = "none";
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-result"
      write(fd, none, sizeof(none));
#pragma GCC diagnostic pop
      close(fd);
    }

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
