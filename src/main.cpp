#include "stdftxui.h"
#include "ui/DeviceSelect.h"

#include <iostream>
#include <vector>
#include <string>
#include <cstring>
#include <stdexcept>

#include <pcap.h>


constexpr int LOOKUP_OP_FAILURE {-1};

PA::UI::ActiveDevices ListDevices();

using namespace ftxui;

int main([[maybe_unused]]int argc, [[maybe_unused]]char* argv[]) {

  auto devices {ListDevices()};

  PA::UI::DeviceSelectScreen deviceSelectScreen(devices);
  deviceSelectScreen.Render();
  
  auto selection {deviceSelectScreen.GetResult()};
  if(!selection) {
    std::cout << "No device selected.\n";
    return 1;
  }

  std::cout << "Capturing on: " << selection->DeviceName << '\n';

  return 0;
}

PA::UI::ActiveDevices ListDevices() {
  PA::UI::ActiveDevices devices;
  
  char szErrorBuffer[PCAP_ERRBUF_SIZE];
  pcap_if_t* pAllDevs;

  auto opResponse = pcap_findalldevs(&pAllDevs, szErrorBuffer);
  if(opResponse == LOOKUP_OP_FAILURE || pAllDevs == nullptr) { 
    throw std::runtime_error("Unable to determine network devices!");
  }
 
  auto GetDeviceDetails = [](pcap_if_t* pDev) -> PA::UI::ActiveDevice {
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
