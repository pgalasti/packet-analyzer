#include "core/Device.h"

#include <stdexcept>
#include <cstring>

#include <pcap.h>

namespace PA::Core {

Devices ListDevices() {
  Devices devices;
  char szErrorBuffer[PCAP_ERRBUF_SIZE];
  pcap_if_t* pAllDevs;

  auto opResponse {pcap_findalldevs(&pAllDevs, szErrorBuffer)};
  if(opResponse == LOOKUP_OP_FAILURE || pAllDevs == nullptr) {
    throw std::runtime_error("Unable to determine network devices!");
  }

  auto GetDeviceDetails = [](pcap_if_t* pDev) -> Device {
    char szBuffer[PA_PCAP_BUF_SIZE];
    strncpy(szBuffer, pDev->name, PA_PCAP_BUF_SIZE-1);
    szBuffer[PA_PCAP_BUF_SIZE-1] = '\0';
    std::string deviceName {szBuffer};

    std::string description;
    if(pDev->description) {
      strncpy(szBuffer, pDev->description, PA_PCAP_BUF_SIZE-1);
      szBuffer[PA_PCAP_BUF_SIZE-1] = '\0';
      description = szBuffer;
    }

    return {
      deviceName, 
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

pcap_t* LiveCapture(const char* pszDevice) {
#ifndef NDEBUG
  std::cout << "Attempting to live caputre for device:" << pszDevice << "..." << std::endl;
#endif

  char szErrorBuffer[PCAP_ERRBUF_SIZE];
  pcap_t* pHandle {pcap_open_live(pszDevice, PCAP_LISTEN_BUF_SIZE, 1, 1000, szErrorBuffer)};
  if(pHandle == nullptr) {
    throw std::runtime_error(std::string("Unable to open device ") + pszDevice
      + " for live capture: " + szErrorBuffer);
  }

#ifndef NDEBUG
  std::cout << "Live capture for " << pszDevice << " is open" << std::endl;
#endif
  return pHandle;
}


}
