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

}
