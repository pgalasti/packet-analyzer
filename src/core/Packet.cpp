#include "core/Packet.h"

#include <cctype>
#include <format>

namespace PA::Core {

std::vector<std::string> HexDumpLines(std::span<const std::uint8_t> bytes) {
  std::vector<std::string> lines;
  lines.reserve((bytes.size() / HEX_DUMP_STRIDE) + 1uz);

  for(std::size_t offset {0uz}; offset < bytes.size(); offset += HEX_DUMP_STRIDE) {
    std::string line {std::format("{:04x}  ", offset)};

    for(std::size_t column {0uz}; column < HEX_DUMP_STRIDE; ++column) {
      if(offset + column < bytes.size()) {
        line += std::format("{:02x} ", bytes[offset + column]);
      } else {
        line += "   ";
      }

      if(column == (HEX_DUMP_STRIDE / 2uz) - 1uz) {
        line += ' ';
      }
    }

    lines.push_back(std::move(line));
  }

  return lines;
}

std::vector<std::string> CharacterDumpLines(std::span<const std::uint8_t> bytes) {
  std::vector<std::string> lines;
  lines.reserve((bytes.size() / HEX_DUMP_STRIDE) + 1uz);

  for(std::size_t offset {0uz}; offset < bytes.size(); offset += HEX_DUMP_STRIDE) {
    std::string line {std::format("{:04x}  ", offset)};

    for(std::size_t column {0uz}; column < HEX_DUMP_STRIDE && offset + column < bytes.size(); ++column) {
      const auto byte {bytes[offset + column]};
      line += std::isprint(static_cast<unsigned char>(byte)) ? static_cast<char>(byte) : '.';
    }

    lines.push_back(std::move(line));
  }

  return lines;
}

}
