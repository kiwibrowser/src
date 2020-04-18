// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <wayland-server-core.h>
#include <xdg-shell-unstable-v5-server-protocol.h>

#include "base/message_loop/message_loop.h"
#include "base/run_loop.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/display/types/display_snapshot.h"
#include "ui/ozone/platform/wayland/fake_server.h"
#include "ui/ozone/platform/wayland/wayland_connection.h"
#include "ui/ozone/platform/wayland/wayland_output.h"

namespace ui {

namespace {

const uint32_t kXdgVersion5 = 5;
const uint32_t kNumOfDisplays = 1;
const uint32_t kWidth = 800;
const uint32_t kHeight = 600;

void CheckDisplaySize(const std::vector<display::DisplaySnapshot*>& displays) {
  ASSERT_TRUE(displays.size() == kNumOfDisplays);

  // TODO(msisov): add multiple displays support.
  display::DisplaySnapshot* display_snapshot = displays.front();
  ASSERT_TRUE(display_snapshot->current_mode()->size() ==
              gfx::Size(kWidth, kHeight));
}
}

class OutputObserver : public WaylandOutput::Observer {
 public:
  explicit OutputObserver(const base::Closure& closure) : closure_(closure) {}

  void OnOutputReadyForUse() override {
    if (!closure_.is_null())
      closure_.Run();
  }

 private:
  const base::Closure closure_;
};

TEST(WaylandConnectionTest, UseUnstableVersion) {
  base::MessageLoopForUI message_loop;
  wl::FakeServer server;
  EXPECT_CALL(*server.xdg_shell(),
              UseUnstableVersion(XDG_SHELL_VERSION_CURRENT));
  ASSERT_TRUE(server.Start(kXdgVersion5));
  WaylandConnection connection;
  ASSERT_TRUE(connection.Initialize());
  connection.StartProcessingEvents();

  base::RunLoop().RunUntilIdle();
  server.Pause();
}

TEST(WaylandConnectionTest, Ping) {
  base::MessageLoopForUI message_loop;
  wl::FakeServer server;
  ASSERT_TRUE(server.Start(kXdgVersion5));
  WaylandConnection connection;
  ASSERT_TRUE(connection.Initialize());
  connection.StartProcessingEvents();

  base::RunLoop().RunUntilIdle();
  server.Pause();

  xdg_shell_send_ping(server.xdg_shell()->resource(), 1234);
  EXPECT_CALL(*server.xdg_shell(), Pong(1234));

  server.Resume();
  base::RunLoop().RunUntilIdle();
  server.Pause();
}

TEST(WaylandConnectionTest, Output) {
  base::MessageLoopForUI message_loop;
  wl::FakeServer server;
  ASSERT_TRUE(server.Start(kXdgVersion5));
  server.output()->SetRect(gfx::Rect(0, 0, kWidth, kHeight));
  WaylandConnection connection;
  ASSERT_TRUE(connection.Initialize());
  connection.StartProcessingEvents();

  base::RunLoop run_loop;
  OutputObserver observer(run_loop.QuitClosure());
  connection.PrimaryOutput()->SetObserver(&observer);
  run_loop.Run();

  connection.PrimaryOutput()->GetDisplaysSnapshot(
      base::BindOnce(&CheckDisplaySize));

  server.Resume();
  base::RunLoop().RunUntilIdle();
  server.Pause();
}

}  // namespace ui
