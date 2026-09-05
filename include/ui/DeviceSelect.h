#ifndef PA_DEVICE_SELECT_H
#define PA_DEVICE_SELECT_H

#include "Screen.h"

#include <string>
#include <vector>
#include <optional>

namespace PA::UI {

struct ActiveDevice {
  std::string DeviceName;
  std::string Description;
  std::string ReadableAddress;
};

using ActiveDevices = std::vector<ActiveDevice>; 
using SelectedDevice = std::optional<ActiveDevice>;

using Base = Screen<ActiveDevices, SelectedDevice>;

class DeviceSelectScreen : public Screen<ActiveDevices, SelectedDevice> {
public:
  DeviceSelectScreen(const ActiveDevices& activeDevices)
    : Base(activeDevices), m_ActiveDevices{activeDevices} {}
  ~DeviceSelectScreen() = default;

  void Render() override;

  void Init() override {}
  void Cleanup() override {}

  SelectedDevice GetResult() override { return m_SelectedDevice; }


private:
  ActiveDevices m_ActiveDevices;
  SelectedDevice m_SelectedDevice;

};

}
#endif
