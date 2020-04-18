// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <memory>

#include "base/at_exit.h"
#include "base/command_line.h"
#include "base/i18n/icu_util.h"
#include "base/macros.h"
#include "base/memory/ptr_util.h"
#include "base/message_loop/message_loop.h"
#include "base/power_monitor/power_monitor.h"
#include "base/power_monitor/power_monitor_device_source.h"
#include "base/run_loop.h"
#include "base/task_scheduler/task_scheduler.h"
#include "build/build_config.h"
#include "components/viz/host/host_frame_sink_manager.h"
#include "third_party/skia/include/core/SkBlendMode.h"
#include "ui/aura/client/default_capture_client.h"
#include "ui/aura/client/window_parenting_client.h"
#include "ui/aura/env.h"
#include "ui/aura/test/test_focus_client.h"
#include "ui/aura/test/test_screen.h"
#include "ui/aura/window.h"
#include "ui/aura/window_delegate.h"
#include "ui/aura/window_tree_host.h"
#include "ui/base/hit_test.h"
#include "ui/base/ime/input_method_initializer.h"
#include "ui/compositor/paint_recorder.h"
#include "ui/compositor/test/in_process_context_factory.h"
#include "ui/events/event.h"
#include "ui/gfx/canvas.h"
#include "ui/gfx/geometry/rect.h"
#include "ui/gfx/skia_util.h"
#include "ui/gl/init/gl_factory.h"

#if defined(USE_X11)
#include "ui/gfx/x/x11_connection.h"  // nogncheck
#endif

#if defined(OS_WIN)
#include "ui/display/win/dpi.h"
#endif

namespace {

// Trivial WindowDelegate implementation that draws a colored background.
class DemoWindowDelegate : public aura::WindowDelegate {
 public:
  explicit DemoWindowDelegate(SkColor color) : color_(color) {}

  // Overridden from WindowDelegate:
  gfx::Size GetMinimumSize() const override { return gfx::Size(); }

  gfx::Size GetMaximumSize() const override { return gfx::Size(); }

  void OnBoundsChanged(const gfx::Rect& old_bounds,
                       const gfx::Rect& new_bounds) override {
    window_bounds_ = new_bounds;
  }
  gfx::NativeCursor GetCursor(const gfx::Point& point) override {
    return gfx::kNullCursor;
  }
  int GetNonClientComponent(const gfx::Point& point) const override {
    return HTCAPTION;
  }
  bool ShouldDescendIntoChildForEventHandling(
      aura::Window* child,
      const gfx::Point& location) override {
    return true;
  }
  bool CanFocus() override { return true; }
  void OnCaptureLost() override {}
  void OnPaint(const ui::PaintContext& context) override {
    ui::PaintRecorder recorder(context, window_bounds_.size());
    recorder.canvas()->DrawColor(color_, SkBlendMode::kSrc);
    gfx::Rect r;
    recorder.canvas()->GetClipBounds(&r);
    // Fill with a non-solid color so that the compositor will exercise its
    // texture upload path.
    while (!r.IsEmpty()) {
      r.Inset(2, 2);
      recorder.canvas()->FillRect(r, color_, SkBlendMode::kXor);
    }
  }
  void OnDeviceScaleFactorChanged(float old_device_scale_factor,
                                  float new_device_scale_factor) override {}
  void OnWindowDestroying(aura::Window* window) override {}
  void OnWindowDestroyed(aura::Window* window) override {}
  void OnWindowTargetVisibilityChanged(bool visible) override {}
  bool HasHitTestMask() const override { return false; }
  void GetHitTestMask(gfx::Path* mask) const override {}

 private:
  SkColor color_;
  gfx::Rect window_bounds_;

  DISALLOW_COPY_AND_ASSIGN(DemoWindowDelegate);
};

class DemoWindowParentingClient : public aura::client::WindowParentingClient {
 public:
  explicit DemoWindowParentingClient(aura::Window* window) : window_(window) {
    aura::client::SetWindowParentingClient(window_, this);
  }

  ~DemoWindowParentingClient() override {
    aura::client::SetWindowParentingClient(window_, nullptr);
  }

