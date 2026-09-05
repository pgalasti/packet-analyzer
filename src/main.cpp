#include "stdftxui.h"

#include "core/Device.h"
#include "ui/DeviceSelect.h"

#include <iostream>
#include <vector>
#include <ranges>
#include <algorithm>

using namespace ftxui;

int main([[maybe_unused]]int argc, [[maybe_unused]]char* argv[]) {

  const auto devices {PA::Core::ListDevices()};
  const PA::UI::ActiveDeviceSelects deviceSelections { devices 
    | std::views::transform([](const PA::Core::Device& device) { 
        return PA::UI::ActiveDeviceSelect{device.DeviceName, device.Description}; 
      })
    | std::ranges::to<PA::UI::ActiveDeviceSelects>() };

  PA::UI::DeviceSelectScreen deviceSelectScreen(deviceSelections);
  deviceSelectScreen.Render();
  
  auto selection {deviceSelectScreen.GetResult()};
  if(!selection) {
    std::cout << "No device selected.\n";
    return 1;
  }

  std::cout << "Capturing on: " << selection->DeviceName << '\n';

  return 0;
}

