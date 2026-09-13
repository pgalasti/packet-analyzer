#ifndef PA_CAPTURE_H
#define PA_CAPTURE_H

#include "core/Packet.h"

#include <pcapplusplus/IpAddress.h>
#include <pcapplusplus/MacAddress.h>

#include <cstdint>
#include <functional>
#include <string>
#include <vector>

// Throw these in a namespace to avoid collision
namespace pcpp {
class Packet;
class PcapLiveDevice;
class RawPacket;
}

namespace PA::Core {

constexpr int CAPTURE_SNAPSHOT_LENGTH {65535};

using PacketHandler = std::function<void(Direction, PacketRecord)>;

class PacketCapture {
public:
  explicit PacketCapture(const std::string& deviceName);
  ~PacketCapture();

  PacketCapture(const PacketCapture&) = delete;
  PacketCapture& operator=(const PacketCapture&) = delete;

  void Start(PacketHandler handler);

  void Stop();

private:
  void OnPacket(pcpp::RawPacket* pRawPacket);
  Direction Classify(const pcpp::Packet& packet) const;
  bool IsLocalAddress(const pcpp::IPAddress& address) const;

  pcpp::PcapLiveDevice* m_pDevice {nullptr};
  pcpp::MacAddress m_MacAddress;
  std::vector<pcpp::IPAddress> m_LocalAddresses;

  PacketHandler m_Handler;
  std::uint64_t m_NextIndex {1u};
};

}

#endif
