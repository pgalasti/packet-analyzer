#ifndef PA_DEVICE_LANDING_H
#define PA_DEVICE_LANDING_H

#include "Screen.h"

#include "core/Packet.h"

#include <cstddef>
#include <mutex>
#include <string>
#include <vector>

namespace PA::UI {

using DeviceLandingBase = Screen<std::string, std::string>;

class DeviceLandingScreen : public DeviceLandingBase {
public:
  explicit DeviceLandingScreen(const std::string& deviceName)
    : DeviceLandingBase(deviceName), m_DeviceName{deviceName} {}
  ~DeviceLandingScreen() override = default;

  void Render() override;

  void Init() override;
  void Cleanup() override;

  std::string GetResult() override { return m_Result; }

  void Record(PA::Core::Direction direction, PA::Core::PacketRecord record);

  bool AnalyzeEnabled() const;

protected:
  struct Totals {
    std::size_t ReceivedCount {0uz};
    std::size_t SentCount {0uz};
    std::size_t ReceivedBytes {0uz};
    std::size_t SentBytes {0uz};
  };

  bool TakeDirty();
  void DrainPending();
  void SyncLabels();

  std::string m_DeviceName;
  std::string m_Result;

  mutable std::mutex m_Mutex;
  PA::Core::PacketHistory m_PendingReceived {PA::Core::PACKET_HISTORY_LIMIT};
  PA::Core::PacketHistory m_PendingSent {PA::Core::PACKET_HISTORY_LIMIT};
  Totals m_Totals;
  bool m_Analyze {true};
  bool m_Dirty {false};

  PA::Core::PacketHistory m_Received {PA::Core::PACKET_HISTORY_LIMIT};
  PA::Core::PacketHistory m_Sent {PA::Core::PACKET_HISTORY_LIMIT};
  Totals m_ShownTotals;

  std::vector<std::string> m_ReceivedLabels;
  std::vector<std::string> m_SentLabels;

  int m_ReceivedSelected {0};
  int m_ReceivedFocused {0};
  int m_SentSelected {0};
  int m_SentFocused {0};
  bool m_AnalyzeChecked {true};
};

}

#endif
