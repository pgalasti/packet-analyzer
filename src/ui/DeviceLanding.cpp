#include "ui/DeviceLanding.h"

#include "stdftxui.h"
#include "Defines.h"

#include <algorithm>
#include <chrono>
#include <deque>
#include <format>
#include <string>
#include <thread>
#include <utility>
#include <vector>

using namespace PA::UI;
using namespace ftxui;

namespace {

constexpr int LIST_PANE_HEIGHT {12};
constexpr int DEVICE_NAME_WIDTH {28};

constexpr auto REFRESH_INTERVAL {std::chrono::milliseconds{100}};

std::string FormatLabel(const PA::Core::PacketRecord& record) {
  return std::format("{:>5}  {:<6}  {:<21} -> {:<21} {:>5}B",
    record.Index, record.Protocol, record.Source, record.Destination, record.WireLength);
}

void AbsorbRecords(PA::Core::PacketHistory& history, std::deque<PA::Core::PacketRecord> records,
                   int& selected, int& focused) {
  if(records.empty()) {
    return;
  }

  const auto previousSize {static_cast<int>(history.Size())};
  const bool following {selected >= previousSize - 1};

  std::size_t evicted {0uz};
  for(auto& record : records) {
    if(history.Size() == history.Capacity()) {
      ++evicted;
    }
    history.Push(std::move(record));
  }

  const auto lastIndex {static_cast<int>(history.Size()) - 1};
  selected = following ? lastIndex : std::clamp(selected - static_cast<int>(evicted), 0, lastIndex);
  focused = selected;
}

MenuOption PacketMenuOption(std::vector<std::string>* pEntries, int* pSelected, int* pFocused) {
  auto option {MenuOption::Vertical()};
  option.entries = pEntries;
  option.selected = pSelected;
  option.focused_entry = pFocused;
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
  SyncLabels();
}

void DeviceLandingScreen::Cleanup() {
  FILE_TRACE_LOG("DeviceLanding:: Cleanup.");
}

bool DeviceLandingScreen::AnalyzeEnabled() const {
  const std::lock_guard lock {m_Mutex};
  return m_Analyze;
}

void DeviceLandingScreen::Record(PA::Core::Direction direction, PA::Core::PacketRecord record) {
  const std::lock_guard lock {m_Mutex};
  m_Dirty = true;

  if(direction == PA::Core::Direction::Received) {
    m_Totals.ReceivedBytes += record.WireLength;
    ++m_Totals.ReceivedCount;
    if(m_Analyze) {
      m_PendingReceived.Push(std::move(record));
    }
  } else {
    m_Totals.SentBytes += record.WireLength;
    ++m_Totals.SentCount;
    if(m_Analyze) {
      m_PendingSent.Push(std::move(record));
    }
  }
}

bool DeviceLandingScreen::TakeDirty() {
  const std::lock_guard lock {m_Mutex};
  return std::exchange(m_Dirty, false);
}

void DeviceLandingScreen::DrainPending() {
  std::deque<PA::Core::PacketRecord> received;
  std::deque<PA::Core::PacketRecord> sent;
  {
    const std::lock_guard lock {m_Mutex};
    received = m_PendingReceived.Release();
    sent = m_PendingSent.Release();
    m_ShownTotals = m_Totals;
  }

  AbsorbRecords(m_Received, std::move(received), m_ReceivedSelected, m_ReceivedFocused);
  AbsorbRecords(m_Sent, std::move(sent), m_SentSelected, m_SentFocused);
  SyncLabels();
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
}

void DeviceLandingScreen::Render() {
  FILE_TRACE_LOG("DeviceLanding:: Rendering landing screen.");

  auto screen {ScreenInteractive::Fullscreen()};

  auto checkboxOption {CheckboxOption::Simple()};
  checkboxOption.on_change = [this] {
    const std::lock_guard lock {m_Mutex};
    m_Analyze = m_AnalyzeChecked;
  };
  auto analyzeCheckbox {Checkbox("Analyze", &m_AnalyzeChecked, checkboxOption)};
  auto receivedMenu {Menu(PacketMenuOption(&m_ReceivedLabels, &m_ReceivedSelected, &m_ReceivedFocused))};
  auto sentMenu {Menu(PacketMenuOption(&m_SentLabels, &m_SentSelected, &m_SentFocused))};

  auto lists {Container::Horizontal({receivedMenu, sentMenu})};
  auto layout {Container::Vertical({analyzeCheckbox, lists})};

  auto renderer {Renderer(layout, [&] {
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
      text(std::format(" Total Packets Received/Sent: {}/{} ",
        m_ShownTotals.ReceivedCount, m_ShownTotals.SentCount))
        | center | flex,
      separator(),
      text(std::format(" Total Packets Received/Sent (bytes): {}/{} ",
        m_ShownTotals.ReceivedBytes, m_ShownTotals.SentBytes))
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
    if(event == Event::Custom) {
      DrainPending();
      return true;
    }
    if(event == Event::Character('q') || event == Event::Escape) {
      FILE_TRACE_LOG("DeviceLanding:: Quitting.");
      m_Result = m_DeviceName;
      screen.Exit();
      return true;
    }
    return false;
  });

  std::jthread refresher {[this, &screen](std::stop_token stopToken) {
    while(!stopToken.stop_requested()) {
      std::this_thread::sleep_for(REFRESH_INTERVAL);
      if(TakeDirty()) {
        screen.PostEvent(Event::Custom);
      }
    }
  }};

  screen.Clear();
  screen.Loop(renderer);

  Cleanup();
}
