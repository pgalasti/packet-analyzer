#include "stdftxui.h"
#include "Defines.h"

#include "core/Capture.h"
#include "core/Device.h"
#include "ui/DeviceSelect.h"
#include "ui/DeviceLanding.h"

#include <exception>
#include <iostream>
#include <ranges>
#include <utility>

using namespace ftxui;

int main([[maybe_unused]]int argc, [[maybe_unused]]char* argv[]) {

  FILE_TRACE_LOG("Starting up...");

  const auto devices {PA::Core::ListDevices()};
  const PA::UI::ActiveDeviceSelects deviceSelections { devices 
    | std::views::transform([](const PA::Core::Device& device) { 
        return PA::UI::ActiveDeviceSelect{device.DeviceName, device.Description}; 
      })
    | std::ranges::to<PA::UI::ActiveDeviceSelects>() };
  FILE_TRACE_LOG("Devices Fetched:");
  FILE_TRACE_LOG(devices.size());

  PA::UI::DeviceSelectScreen deviceSelectScreen(deviceSelections);
  deviceSelectScreen.Render();
  
  auto selection {deviceSelectScreen.GetResult()};
  if(!selection) {
    std::cout << "No device selected.\n";
    return 1;
  }

  FILE_TRACE_LOG("Selected device: " << selection->DeviceName);

  PA::UI::DeviceLandingScreen landingScreen(selection->DeviceName);
  try {
    PA::Core::PacketCapture capture(selection->DeviceName);
    landingScreen.Init();
    capture.Start([&landingScreen](PA::Core::Direction direction, PA::Core::PacketRecord record) {
      landingScreen.Record(direction, std::move(record));
    });
    landingScreen.Render();
    capture.Stop();
  } catch(const std::exception& e) {
    FILE_TRACE_LOG("Capture failed: " << e.what());
    std::cerr << e.what() << '\n';
    return 1;
  }

  return 0;
}

