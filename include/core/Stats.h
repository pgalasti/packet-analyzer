#ifndef PA_STATS_H
#define PA_STATS_H

#include <cstddef>
#include <string>

namespace PA::Core {

struct PacketStats {

  std::string Address {""};
  size_t Packets {0uz};

  PacketStats& operator+=(size_t packets) { 
    Packets += packets; 
    return *this; 
  }
  PacketStats& operator+=(const PacketStats& packet) { 
    Packets += packet.Packets; 
    return *this; 
  }
};

}

#endif
