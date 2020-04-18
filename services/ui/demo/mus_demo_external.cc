// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/ui/demo/mus_demo_external.h"

#include "base/command_line.h"
#include "base/strings/string_number_conversions.h"
#include "services/service_manager/public/cpp/service_context.h"
#include "services/ui/demo/window_tree_data.h"
#include "services/ui/public/interfaces/constants.mojom.h"
#include "services/ui/public/interfaces/window_tree_host.mojom.h"
#include "ui/aura/mus/window_tree_client.h"
#include "ui/aura/mus/window_tree_host_mus.h"
#include "ui/display/display.h"

namespace ui {
namespace demo {

namespace {

class WindowTreeDataExternal : public WindowTreeData {
 public:
  // Creates a new window tree host associated to the WindowTreeData.
  WindowTreeDataExternal(aura::WindowTreeClient* window_tree_client,
                         int square_size)
      : WindowTreeData(square_size) {}

  DISALLOW_COPY_AND_ASSIGN(WindowTreeDataExternal);
};

int GetSquareSizeForWindow(int window_index) {
  return 50 * window_index + 400;
}

}  // namespace

MusDemoExternal::MusDemoExternal() {}

MusDemoExternal::~MusDemoExternal() {}

std::unique_ptr<aura::WindowTreeClient>
MusDemoExternal::CreateWindowTreeClient() {
  return aura::WindowTreeClient::CreateForWindowTreeHostFactory(
      context()->connector(), this);
}

void MusDemoExternal::OnStartImpl() {
  const base::CommandLine* command_line =
      base::CommandLine::ForCurrentProcess();
  if (command_line->HasSwitch("external-window-count")) {
    if (!base::StringToSizeT(
            command_line->GetSwitchValueASCII("external-window-count"),
            &number_of_windows_)) {
      LOG(FATAL) << "Invalid value for \'external-window-count\'";
      return;
    }
  }

  // TODO(tonikitoo,fwang): Implement management of displays in external mode.
  // For now, a fake display is created in order to work around an assertion in
  // aura::GetDeviceScaleFactorFromDisplay().
  AddPrimaryDisplay(display::Display(0));
}

void MusDemoExternal::OpenNewWindow() {
  // TODO(tonikitoo,msisov): Extend the WindowTreeClient API to allow creation
  // of various window tree host. Currently is only works once, and is
  // incorrect when kNumberOfWindows > 1.
  AppendWindowTreeData(std::make_unique<WindowTreeDataExternal>(
      window_tree_client(),
      GetSquareSizeForWindow(initialized_windows_count_)));
}

void MusDemoExternal::OnEmbed(
    std::unique_ptr<aura::WindowTreeHostMus> window_tree_host) {
  if (initialized_windows_count_ == 0) {
    // The initial connection to the WindowService has been established.
    OpenNewWindow();
  }

  InitWindowTreeData(std::move(window_tree_host));
  initialized_windows_count_++;

  // Open the next window until the requested number of windows is reached.
  if (initialized_windows_count_ < number_of_windows_)
    OpenNewWindow();
}

void MusDemoExternal::OnEmbedRootDestroyed(
    aura::WindowTreeHostMus* window_tree_host) {
  RemoveWindowTreeData(window_tree_host);
}

}  // namespace demo
}  // namespace ui
