#include <iostream>
#include <vector>
#include <string>
#include <cstring>
#include <stdexcept>
#include <iomanip>
#include <algorithm>

#include <pcap.h>

#include "stdftxui.h"
#include <ftxui/screen/terminal.hpp>

constexpr int LOOKUP_OP_FAILURE {-1};

struct ActiveDevice {
  std::string DeviceName;
  std::string Description;
  std::string ReadableAddress;
};

std::vector<ActiveDevice> ListDevices(); 
ftxui::Element GenerateUIElements(const std::vector<ActiveDevice>&);


using namespace ftxui;

int main([[maybe_unused]]int argc, [[maybe_unused]]char* argv[]) {

  auto devices {ListDevices()};
  
  auto document = GenerateUIElements(devices);

  document->ComputeRequirement();
  const auto required = document->requirement();
  auto screen = Screen(std::max(required.min_x, Terminal::Size().dimx),
                       required.min_y);
  Render(screen, document);
  screen.Print();

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

  return devices; 
}

ftxui::Element GenerateUIElements(const std::vector<ActiveDevice>& devices) {
  std::vector<Element> deviceRows;
  deviceRows.reserve(devices.size());

  std::transform(devices.begin(), devices.end(), std::back_inserter(deviceRows),
		[](const ActiveDevice& device) {
                  return hbox({
                    text(device.DeviceName) | border | color(Color::Green) | size(WIDTH, EQUAL, 30),
		    text(device.Description) | border | flex
		  });
		});
  return vbox({
    hbox({
      text("Device Interface") | border | color(Color::Green) | bold | size(WIDTH, EQUAL, 30),
      text("Description") | border | color(Color::CyanLight) | bold | flex,
    }),
    vbox(std::move(deviceRows))
  });
}

