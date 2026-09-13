#ifndef PA_DEVICE_H
#define PA_DEVICE_H

#include <pcap.h>

#include <string>
#include <vector>
#include <stdexcept>

namespace PA::Core {

constexpr int LOOKUP_OP_FAILURE {-1};
constexpr size_t PA_PCAP_BUF_SIZE {128};

struct Device {
  std::string DeviceName;
  std::string Description;
  std::string AddressStr;
};
using Devices = std::vector<Device>;

Devices ListDevices();

}

#endif
