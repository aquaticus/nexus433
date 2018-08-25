#pragma once

#include <string>

class Led
{
public:
  Led():
  m_InternalLed(-1)
  {

  }

  ~Led()
  {
    Close();
  }

  bool Open(std::string device);
  void Close();
  void Control(bool on);

protected:
  int m_InternalLed;
};
