// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/exo/wayland/clients/test/wayland_client_test.h"

#include <stdlib.h>

#include <memory>

#include "ash/public/cpp/config.h"
#include "ash/session/session_controller.h"
#include "ash/shell.h"
#include "ash/test/ash_test_environment.h"
#include "ash/test/ash_test_helper.h"
#include "ash/test/ash_test_views_delegate.h"
#include "base/command_line.h"
#include "base/files/scoped_temp_dir.h"
#include "base/message_loop/message_loop.h"
#include "base/message_loop/message_loop_current.h"
#include "base/synchronization/waitable_event.h"
#include "components/exo/display.h"
#include "components/exo/file_helper.h"
#include "components/exo/wayland/server.h"
#include "components/exo/wm_helper.h"
#include "ui/aura/window_tree_host.h"
#include "ui/events/gesture_detection/gesture_configuration.h"
#include "ui/wm/core/cursor_manager.h"
#include "ui/wm/core/wm_core_switches.h"

namespace exo {

class AshTestEnvironmentWayland : public ash::AshTestEnvironment {
 public:
  AshTestEnvironmentWayland() = default;
  ~AshTestEnvironmentWayland() override = default;

  // Overriden from ash::AshTestEnvironment:
  std::unique_ptr<ash::AshTestViewsDelegate> CreateViewsDelegate() override {
    return std::make_unique<ash::AshTestViewsDelegate>();
  }

 private:
  DISALLOW_COPY_AND_ASSIGN(AshTestEnvironmentWayland);
};

// The ui message loop for running the wayland server. If it is not provided, we
// will use external wayland server.
base::MessageLoop* ui_message_loop_ = nullptr;

class WaylandClientTest::WaylandWatcher
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

WaylandClientTest::WaylandClientTest() = default;

WaylandClientTest::~WaylandClientTest() = default;

// static
void WaylandClientTest::SetUIMessageLoop(base::MessageLoop* message_loop) {
  DCHECK_NE(!!message_loop, !!ui_message_loop_);
  ui_message_loop_ = message_loop;
}

void WaylandClientTest::SetUp() {
  if (!ui_message_loop_)
    return;

  DCHECK_NE(base::MessageLoopCurrent::Get(), ui_message_loop_);

  base::WaitableEvent event(base::WaitableEvent::ResetPolicy::MANUAL,
                            base::WaitableEvent::InitialState::NOT_SIGNALED);
  ui_message_loop_->task_runner()->PostTask(
      FROM_HERE, base::Bind(&WaylandClientTest::SetUpOnUIThread,
                            base::Unretained(this), &event));
  event.Wait();
}

void WaylandClientTest::TearDown() {
  if (!ui_message_loop_)
    return;

  DCHECK(ui_message_loop_);
  DCHECK_NE(base::MessageLoopCurrent::Get(), ui_message_loop_);

  base::WaitableEvent event(base::WaitableEvent::ResetPolicy::MANUAL,
                            base::WaitableEvent::InitialState::NOT_SIGNALED);
  ui_message_loop_->task_runner()->PostTask(
      FROM_HERE, base::Bind(&WaylandClientTest::TearDownOnUIThread,
                            base::Unretained(this), &event));
  event.Wait();
}

void WaylandClientTest::SetUpOnUIThread(base::WaitableEvent* event) {
  xdg_temp_dir_ = std::make_unique<base::ScopedTempDir>();
  ASSERT_TRUE(xdg_temp_dir_->CreateUniqueTempDir());
  setenv("XDG_RUNTIME_DIR", xdg_temp_dir_->GetPath().MaybeAsASCII().c_str(),
         1 /* overwrite */);

  base::CommandLine* command_line = base::CommandLine::ForCurrentProcess();
  // Disable window animation when running tests.
  command_line->AppendSwitch(wm::switches::kWindowAnimationsDisabled);

  ash_test_environment_ = std::make_unique<AshTestEnvironmentWayland>();
  ash_test_helper_ =
      std::make_unique<ash::AshTestHelper>(ash_test_environment_.get());

  ash_test_helper_->SetUp(false /* start_session */,
                          true /* provide_local_state */);
  ash::Shell::GetPrimaryRootWindow()->Show();
  ash::Shell::GetPrimaryRootWindow()->GetHost()->Show();
  ash::Shell::GetPrimaryRootWindow()->MoveCursorTo(gfx::Point(-1000, -1000));

  if (ash::Shell::GetAshConfig() == ash::Config::CLASSIC)
    ash::Shell::Get()->cursor_manager()->EnableMouseEvents();

  // Changing GestureConfiguration shouldn't make tests fail. These values
  // prevent unexpected events from being generated during tests. Such as
  // delayed events which create race conditions on slower tests.
  ui::GestureConfiguration* gesture_config =
      ui::GestureConfiguration::GetInstance();
  gesture_config->set_max_touch_down_duration_for_click_in_ms(800);
  gesture_config->set_long_press_time_in_ms(1000);
  gesture_config->set_max_touch_move_in_pixels_for_click(5);

  wm_helper_ = std::make_unique<WMHelper>();
  WMHelper::SetInstance(wm_helper_.get());
  display_ = std::make_unique<Display>(nullptr, nullptr);
  wayland_server_ = exo::wayland::Server::Create(display_.get());
  DCHECK(wayland_server_);
  wayland_watcher_ = std::make_unique<WaylandWatcher>(wayland_server_.get());
  event->Signal();
}

void WaylandClientTest::TearDownOnUIThread(base::WaitableEvent* event) {
  wayland_watcher_.reset();
  wayland_server_.reset();
  display_.reset();
  WMHelper::SetInstance(nullptr);
  wm_helper_.reset();

  ash::Shell::Get()->session_controller()->NotifyChromeTerminating();
  ash_test_helper_->TearDown();
  ash_test_helper_ = nullptr;
  ash_test_environment_ = nullptr;
  xdg_temp_dir_ = nullptr;
  event->Signal();
}

}  // namespace exo
