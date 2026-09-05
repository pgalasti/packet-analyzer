#include "stdftxui.h"

#include "core/Device.h"
#include "ui/DeviceSelect.h"

#include <iostream>
#include <vector>
#include <ranges>
#include <algorithm>

using namespace ftxui;

void packetCallback(u_char* args, const struct pcap_pkthdr* pHeader, const u_char* packet) {
  std::cout << "Packet length grabbed: " << pHeader->len << std::endl;
}

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

  auto pHandle {PA::Core::LiveCapture(selection->DeviceName.c_str())};
  pcap_loop(pHandle, 10, packetCallback, nullptr);

  pcap_close(pHandle);

  return 0;
}

