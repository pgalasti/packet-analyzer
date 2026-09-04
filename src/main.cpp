#include <iostream>
#include <vector>
#include <string>
#include <cstring>
#include <stdexcept>

#include <pcap.h>

constexpr int LOOKUP_OP_FAILURE {-1};

struct ActiveDevice {
  std::string DeviceName;
  std::string ReadableAddress;
};

std::vector<ActiveDevice> ListDevices(); 

int main(int argc, char* argv[]) {

  auto devices {ListDevices()};

  for(auto device : devices) {
    std::cout << device.DeviceName << std::endl;
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
 
  auto GetDeviceDetails = [](pcap_if_t* pAllDevs) -> ActiveDevice {
    char szDeviceName[128];
    strncpy(szDeviceName, pAllDevs->name, 127);
    szDeviceName[127] = '\0';
    return {std::string(szDeviceName), ""};
  };

  for(auto pDev{pAllDevs}; pDev != nullptr; pDev = pDev->next) {
    devices.push_back(GetDeviceDetails(pDev));
  }

  return devices; 
}
