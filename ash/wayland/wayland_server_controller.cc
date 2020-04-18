// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/wayland/wayland_server_controller.h"

#include <memory>

#include "ash/public/cpp/ash_switches.h"
#include "ash/public/cpp/config.h"
#include "ash/system/message_center/arc/arc_notification_surface_manager_impl.h"
#include "base/command_line.h"
#include "base/memory/ptr_util.h"
#include "base/message_loop/message_loop_current.h"
#include "components/exo/display.h"
#include "components/exo/file_helper.h"
#include "components/exo/wayland/server.h"
#include "components/exo/wm_helper.h"

namespace ash {

class WaylandServerController::WaylandWatcher
    : public base::MessagePumpLibevent::FdWatcher {
 public:
  explicit WaylandWatcher(exo::wayland::Server* server)
      : controller_(FROM_HERE), server_(server) {
    base::MessageLoopCurrentForUI::Get()->WatchFileDescriptor(
        server_->GetFileDescriptor(),
        true,  // persistent
        base::MessagePumpLibevent::WATCH_READ, &controller_, this);
  }

  // base::MessagePumpLibevent::FdWatcher:
  void OnFileCanReadWithoutBlocking(int fd) override {
    server_->Dispatch(base::TimeDelta());
    server_->Flush();
  }
  void OnFileCanWriteWithoutBlocking(int fd) override { NOTREACHED(); }

 private:
  base::MessagePumpLibevent::FdWatchController controller_;
  exo::wayland::Server* const server_;

  DISALLOW_COPY_AND_ASSIGN(WaylandWatcher);
};

// static
std::unique_ptr<WaylandServerController>
WaylandServerController::CreateIfNecessary(
    std::unique_ptr<exo::FileHelper> file_helper) {
  if (!base::CommandLine::ForCurrentProcess()->HasSwitch(
          switches::kAshEnableWaylandServer)) {
    return nullptr;
  }

  return base::WrapUnique(new WaylandServerController(std::move(file_helper)));
}

WaylandServerController::~WaylandServerController() {
  wayland_watcher_.reset();
  wayland_server_.reset();
  display_.reset();
  exo::WMHelper::SetInstance(nullptr);
  wm_helper_.reset();
}

WaylandServerController::WaylandServerController(
    std::unique_ptr<exo::FileHelper> file_helper) {
  arc_notification_surface_manager_ =
      std::make_unique<ArcNotificationSurfaceManagerImpl>();
  wm_helper_ = std::make_unique<exo::WMHelper>();
  exo::WMHelper::SetInstance(wm_helper_.get());
  display_ = std::make_unique<exo::Display>(
      arc_notification_surface_manager_.get(), std::move(file_helper));
  wayland_server_ = exo::wayland::Server::Create(display_.get());
  // Wayland server creation can fail if XDG_RUNTIME_DIR is not set correctly.
  if (wayland_server_)
    wayland_watcher_ = std::make_unique<WaylandWatcher>(wayland_server_.get());
}

}  // namespace ash
