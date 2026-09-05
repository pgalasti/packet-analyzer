#include <iostream>
#include <vector>
#include <string>
#include <cstring>
#include <stdexcept>
#include <iomanip>

#include <pcap.h>

constexpr int LOOKUP_OP_FAILURE {-1};

struct ActiveDevice {
  std::string DeviceName;
  std::string Description;
  std::string ReadableAddress;
};

std::vector<ActiveDevice> ListDevices(); 

int main([[maybe_unused]]int argc, [[maybe_unused]]char* argv[]) {

  auto devices {ListDevices()};

  for(auto device : devices) {
    std::cout << std::left << std::setw(20) 
      << device.DeviceName << std::setw(10)
      << " - " << std::left << std::setw(30) 
      << device.Description << std::endl;
  }

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
