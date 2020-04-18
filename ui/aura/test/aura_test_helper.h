// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_AURA_TEST_AURA_TEST_HELPER_H_
#define UI_AURA_TEST_AURA_TEST_HELPER_H_

#include <memory>

#include "base/macros.h"
#include "ui/aura/env.h"
#include "ui/aura/mus/window_tree_client.h"
#include "ui/aura/window_event_dispatcher.h"
#include "ui/aura/window_tree_host.h"

namespace ui {
class ContextFactory;
class ContextFactoryPrivate;
class ScopedAnimationDurationScaleMode;
}

namespace wm {
class WMState;
}

namespace aura {
class Env;
class TestScreen;
class TestWindowManagerDelegate;
class TestWindowTree;
class TestWindowTreeClientDelegate;
class TestWindowTreeClientSetup;
class Window;
class WindowManagerDelegate;
class WindowTreeClient;
class WindowTreeClientDelegate;

namespace client {
class CaptureClient;
class DefaultCaptureClient;
class FocusClient;
}
namespace test {
class EnvWindowTreeClientSetter;
class TestWindowParentingClient;

// A helper class owned by tests that does common initialization required for
// Aura use. This class creates a root window with clients and other objects
// that are necessary to run test on Aura.
class AuraTestHelper {
 public:
  AuraTestHelper();
  ~AuraTestHelper();

  // Returns the current AuraTestHelper, or nullptr if it's not alive.
  static AuraTestHelper* GetInstance();

  // Makes aura target mus with a mock WindowTree (TestWindowTree). Must be
  // called before SetUp().
  // TODO(sky): remove |config|. https://crbug.com/842365
  void EnableMusWithTestWindowTree(
      WindowTreeClientDelegate* window_tree_delegate,
      WindowManagerDelegate* window_manager_delegate,
      WindowTreeClient::Config config = WindowTreeClient::Config::kMash);

  // Makes aura target mus with the specified WindowTreeClient. Must be called
  // before SetUp().
  void EnableMusWithWindowTreeClient(WindowTreeClient* window_tree_client);

  // Deletes the WindowTreeClient now. Normally the WindowTreeClient is deleted
  // at the right time and there is no need to call this. This is provided for
  // testing shutdown ordering.
  void DeleteWindowTreeClient();

  // Creates and initializes (shows and sizes) the RootWindow for use in tests.
  void SetUp(ui::ContextFactory* context_factory,
             ui::ContextFactoryPrivate* context_factory_private);

  // Clean up objects that are created for tests. This also deletes the Env
  // object.
  void TearDown();

  // Flushes message loop.
  void RunAllPendingInMessageLoop();

  Window* root_window() { return host_ ? host_->window() : nullptr; }
  ui::EventSink* event_sink() { return host_->event_sink(); }
  WindowTreeHost* host() { return host_.get(); }

  TestScreen* test_screen() { return test_screen_.get(); }

  // This function only returns a valid value if EnableMusWithTestWindowTree()
  // was called.
  TestWindowTree* window_tree();

  // Returns a WindowTreeClient only if one of the EnableMus functions is
  // called.
  WindowTreeClient* window_tree_client();

  client::FocusClient* focus_client() { return focus_client_.get(); }
  client::CaptureClient* capture_client();

 private:
  enum class Mode {
    // Classic aura.
    LOCAL,

    // Mus with a test WindowTree implementation that does not target the real
    // service:ui.
    MUS_CREATE_WINDOW_TREE_CLIENT,

    // Mus with a test WindowTree implementation that does not target the real
    // service:ui.
    // TODO(sky): combine this with MUS_CREATE_WINDOW_TREE_CLIENT.
    // https://crbug.com/842365.
    MUS2_CREATE_WINDOW_TREE_CLIENT,

    // Mus without creating a WindowTree. This is used when the test wants to
    // create the WindowTreeClient itself. This mode is enabled by way of
    // EnableMusWithWindowTreeClient().
    MUS,
  };

  // Initializes a WindowTreeClient with a test WindowTree.
  void InitWindowTreeClient();

  Mode mode_ = Mode::LOCAL;
  bool setup_called_;
  bool teardown_called_;
  ui::ContextFactory* context_factory_to_restore_ = nullptr;
  ui::ContextFactoryPrivate* context_factory_private_to_restore_ = nullptr;
  std::unique_ptr<EnvWindowTreeClientSetter> env_window_tree_client_setter_;
  // This is only created if Env has already been created and it's Mode is MUS.
  std::unique_ptr<TestWindowTreeClientDelegate>
      test_window_tree_client_delegate_;
  // This is only created if Env has already been created and it's Mode is MUS.
  std::unique_ptr<TestWindowManagerDelegate> test_window_manager_delegate_;
  std::unique_ptr<TestWindowTreeClientSetup> window_tree_client_setup_;
  Env::Mode env_mode_to_restore_ = Env::Mode::LOCAL;
  std::unique_ptr<aura::Env> env_;
  std::unique_ptr<wm::WMState> wm_state_;
  std::unique_ptr<WindowTreeHost> host_;
  std::unique_ptr<TestWindowParentingClient> parenting_client_;
  std::unique_ptr<client::DefaultCaptureClient> capture_client_;
  std::unique_ptr<client::FocusClient> focus_client_;
  std::unique_ptr<TestScreen> test_screen_;
  std::unique_ptr<ui::ScopedAnimationDurationScaleMode> zero_duration_mode_;
  WindowTreeClientDelegate* window_tree_delegate_ = nullptr;
  WindowManagerDelegate* window_manager_delegate_ = nullptr;

  WindowTreeClient* window_tree_client_ = nullptr;

  DISALLOW_COPY_AND_ASSIGN(AuraTestHelper);
};

}  // namespace test
}  // namespace aura

#endif  // UI_AURA_TEST_AURA_TEST_HELPER_H_
