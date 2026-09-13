#include "core/Capture.h"

#include <pcapplusplus/ArpLayer.h>
#include <pcapplusplus/EthLayer.h>
#include <pcapplusplus/IPv4Layer.h>
#include <pcapplusplus/IPv6Layer.h>
#include <pcapplusplus/Logger.h>
#include <pcapplusplus/Packet.h>
#include <pcapplusplus/PcapLiveDevice.h>
#include <pcapplusplus/PcapLiveDeviceList.h>
#include <pcapplusplus/RawPacket.h>
#include <pcapplusplus/Sll2Layer.h>
#include <pcapplusplus/SllLayer.h>
#include <pcapplusplus/TcpLayer.h>
#include <pcapplusplus/UdpLayer.h>

#include <arpa/inet.h>

#include <algorithm>
#include <format>
#include <stdexcept>
#include <string_view>

namespace PA::Core {

namespace {

constexpr std::uint16_t LINUX_PACKET_OUTGOING {4u};

void SilenceLogs() {
  pcpp::Logger::getInstance().setLogPrinter(
    [](pcpp::LogLevel, const std::string&, const std::string&, const std::string&, const int) {});
}

std::string_view ProtocolName(pcpp::ProtocolType protocol) {
  switch(protocol) {
    case pcpp::Ethernet:
    case pcpp::EthernetDot3:         return "ETH";
    case pcpp::IPv4:                 return "IPv4";
    case pcpp::IPv6:                 return "IPv6";
    case pcpp::TCP:                  return "TCP";
    case pcpp::UDP:                  return "UDP";
    case pcpp::HTTPRequest:
    case pcpp::HTTPResponse:         return "HTTP";
    case pcpp::ARP:                  return "ARP";
    case pcpp::VLAN:                 return "VLAN";
    case pcpp::ICMP:                 return "ICMP";
    case pcpp::ICMPv6:               return "ICMPv6";
    case pcpp::PPPoESession:
    case pcpp::PPPoEDiscovery:       return "PPPoE";
    case pcpp::DNS:                  return "DNS";
    case pcpp::MPLS:                 return "MPLS";
    case pcpp::GREv0:
    case pcpp::GREv1:                return "GRE";
    case pcpp::SSL:                  return "TLS";
    case pcpp::SLL:
    case pcpp::SLL2:                 return "SLL";
    case pcpp::DHCP:                 return "DHCP";
    case pcpp::DHCPv6:               return "DHCPv6";
    case pcpp::NULL_LOOPBACK:        return "LOOP";
    case pcpp::IGMPv1:
    case pcpp::IGMPv2:
    case pcpp::IGMPv3:               return "IGMP";
    case pcpp::VXLAN:                return "VXLAN";
    case pcpp::SIPRequest:
    case pcpp::SIPResponse:          return "SIP";
    case pcpp::SDP:                  return "SDP";
    case pcpp::Radius:               return "RADIUS";
    case pcpp::GTPv1:
    case pcpp::GTPv2:                return "GTP";
    case pcpp::BGP:                  return "BGP";
    case pcpp::SSH:                  return "SSH";
    case pcpp::AuthenticationHeader: return "AH";
    case pcpp::ESP:                  return "ESP";
    case pcpp::NTP:                  return "NTP";
    case pcpp::Telnet:               return "TELNET";
    case pcpp::FTPControl:
    case pcpp::FTPData:              return "FTP";
    case pcpp::STP:                  return "STP";
    case pcpp::LLC:                  return "LLC";
    case pcpp::SomeIP:               return "SOMEIP";
    case pcpp::WakeOnLan:            return "WOL";
    case pcpp::NFLOG:                return "NFLOG";
    case pcpp::TPKT:                 return "TPKT";
    case pcpp::VRRPv2:
    case pcpp::VRRPv3:               return "VRRP";
    case pcpp::COTP:                 return "COTP";
    case pcpp::S7COMM:               return "S7COMM";
    case pcpp::SMTP:                 return "SMTP";
    case pcpp::LDAP:                 return "LDAP";
    case pcpp::WireGuard:            return "WG";
    case pcpp::CiscoHDLC:            return "HDLC";
    case pcpp::DOIP:                 return "DOIP";
    case pcpp::Modbus:               return "MODBUS";
    case pcpp::Postgres:             return "PGSQL";
    case pcpp::MySQL:                return "MYSQL";
    default:                         return {};
  }
}

std::string DescribeProtocol(const pcpp::Packet& packet) {
  for(auto pLayer {packet.getLastLayer()}; pLayer != nullptr; pLayer = pLayer->getPrevLayer()) {
    if(const auto name {ProtocolName(pLayer->getProtocol())}; !name.empty()) {
      return std::string{name};
    }
  }
  return "UNKNOWN";
}

std::string Endpoint(const std::string& address, bool isIPv6, std::uint16_t port) {
  return isIPv6 ? std::format("[{}]:{}", address, port) : std::format("{}:{}", address, port);
}

void DescribeEndpoints(const pcpp::Packet& packet, PacketRecord& record) {
  std::string sourceAddress;
  std::string destinationAddress;
  bool isIPv6 {false};

  if(const auto pIPv4 {packet.getLayerOfType<pcpp::IPv4Layer>()}) {
    sourceAddress = pIPv4->getSrcIPv4Address().toString();
    destinationAddress = pIPv4->getDstIPv4Address().toString();
  } else if(const auto pIPv6 {packet.getLayerOfType<pcpp::IPv6Layer>()}) {
    sourceAddress = pIPv6->getSrcIPv6Address().toString();
    destinationAddress = pIPv6->getDstIPv6Address().toString();
    isIPv6 = true;
  } else if(const auto pArp {packet.getLayerOfType<pcpp::ArpLayer>()}) {
    record.Source = pArp->getSenderIpAddr().toString();
    record.Destination = pArp->getTargetIpAddr().toString();
    return;
  } else if(const auto pEth {packet.getLayerOfType<pcpp::EthLayer>()}) {
    record.Source = pEth->getSourceMac().toString();
    record.Destination = pEth->getDestMac().toString();
    return;
  } else {
    record.Source = "--";
    record.Destination = "--";
    return;
  }

  if(const auto pTcp {packet.getLayerOfType<pcpp::TcpLayer>()}) {
    record.Source = Endpoint(sourceAddress, isIPv6, pTcp->getSrcPort());
    record.Destination = Endpoint(destinationAddress, isIPv6, pTcp->getDstPort());
  } else if(const auto pUdp {packet.getLayerOfType<pcpp::UdpLayer>()}) {
    record.Source = Endpoint(sourceAddress, isIPv6, pUdp->getSrcPort());
    record.Destination = Endpoint(destinationAddress, isIPv6, pUdp->getDstPort());
  } else {
    record.Source = std::move(sourceAddress);
    record.Destination = std::move(destinationAddress);
  }
}

}

PacketCapture::PacketCapture(const std::string& deviceName) {
  auto& logger {pcpp::Logger::getInstance()};

  std::string lastError;
  logger.setLogPrinter([&lastError](pcpp::LogLevel level, const std::string& message,
                                    const std::string&, const std::string&, const int) {
    if(level == pcpp::LogLevel::Error) {
      lastError = message;
    }
  });

  const auto& deviceList {pcpp::PcapLiveDeviceList::getInstance()};
  m_pDevice = deviceList.getDeviceByName(deviceName);

  const bool opened {m_pDevice != nullptr
    && m_pDevice->open(pcpp::PcapLiveDevice::DeviceConfiguration(
         pcpp::PcapLiveDevice::Promiscuous, 0, CAPTURE_SNAPSHOT_LENGTH))};
  SilenceLogs();

  if(m_pDevice == nullptr) {
    throw std::runtime_error("Unable to find device " + deviceName + " for live capture");
  }
  if(!opened) {
    throw std::runtime_error("Unable to open device " + deviceName + " for live capture"
      + (lastError.empty() ? std::string{} : ": " + lastError));
  }

  m_MacAddress = m_pDevice->getMacAddress();
  for(const auto pDevice : deviceList.getPcapLiveDevicesList()) {
    const auto addresses {pDevice->getIPAddresses()};
    m_LocalAddresses.insert(m_LocalAddresses.end(), addresses.begin(), addresses.end());
  }
}

PacketCapture::~PacketCapture() {
  Stop();
  if(m_pDevice != nullptr) {
    m_pDevice->close();
  }
}

void PacketCapture::Start(PacketHandler handler) {
  m_Handler = std::move(handler);

  const bool started {m_pDevice->startCapture(
    [this](pcpp::RawPacket* pRawPacket, pcpp::PcapLiveDevice*, void*) { OnPacket(pRawPacket); },
    nullptr)};
  if(!started) {
    throw std::runtime_error("Unable to start capture on device " + m_pDevice->getName());
  }
}

void PacketCapture::Stop() {
  if(m_pDevice != nullptr && m_pDevice->captureActive()) {
    m_pDevice->stopCapture();
  }
}

void PacketCapture::OnPacket(pcpp::RawPacket* pRawPacket) {
  const pcpp::Packet packet {pRawPacket, false};

  PacketRecord record;
  record.Index = m_NextIndex++;

  const auto timestamp {pRawPacket->getPacketTimeStamp()};
  record.Timestamp = static_cast<double>(timestamp.tv_sec) + static_cast<double>(timestamp.tv_nsec) / 1e9;

  record.Protocol = DescribeProtocol(packet);
  DescribeEndpoints(packet, record);

  record.WireLength = static_cast<std::size_t>(std::max(pRawPacket->getFrameLength(), 0));
  const auto pData {pRawPacket->getRawData()};
  record.Bytes.assign(pData, pData + std::max(pRawPacket->getRawDataLen(), 0));

  m_Handler(Classify(packet), std::move(record));
}

Direction PacketCapture::Classify(const pcpp::Packet& packet) const {
  if(const auto pSll {packet.getLayerOfType<pcpp::SllLayer>()}) {
    return ntohs(pSll->getSllHeader()->packet_type) == LINUX_PACKET_OUTGOING
      ? Direction::Sent : Direction::Received;
  }
  if(const auto pSll2 {packet.getLayerOfType<pcpp::Sll2Layer>()}) {
    return pSll2->getPacketType() == LINUX_PACKET_OUTGOING ? Direction::Sent : Direction::Received;
  }

  if(const auto pEth {packet.getLayerOfType<pcpp::EthLayer>()}; pEth && m_MacAddress != pcpp::MacAddress::Zero) {
    return pEth->getSourceMac() == m_MacAddress ? Direction::Sent : Direction::Received;
  }

  if(const auto pIPv4 {packet.getLayerOfType<pcpp::IPv4Layer>()}) {
    return IsLocalAddress(pIPv4->getSrcIPAddress()) ? Direction::Sent : Direction::Received;
  }
  if(const auto pIPv6 {packet.getLayerOfType<pcpp::IPv6Layer>()}) {
    return IsLocalAddress(pIPv6->getSrcIPAddress()) ? Direction::Sent : Direction::Received;
  }

  return Direction::Received;
}

bool PacketCapture::IsLocalAddress(const pcpp::IPAddress& address) const {
  return std::ranges::find(m_LocalAddresses, address) != m_LocalAddresses.end();
}

}
