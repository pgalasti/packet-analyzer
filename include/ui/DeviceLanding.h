#ifndef PA_DEVICE_LANDING_H
#define PA_DEVICE_LANDING_H

#include "Screen.h"

#include <string>

namespace PA::UI {

using Base = Screen<std::string, std::string>

class DeviceLandingScreen : public Screen<std::string, std::string> {
public:
  DeviceLandingScreen(const& std::string tbd)
    : Base(tbd), m_tbd{tbd} {}
  ~DeviceLandingScreen() = default;

  void Render() override{}

  void Init override{}
  void Cleanup override{}

  std::string GetResult() override {return "";}

protected:
  std::string m_tbd;
};

}

#endif
