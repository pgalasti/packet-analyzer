#ifndef PA_PACKET_H
#define PA_PACKET_H

#include <cstddef>
#include <cstdint>
#include <deque>
#include <span>
#include <string>
#include <vector>

namespace PA::Core {

constexpr std::size_t PACKET_HISTORY_LIMIT {1000uz};

constexpr std::size_t HEX_DUMP_STRIDE {16uz};

enum class Direction { Received, Sent };

struct PacketRecord {
  std::uint64_t Index {0u};
  double Timestamp {0.0};
  std::string Protocol {"UNKNOWN"};
  std::string Source;
  std::string Destination;
  std::size_t WireLength {0uz};
  std::vector<std::uint8_t> Bytes;
};

class PacketHistory {
public:
  explicit PacketHistory(std::size_t capacity = PACKET_HISTORY_LIMIT)
    : m_Capacity{capacity == 0uz ? 1uz : capacity} {}

  void Push(PacketRecord record) {
    while(m_Records.size() >= m_Capacity) {
      m_Records.pop_front();
    }
    m_Records.push_back(std::move(record));
  }

  void Clear() { m_Records.clear(); }

  std::size_t Capacity() const noexcept { return m_Capacity; }
  std::size_t Size() const noexcept { return m_Records.size(); }
  bool Empty() const noexcept { return m_Records.empty(); }

  const PacketRecord& At(std::size_t index) const { return m_Records.at(index); }

  auto begin() const noexcept { return m_Records.begin(); }
  auto end() const noexcept { return m_Records.end(); }

private:
  std::size_t m_Capacity;
  std::deque<PacketRecord> m_Records;
};

std::vector<std::string> HexDumpLines(std::span<const std::uint8_t> bytes);

std::vector<std::string> CharacterDumpLines(std::span<const std::uint8_t> bytes);

}

#endif
