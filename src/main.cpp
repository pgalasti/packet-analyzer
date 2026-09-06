#include "stdftxui.h"
#include "Defines.h"

#include "core/Device.h"
#include "ui/DeviceSelect.h"

#include <arpa/inet.h>
#include <netinet/in.h>

#include <iostream>
#include <vector>
#include <ranges>
#include <algorithm>
#include <cstdint>
#include <cstring>

using namespace ftxui;



// Will move this stuff to core while i test it
constexpr int linkHeaderLen {14};
constexpr uint16_t IPv4_Tag  {0x0800};
constexpr uint16_t IPv6_Tag  {0x86DD};
constexpr uint16_t VLAN_Tag1 {0x8100};
constexpr uint16_t VLAN_Tag2 {0x88A8};

void hex_dump(const u_char* data, size_t len) {
    for (size_t i = 0; i < len; i += 16) {
        printf("%04zx  ", i);

        for (size_t j = 0; j < 16; ++j) {
            if (i + j < len) printf("%02x ", data[i + j]);
            else             printf("   ");
            if (j == 7) putchar(' ');          // gap after 8 bytes
        }

        printf(" |");
        for (size_t j = 0; j < 16 && i + j < len; ++j) {
            unsigned char c = data[i + j];
            putchar(std::isprint(c) ? c : '.');
        }
        printf("|\n");
    }
}

void packetCallback(u_char* args, const struct pcap_pkthdr* pHeader, const u_char* packet) {
  std::cout << "Packet length grabbed: " << pHeader->len << std::endl;

  if(pHeader->caplen < linkHeaderLen+1) {
    return;
  }

  uint16_t etherType;
  std::memcpy(&etherType, packet+12, sizeof(etherType));
  etherType = ntohs(etherType);

  size_t offset {linkHeaderLen};
  while(etherType == VLAN_Tag1 || etherType == VLAN_Tag2) {
    if(pHeader->caplen < offset + 4) return;
    memcpy(&etherType, packet + offset + 2, sizeof(etherType));
    etherType = ntohs(etherType);
    offset += 4;
  }
 
  char szBuf [INET6_ADDRSTRLEN];

  if(etherType == IPv4_Tag) {
    if(pHeader->caplen < offset+20) return;
    in_addr src;
    memcpy(&src, packet+offset+12, sizeof(src));
    inet_ntop(AF_INET, &src, szBuf, sizeof(szBuf));

    auto ip{packet+linkHeaderLen};
    if(ip[9] == 6) {
      std::cout << "TCP" << std::endl;
    } else if(ip[9] == 17) {
      std::cout << "UDP" << std::endl;
    } else if(ip[9] == 1) {
      std::cout << "ICMP" << std::endl;
    }

  } else if(etherType == IPv6_Tag) {
    if(pHeader->caplen < offset +40) return;
    in6_addr src;
    memcpy(&src, packet+offset+8, sizeof(src));
    inet_ntop(AF_INET6, &src, szBuf, sizeof(szBuf));
  } else {
    std::cout << "Unknown ether type" << std::endl;
    return;
  }

  std::cout << szBuf << std::endl;
  hex_dump(packet, pHeader->caplen);

}

int main([[maybe_unused]]int argc, [[maybe_unused]]char* argv[]) {

  FILE_TRACE_LOG("Starting up...");

  const auto devices {PA::Core::ListDevices()};
  const PA::UI::ActiveDeviceSelects deviceSelections { devices 
    | std::views::transform([](const PA::Core::Device& device) { 
        return PA::UI::ActiveDeviceSelect{device.DeviceName, device.Description}; 
      })
    | std::ranges::to<PA::UI::ActiveDeviceSelects>() };
  FILE_TRACE_LOG("Devices Fetched:");
  FILE_TRACE_LOG(devices.size());

  PA::UI::DeviceSelectScreen deviceSelectScreen(deviceSelections);
  deviceSelectScreen.Render();
  
  auto selection {deviceSelectScreen.GetResult()};
  if(!selection) {
    std::cout << "No device selected.\n";
    return 1;
  }

  std::cout << "Capturing on: " << selection->DeviceName << '\n';

  auto pHandle {PA::Core::LiveCapture(selection->DeviceName.c_str())};
  pcap_loop(pHandle, 0, packetCallback, nullptr);

  pcap_close(pHandle);

  return 0;
}

