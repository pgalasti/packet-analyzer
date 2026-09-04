#include <iostream>

#include <pcap.h>

int main(int argc, char* argv[]) {

  char szErrorBuffer[PCAP_ERRBUF_SIZE];
  char* pDevice { pcap_lookupdev(szErrorBuffer) };

  if(pDevice == nullptr) {
    std::cerr << "Error finding device: " << szErrorBuffer << std::endl;
    return EXIT_FAILURE;
  }

  std::cout << "Network device detected: " << pDevice << std::endl;


  return 0;

}
