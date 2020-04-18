// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/ozone/platform/wayland/wayland_test.h"

#include "base/run_loop.h"
#include "ui/events/ozone/layout/keyboard_layout_engine_manager.h"

#if BUILDFLAG(USE_XKBCOMMON)
#include "ui/ozone/platform/wayland/wayland_xkb_keyboard_layout_engine.h"
#else
#include "ui/events/ozone/layout/stub/stub_keyboard_layout_engine.h"
#endif

using ::testing::SaveArg;
using ::testing::_;

namespace ui {

WaylandTest::WaylandTest() {
#if BUILDFLAG(USE_XKBCOMMON)
  KeyboardLayoutEngineManager::SetKeyboardLayoutEngine(
      std::make_unique<WaylandXkbKeyboardLayoutEngine>(
          xkb_evdev_code_converter_));
#else
  KeyboardLayoutEngineManager::SetKeyboardLayoutEngine(
      std::make_unique<StubKeyboardLayoutEngine>());
#endif
  connection_.reset(new WaylandConnection);
  window_ = std::make_unique<WaylandWindow>(&delegate_, connection_.get(),
                                            gfx::Rect(0, 0, 800, 600));
}

WaylandTest::~WaylandTest() {}

void WaylandTest::SetUp() {
  ASSERT_TRUE(server_.Start(GetParam()));
  ASSERT_TRUE(connection_->Initialize());
  EXPECT_CALL(delegate_, OnAcceleratedWidgetAvailable(_, _))
      .WillOnce(SaveArg<0>(&widget_));
  ASSERT_TRUE(window_->Initialize());
  ASSERT_NE(widget_, gfx::kNullAcceleratedWidget);

  // Wait for the client to flush all pending requests from initialization.
  base::RunLoop().RunUntilIdle();

  // Pause the server after it has responded to all incoming events.
  server_.Pause();

  surface_ = server_.GetObject<wl::MockSurface>(widget_);
  ASSERT_TRUE(surface_);

  initialized_ = true;
}

void WaylandTest::TearDown() {
  if (initialized_)
    Sync();
}

void WaylandTest::Sync() {
  // Resume the server, flushing its pending events.
  server_.Resume();

  // Wait for the client to finish processing these events.
  base::RunLoop().RunUntilIdle();

  // Pause the server, after it has finished processing any follow-up requests
  // from the client.
  server_.Pause();
}

}  // namespace ui
