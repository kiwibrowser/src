// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_UI_WS2_WINDOW_SERVICE_TEST_SETUP_H_
#define SERVICES_UI_WS2_WINDOW_SERVICE_TEST_SETUP_H_

#include <memory>

#include "base/macros.h"
#include "base/test/scoped_task_environment.h"
#include "services/ui/ws2/test_window_service_delegate.h"
#include "services/ui/ws2/test_window_tree_client.h"
#include "services/ui/ws2/window_service_client_test_helper.h"
#include "ui/aura/test/aura_test_helper.h"
#include "ui/compositor/test/context_factories_for_test.h"
#include "ui/wm/core/focus_controller.h"

namespace wm {
class ScopedCaptureClient;
}

namespace ui {
namespace ws2 {

class WindowService;
class WindowServiceClient;
class WindowServiceClientTestHelper;

struct EmbeddingHelper;

// Helper to setup state needed for WindowService tests.
class WindowServiceTestSetup {
 public:
  // See |WindowServiceClient::intercepts_events| for details on
  // |intercepts_events|.
  explicit WindowServiceTestSetup(bool intercepts_events = false);
  ~WindowServiceTestSetup();

  // |flags| mirrors that from mojom::WindowTree::Embed(), see it for details.
  std::unique_ptr<EmbeddingHelper> CreateEmbedding(aura::Window* embed_root,
                                                   uint32_t flags = 0);

  aura::Window* root() { return aura_test_helper_.root_window(); }
  TestWindowServiceDelegate* delegate() { return &delegate_; }
  TestWindowTreeClient* window_tree_client() { return &window_tree_client_; }
  WindowServiceClientTestHelper* client_test_helper() {
    return client_test_helper_.get();
  }
  wm::FocusController* focus_controller() { return &focus_controller_; }

  std::vector<Change>* changes() {
    return window_tree_client_.tracker()->changes();
  }

 private:
  base::test::ScopedTaskEnvironment task_environment_{
      base::test::ScopedTaskEnvironment::MainThreadType::UI};
  wm::FocusController focus_controller_;
  aura::test::AuraTestHelper aura_test_helper_;
  std::unique_ptr<wm::ScopedCaptureClient> scoped_capture_client_;
  TestWindowServiceDelegate delegate_;
  std::unique_ptr<WindowService> service_;
  TestWindowTreeClient window_tree_client_;
  std::unique_ptr<WindowServiceClient> window_service_client_;
  std::unique_ptr<WindowServiceClientTestHelper> client_test_helper_;

  DISALLOW_COPY_AND_ASSIGN(WindowServiceTestSetup);
};

// EmbeddingHelper contains the object necessary for an embedding. This is
// created by way of WindowServiceTestSetup::CreateEmbedding().
struct EmbeddingHelper {
  EmbeddingHelper();
  ~EmbeddingHelper();

  std::vector<Change>* changes() {
    return window_tree_client.tracker()->changes();
  }

  // The Embedding. This is owned by the window the embedding was created on.
  Embedding* embedding = nullptr;

  TestWindowTreeClient window_tree_client;

  // The client Embed() was called on.
  WindowServiceClient* parent_window_service_client = nullptr;

  // NOTE: this is owned by |parent_window_service_client|.
  WindowServiceClient* window_service_client = nullptr;

  std::unique_ptr<WindowServiceClientTestHelper> client_test_helper;
};

}  // namespace ws2
}  // namespace ui

#endif  // SERVICES_UI_WS2_WINDOW_SERVICE_TEST_SETUP_H_
