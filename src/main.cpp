#include <iostream>
#include <vector>
#include <string>
#include <cstring>
#include <stdexcept>
#include <optional>
#include <unordered_map>

#include <pcap.h>

#include "stdftxui.h"

constexpr int LOOKUP_OP_FAILURE {-1};
constexpr int NAME_COLUMN_WIDTH {24};

struct ActiveDevice {
  std::string DeviceName;
  std::string Description;
  std::string ReadableAddress;
};

std::vector<ActiveDevice> ListDevices();
std::optional<ActiveDevice> SelectDevice(const std::vector<ActiveDevice>&);


using namespace ftxui;

int main([[maybe_unused]]int argc, [[maybe_unused]]char* argv[]) {

  auto devices {ListDevices()};

  auto selection {SelectDevice(devices)};
  if(!selection) {
    std::cout << "No device selected.\n";
    return 1;
  }

  std::cout << "Capturing on: " << selection->DeviceName << '\n';

  return 0;

}


std::vector<ActiveDevice> ListDevices() {
  std::vector<ActiveDevice> devices;
  
  char szErrorBuffer[PCAP_ERRBUF_SIZE];
  pcap_if_t* pAllDevs;

  auto opResponse = pcap_findalldevs(&pAllDevs, szErrorBuffer);
  if(opResponse == LOOKUP_OP_FAILURE || pAllDevs == nullptr) { 
    throw std::runtime_error("Unable to determine network devices!");
  }
 
  auto GetDeviceDetails = [](pcap_if_t* pDev) -> ActiveDevice {
    char szBuffer[128];
    strncpy(szBuffer, pDev->name, 127);
    szBuffer[127] = '\0';
    std::string deviceName {szBuffer};

    std::string description;
    if(pDev->description) {
      strncpy(szBuffer, pDev->description, 127);
      szBuffer[127] = '\0';
      description = szBuffer;
    }
    
    return {deviceName, 
	    description.empty() ? "No Description Available" : description, 
	    ""
    };
  };

  for(auto pDev{pAllDevs}; pDev != nullptr; pDev = pDev->next) {
    devices.push_back(GetDeviceDetails(pDev));
  }

  pcap_freealldevs(pAllDevs);

  return devices; 
}

std::optional<ActiveDevice> SelectDevice(const std::vector<ActiveDevice>& devices) {
  if(devices.empty()) {
    return std::nullopt;
  }

  std::vector<std::string> deviceNames;
  std::unordered_map<std::string, std::string> descriptions;
  deviceNames.reserve(devices.size());
  for(const auto& device : devices) {
    deviceNames.push_back(device.DeviceName);
    descriptions.emplace(device.DeviceName, device.Description);
  }

  int selected {0};
  bool confirmed {false};

  auto screen {ScreenInteractive::Fullscreen()};

  auto menuOption {MenuOption::Vertical()};
  menuOption.entries = &deviceNames;
  menuOption.selected = &selected;
  menuOption.on_enter = [&] {
    confirmed = true;
    screen.Exit();
  };
  menuOption.entries_option.transform = [&descriptions](const EntryState& state) {
    auto row = hbox({
      text(state.active ? " > " : "   "),
      text(state.label) | size(WIDTH, EQUAL, NAME_COLUMN_WIDTH),
      text(descriptions.at(state.label)) | flex,
    });

    if(state.active) {
      return row | bold | color(Color::Black) | bgcolor(Color::Green);
    }
    return row | color(Color::GrayLight);
  };

  auto menu = Menu(menuOption);

  auto renderer = Renderer(menu, [&] {
    return vbox({
      text(" Select a capture interface ") | bold | color(Color::Green),
      separator(),
      hbox({
        text("   "),
        text("INTERFACE") | size(WIDTH, EQUAL, NAME_COLUMN_WIDTH),
        text("DESCRIPTION") | flex,
      }) | bold | color(Color::CyanLight),
      separator(),
      menu->Render() | vscroll_indicator | yframe | flex,
      separator(),
      hbox({
        text(" up/down ") | bold, text("move   "),
        text("enter ") | bold, text("select   "),
        text("q ") | bold, text("quit"),
      }) | color(Color::GrayDark),
    }) | border | color(Color::Green);
  });

  renderer |= CatchEvent([&](Event event) {
    if(event == Event::Character('q') || event == Event::Escape) {
      screen.Exit();
      return true;
    }
    return false;
  });

  screen.Clear();
  screen.Loop(renderer);

  if(!confirmed) {
    return std::nullopt;
  }
  return devices.at(static_cast<std::size_t>(selected));
}