  // Overridden from aura::client::WindowParentingClient:
  aura::Window* GetDefaultParent(aura::Window* window,
                                 const gfx::Rect& bounds) override {
    if (!capture_client_) {
      capture_client_.reset(
          new aura::client::DefaultCaptureClient(window_->GetRootWindow()));
    }
    return window_;
  }

 private:
  aura::Window* window_;

  std::unique_ptr<aura::client::DefaultCaptureClient> capture_client_;

  DISALLOW_COPY_AND_ASSIGN(DemoWindowParentingClient);
};

int DemoMain() {
#if defined(USE_X11)
  // This demo uses InProcessContextFactory which uses X on a separate Gpu
  // thread.
  gfx::InitializeThreadedX11();
#endif

  gl::init::InitializeGLOneOff();

#if defined(OS_WIN)
  display::win::SetDefaultDeviceScaleFactor(1.0f);
#endif

  // Create the message-loop here before creating the root window.
  base::MessageLoopForUI message_loop;
  base::TaskScheduler::CreateAndStartWithDefaultParams("demo");
  ui::InitializeInputMethodForTesting();

  // The ContextFactory must exist before any Compositors are created.
  viz::HostFrameSinkManager host_frame_sink_manager;
  viz::FrameSinkManagerImpl frame_sink_manager;
  host_frame_sink_manager.SetLocalManager(&frame_sink_manager);
  frame_sink_manager.SetLocalClient(&host_frame_sink_manager);
  auto context_factory = std::make_unique<ui::InProcessContextFactory>(
      &host_frame_sink_manager, &frame_sink_manager);
  context_factory->set_use_test_surface(false);

  base::PowerMonitor power_monitor(
      base::WrapUnique(new base::PowerMonitorDeviceSource));

  std::unique_ptr<aura::Env> env = aura::Env::CreateInstance();
  env->set_context_factory(context_factory.get());
  env->set_context_factory_private(context_factory.get());
  std::unique_ptr<aura::TestScreen> test_screen(
      aura::TestScreen::Create(gfx::Size()));
  display::Screen::SetScreenInstance(test_screen.get());
  std::unique_ptr<aura::WindowTreeHost> host(
      test_screen->CreateHostForPrimaryDisplay());
  std::unique_ptr<DemoWindowParentingClient> window_parenting_client(
      new DemoWindowParentingClient(host->window()));
  aura::test::TestFocusClient focus_client;
  aura::client::SetFocusClient(host->window(), &focus_client);

  // Create a hierarchy of test windows.
  gfx::Rect window1_bounds(100, 100, 400, 400);
  DemoWindowDelegate window_delegate1(SK_ColorBLUE);
  aura::Window window1(&window_delegate1);
  window1.set_id(1);
  window1.Init(ui::LAYER_TEXTURED);
  window1.SetBounds(window1_bounds);
  window1.Show();
  aura::client::ParentWindowWithContext(&window1, host->window(), gfx::Rect());

  gfx::Rect window2_bounds(200, 200, 350, 350);
  DemoWindowDelegate window_delegate2(SK_ColorRED);
  aura::Window window2(&window_delegate2);
  window2.set_id(2);
  window2.Init(ui::LAYER_TEXTURED);
  window2.SetBounds(window2_bounds);
  window2.Show();
  aura::client::ParentWindowWithContext(&window2, host->window(), gfx::Rect());

  gfx::Rect window3_bounds(10, 10, 50, 50);
  DemoWindowDelegate window_delegate3(SK_ColorGREEN);
  aura::Window window3(&window_delegate3);
  window3.set_id(3);
  window3.Init(ui::LAYER_TEXTURED);
  window3.SetBounds(window3_bounds);
  window3.Show();
  window2.AddChild(&window3);

  host->Show();
  base::RunLoop().Run();

  return 0;
}

}  // namespace

int main(int argc, char** argv) {
  base::CommandLine::Init(argc, argv);

  // The exit manager is in charge of calling the dtors of singleton objects.
  base::AtExitManager exit_manager;

  base::i18n::InitializeICU();

  return DemoMain();
}
