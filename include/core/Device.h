#ifndef PA_DEVICE_H
#define PA_DEVICE_H

#include <pcap.h>

#include <string>
#include <vector>
#include <stdexcept>
#include <iostream>

namespace PA::Core {

constexpr int LOOKUP_OP_FAILURE {-1};
constexpr size_t PA_PCAP_BUF_SIZE {128};
constexpr size_t PCAP_LISTEN_BUF_SIZE {512};

struct Device {
  std::string DeviceName;
  std::string Description;
  std::string AddressStr;
};
using Devices = std::vector<Device>;

Devices ListDevices();

pcap_t* LiveCapture(const char* pszDevice); 

}

#endif
