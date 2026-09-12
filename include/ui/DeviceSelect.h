#ifndef PA_DEVICE_SELECT_H
#define PA_DEVICE_SELECT_H

#include "Screen.h"

#include <string>
#include <vector>
#include <optional>

namespace PA::UI {

struct ActiveDeviceSelect {
  std::string DeviceName;
  std::string Description;
};

using ActiveDeviceSelects = std::vector<ActiveDeviceSelect>; 
using SelectedDevice = std::optional<ActiveDeviceSelect>;

using DeviceSelectBase = Screen<ActiveDeviceSelects, SelectedDevice>;

class DeviceSelectScreen : public Screen<ActiveDeviceSelects, SelectedDevice> {
public:
  DeviceSelectScreen(const ActiveDeviceSelects& activeDevices)
    : DeviceSelectBase(activeDevices), m_ActiveDeviceSelects{activeDevices} {}
  ~DeviceSelectScreen() = default;

  void Render() override;

  void Init() override {}
  void Cleanup() override {}

  SelectedDevice GetResult() override { return m_SelectedDevice; }


private:
  ActiveDeviceSelects m_ActiveDeviceSelects;
  SelectedDevice m_SelectedDevice;

};

}
#endif
