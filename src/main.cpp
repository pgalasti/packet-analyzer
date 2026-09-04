#include <iostream>

#include <pcap.h>

constexpr int LOOKUP_OP_FAILURE {-1};

int main(int argc, char* argv[]) {

  char szErrorBuffer[PCAP_ERRBUF_SIZE];
  char* pDevice { pcap_lookupdev(szErrorBuffer) };

  if(pDevice == nullptr) {
    std::cerr << "Error finding device: " << szErrorBuffer << std::endl;
    return EXIT_FAILURE;
  }

  std::cout << "Network device detected: " << pDevice << std::endl;

  bpf_u_int32 addressRaw;
  bpf_u_int32 mask;
  auto lookupOp {pcap_lookupnet(pDevice, &addressRaw, &mask, szErrorBuffer) };
  if(lookupOp == LOOKUP_OP_FAILURE) {
    std::cerr << "Error lookup address for device: " << szErrorBuffer << std::endl;
    return EXIT_FAILURE;
  }

  in_addr address;
  address.s_addr = addressRaw;
  char* strAdd {inet_ntoa(address)};
  if(strAdd == nullptr) {
    std::cerr << "Error fetching readable address inet_ntoa" << std::endl;
    return EXIT_FAILURE;
  }

  std::cout << "Address: " << strAdd << std::endl;
  
  return 0;

}
