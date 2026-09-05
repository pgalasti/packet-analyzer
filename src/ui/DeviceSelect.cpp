#include "ui/DeviceSelect.h"

#include "stdftxui.h"
#include "Defines.h"

#include <vector>
#include <string>
#include <unordered_map>

using namespace PA::UI;
using namespace ftxui;

constexpr int NAME_COLUMN_WIDTH {24};

void DeviceSelectScreen::Render() {
  FILE_TRACE_LOG("DeviceSelect:: Rendering Device Select Screen.");

  if(m_ActiveDeviceSelects.empty()) {
    m_SelectedDevice = std::nullopt;
    FILE_TRACE_LOG("DeviceSelect:: Device selection is empty!");
    return;
  }

  std::vector<std::string> deviceNames;
  std::unordered_map<std::string, std::string> descriptions;
  deviceNames.reserve(m_ActiveDeviceSelects.size());
  for(const auto& device : m_ActiveDeviceSelects) {
    deviceNames.push_back(device.DeviceName);
    descriptions.emplace(device.DeviceName, device.Description);
  }

  int selected {0};
  bool confirmed {false};

  auto screen {ScreenInteractive::Fullscreen()};

  auto menuOption {MenuOption::Vertical()};
  menuOption.entries = &deviceNames;
  menuOption.selected = &selected;
  menuOption.on_enter = [&] {
    confirmed = true;
    screen.Exit();
  };
  menuOption.entries_option.transform = [&descriptions](const EntryState& state) {
    auto row = hbox({
      text(state.active ? " > " : "   "),
      text(state.label) | size(WIDTH, EQUAL, NAME_COLUMN_WIDTH),
      text(descriptions.at(state.label)) | flex,
    });

    if(state.active) {
      return row | bold | color(Color::Black) | bgcolor(Color::Green);
    }
    return row | color(Color::GrayLight);
  };

  auto menu = Menu(menuOption);

  auto renderer = Renderer(menu, [&] {
    return vbox({
      text(" Select a capture interface ") | bold | color(Color::Green),
      separator(),
      hbox({
        text("   "),
        text("INTERFACE") | size(WIDTH, EQUAL, NAME_COLUMN_WIDTH),
        text("DESCRIPTION") | flex,
      }) | bold | color(Color::CyanLight),
      separator(),
      menu->Render() | vscroll_indicator | yframe | flex,
      separator(),
      hbox({
        text(" up/down ") | bold, text("move   "),
        text("enter ") | bold, text("select   "),
        text("q ") | bold, text("quit"),
      }) | color(Color::GrayDark),
    }) | border | color(Color::Green);
  });

  renderer |= CatchEvent([&](Event event) {
    if(event == Event::Character('q') || event == Event::Escape) {
      screen.Exit();
      FILE_TRACE_LOG("DeviceSelect:: Quitting.");
      return true;
    }
    return false;
  });

  screen.Clear();
  screen.Loop(renderer);

  if(!confirmed) {
    FILE_TRACE_LOG("DeviceSelect::Not confirmed.");
    m_SelectedDevice = std::nullopt;
    return;
  }
  m_SelectedDevice = m_ActiveDeviceSelects.at(static_cast<std::size_t>(selected));
  FILE_TRACE_LOG("DeviceSelect:: Selection Complete.");
}
