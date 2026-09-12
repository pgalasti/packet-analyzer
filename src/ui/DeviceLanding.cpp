#include "ui/DeviceLanding.h"

#include "stdftxui.h"
#include "Defines.h"

#include <algorithm>
#include <format>
#include <string>
#include <vector>

using namespace PA::UI;
using namespace ftxui;

namespace {

constexpr int LIST_PANE_HEIGHT {12};
constexpr int DEVICE_NAME_WIDTH {28};

// Dummy code until I implement pcap
PA::Core::PacketRecord MakeDummyPacket(std::uint64_t index, PA::Core::Direction direction) {
  static const std::vector<std::string> protocols {"TCP", "UDP", "ICMP", "ARP", "TCP", "UDP"};
  static const std::vector<std::string> locals {
    "192.168.1.24:52344", "192.168.1.24:443", "192.168.1.24:8080", "192.168.1.24:22"};
  static const std::vector<std::string> remotes {
    "142.250.72.14:443", "10.0.0.5:53", "93.184.216.34:80", "172.217.12.238:443"};

  const auto slot {static_cast<std::size_t>(index)};
  const bool inbound {direction == PA::Core::Direction::Received};

  PA::Core::PacketRecord record;
  record.Index = index;
  record.Timestamp = 1757700000.0 + static_cast<double>(index) * 0.137;
  record.Protocol = protocols.at(slot % protocols.size());
  record.Source = inbound ? remotes.at(slot % remotes.size()) : locals.at(slot % locals.size());
  record.Destination = inbound ? locals.at(slot % locals.size()) : remotes.at(slot % remotes.size());
  record.WireLength = 64uz + (slot * 37uz) % 1450uz;

  const std::vector<std::uint8_t> preamble {
    0x00, 0x1a, 0x2b, 0x3c, 0x4d, 0x5e, 0xa4, 0x83, 0xe7, 0x11, 0x22, 0x33, 0x08, 0x00,
    0x45, 0x00, 0x00, 0x3c, 0x1c, 0x46, 0x40, 0x00, 0x40, 0x06, 0xb1, 0xe6,
    0xc0, 0xa8, 0x01, 0x18, 0x8e, 0xfa, 0x48, 0x0e};
  record.Bytes = preamble;

  const std::string payload {
    std::format("GET /index.html?seq={} HTTP/1.1\r\nHost: example.com\r\n"
                "User-Agent: packet-analyzer/0.1\r\nAccept: */*\r\n\r\n", index)};
  record.Bytes.insert(record.Bytes.end(), payload.begin(), payload.end());

  for(std::size_t filler {0uz}; filler < (slot % 5uz) * 16uz; ++filler) {
    record.Bytes.push_back(static_cast<std::uint8_t>((filler * 17uz + slot) & 0xffuz));
  }

  return record;
}

std::string FormatLabel(const PA::Core::PacketRecord& record) {
  return std::format("{:>5}  {:<4}  {:<21} -> {:<21} {:>5}B",
    record.Index, record.Protocol, record.Source, record.Destination, record.WireLength);
}

MenuOption PacketMenuOption(std::vector<std::string>* pEntries, int* pSelected) {
  auto option {MenuOption::Vertical()};
  option.entries = pEntries;
  option.selected = pSelected;
  option.entries_option.transform = [](const EntryState& state) {
    auto row {hbox({text(state.active ? ">" : " "), text(state.label) | flex})};
    if(state.active && state.focused) {
      return row | bold | color(Color::Black) | bgcolor(Color::Green);
    }
    if(state.active) {
      return row | bold | color(Color::GreenLight);
    }
    return row | color(Color::GrayLight);
  };
  return option;
}

Element DumpPane(const std::vector<std::string>& lines, Color tone) {
  if(lines.empty()) {
    return text(" (no packet selected) ") | color(Color::GrayDark) | center | flex;
  }

  Elements rows;
  rows.reserve(lines.size());
  for(const auto& line : lines) {
    rows.push_back(text(line) | color(tone));
  }
  return vbox(std::move(rows)) | vscroll_indicator | yframe | flex;
}

}

void DeviceLandingScreen::Init() {
  FILE_TRACE_LOG("DeviceLanding:: Init for device " << m_DeviceName);

  const std::lock_guard lock {m_Mutex};

  for(std::uint64_t index {1u}; index <= 60u; ++index) {
    auto received {MakeDummyPacket(index, PA::Core::Direction::Received)};
    m_ReceivedBytes += received.WireLength;
    ++m_ReceivedCount;
    m_Received.Push(std::move(received));

    auto sent {MakeDummyPacket(index, PA::Core::Direction::Sent)};
    m_SentBytes += sent.WireLength;
    ++m_SentCount;
    m_Sent.Push(std::move(sent));
  }

  SyncLabels();
}

void DeviceLandingScreen::Cleanup() {
  FILE_TRACE_LOG("DeviceLanding:: Cleanup.");
  const std::lock_guard lock {m_Mutex};
  m_pScreen = nullptr;
}

bool DeviceLandingScreen::AnalyzeEnabled() const {
  const std::lock_guard lock {m_Mutex};
  return m_Analyze;
}

void DeviceLandingScreen::Record(PA::Core::Direction direction, PA::Core::PacketRecord record) {
  ScreenInteractive* pScreen {nullptr};

  {
    const std::lock_guard lock {m_Mutex};

    if(direction == PA::Core::Direction::Received) {
      m_ReceivedBytes += record.WireLength;
      ++m_ReceivedCount;
      m_Received.Push(std::move(record));
    } else {
      m_SentBytes += record.WireLength;
      ++m_SentCount;
      m_Sent.Push(std::move(record));
    }

    SyncLabels();
    pScreen = m_pScreen;
  }

  if(pScreen != nullptr) {
    pScreen->PostEvent(Event::Custom);
  }
}

