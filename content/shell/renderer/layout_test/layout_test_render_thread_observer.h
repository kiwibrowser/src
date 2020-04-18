// Copyright (c) 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_SHELL_RENDERER_LAYOUT_TEST_LAYOUT_TEST_RENDER_THREAD_OBSERVER_H_
#define CONTENT_SHELL_RENDERER_LAYOUT_TEST_LAYOUT_TEST_RENDER_THREAD_OBSERVER_H_

#include <memory>

#include "base/compiler_specific.h"
#include "base/files/file_path.h"
#include "base/macros.h"
#include "content/public/renderer/render_thread_observer.h"
#include "ipc/ipc_platform_file.h"

namespace base {
class DictionaryValue;
}

namespace test_runner {
class WebTestInterfaces;
}

namespace content {

class LayoutTestRenderThreadObserver : public RenderThreadObserver {
 public:
  static LayoutTestRenderThreadObserver* GetInstance();

  LayoutTestRenderThreadObserver();
  ~LayoutTestRenderThreadObserver() override;

  // RenderThreadObserver implementation.
  bool OnControlMessageReceived(const IPC::Message& message) override;

  test_runner::WebTestInterfaces* test_interfaces() const {
    return test_interfaces_.get();
  }

 private:
  // Message handlers.
  void OnReplicateLayoutTestRuntimeFlagsChanges(
      const base::DictionaryValue& changed_layout_test_runtime_flags);

  std::unique_ptr<test_runner::WebTestInterfaces> test_interfaces_;

  DISALLOW_COPY_AND_ASSIGN(LayoutTestRenderThreadObserver);
};

}  // namespace content

#endif  // CONTENT_SHELL_RENDERER_LAYOUT_TEST_LAYOUT_TEST_RENDER_THREAD_OBSERVER_H_