void DeviceLandingScreen::SyncLabels() {
  m_ReceivedLabels.clear();
  m_ReceivedLabels.reserve(m_Received.Size());
  for(const auto& record : m_Received) {
    m_ReceivedLabels.push_back(FormatLabel(record));
  }

  m_SentLabels.clear();
  m_SentLabels.reserve(m_Sent.Size());
  for(const auto& record : m_Sent) {
    m_SentLabels.push_back(FormatLabel(record));
  }

  m_ReceivedSelected = std::clamp(m_ReceivedSelected, 0,
    std::max(0, static_cast<int>(m_ReceivedLabels.size()) - 1));
  m_SentSelected = std::clamp(m_SentSelected, 0,
    std::max(0, static_cast<int>(m_SentLabels.size()) - 1));
}

void DeviceLandingScreen::Render() {
  FILE_TRACE_LOG("DeviceLanding:: Rendering landing screen.");

  auto screen {ScreenInteractive::Fullscreen()};
  {
    const std::lock_guard lock {m_Mutex};
    m_pScreen = &screen;
  }

  auto analyzeCheckbox {Checkbox("Analyze", &m_Analyze)};
  auto receivedMenu {Menu(PacketMenuOption(&m_ReceivedLabels, &m_ReceivedSelected))};
  auto sentMenu {Menu(PacketMenuOption(&m_SentLabels, &m_SentSelected))};

  auto lists {Container::Horizontal({receivedMenu, sentMenu})};
  auto layout {Container::Vertical({analyzeCheckbox, lists})};

  auto renderer {Renderer(layout, [&] {
    const std::lock_guard lock {m_Mutex};

    const bool sentFocused {sentMenu->Focused()};
    const auto& history {sentFocused ? m_Sent : m_Received};
    const auto selected {static_cast<std::size_t>(sentFocused ? m_SentSelected : m_ReceivedSelected)};
    const bool hasSelection {selected < history.Size()};

    std::string protocol {"--"};
    std::string sizeText {"--"};
    std::vector<std::string> hexLines;
    std::vector<std::string> charLines;

    if(hasSelection) {
      const auto& record {history.At(selected)};
      protocol = record.Protocol;
      sizeText = std::format("{} bytes ({} captured)", record.WireLength, record.Bytes.size());
      hexLines = PA::Core::HexDumpLines(record.Bytes);
      charLines = PA::Core::CharacterDumpLines(record.Bytes);
    }

    auto header {hbox({
      analyzeCheckbox->Render() | flex,
      separator(),
      text(m_DeviceName) | bold | color(Color::CyanLight)
        | size(WIDTH, EQUAL, DEVICE_NAME_WIDTH) | center,
    })};

    auto totals {hbox({
      text(std::format(" Total Packets Received/Sent: {}/{} ", m_ReceivedCount, m_SentCount))
        | center | flex,
      separator(),
      text(std::format(" Total Packets Received/Sent (bytes): {}/{} ", m_ReceivedBytes, m_SentBytes))
        | center | flex,
    })};

    auto detailHeader {hbox({
      text(std::format(" Protocol Type: {} ", protocol)) | center | flex,
      separator(),
      text(std::format(" Size of Packet: {} ", sizeText)) | center | flex,
    }) | color(Color::CyanLight)};

    auto detail {hbox({
      vbox({
        detailHeader,
        separator(),
        DumpPane(hexLines, Color::GreenLight),
      }) | flex,
      separator(),
      vbox({
        text(" Character Dump ") | bold | center | color(Color::CyanLight),
        separator(),
        DumpPane(charLines, Color::YellowLight),
      }) | flex,
    }) | flex};

    auto listPanes {hbox({
      vbox({
        text(std::format(" Received ({}/{} held) ", m_ReceivedLabels.size(), m_Received.Capacity()))
          | bold | color(sentFocused ? Color::GrayDark : Color::GreenLight),
        separator(),
        receivedMenu->Render() | vscroll_indicator | yframe | flex,
      }) | flex,
      separator(),
      vbox({
        text(std::format(" Sent ({}/{} held) ", m_SentLabels.size(), m_Sent.Capacity()))
          | bold | color(sentFocused ? Color::GreenLight : Color::GrayDark),
        separator(),
        sentMenu->Render() | vscroll_indicator | yframe | flex,
      }) | flex,
    }) | size(HEIGHT, EQUAL, LIST_PANE_HEIGHT)};

    return vbox({
      header,
      separator(),
      totals,
      separator(),
      detail,
      separator(),
      listPanes,
      separator(),
      hbox({
        text(" tab ") | bold, text("switch pane   "),
        text("up/down ") | bold, text("select packet   "),
        text("space ") | bold, text("toggle analyze   "),
        text("q ") | bold, text("back"),
      }) | color(Color::GrayDark),
    }) | border | color(Color::Green);
  })};

  renderer |= CatchEvent([&](Event event) {
    if(event == Event::Character('q') || event == Event::Escape) {
      FILE_TRACE_LOG("DeviceLanding:: Quitting.");
      m_Result = m_DeviceName;
      screen.Exit();
      return true;
    }
    return false;
  });

  screen.Clear();
  screen.Loop(renderer);

  Cleanup();
}
