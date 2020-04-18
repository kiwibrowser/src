// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_UI_WS2_TEST_WINDOW_SERVICE_DELEGATE_H_
#define SERVICES_UI_WS2_TEST_WINDOW_SERVICE_DELEGATE_H_

#include "base/macros.h"
#include "services/ui/ws2/window_service_delegate.h"

namespace aura {
class WindowDelegate;
}

namespace ui {
namespace ws2 {

class TestWindowServiceDelegate : public WindowServiceDelegate {
 public:
  // |top_level_parent| is the parent of new top-levels. If null, top-levels
  // have no parent.
  explicit TestWindowServiceDelegate(aura::Window* top_level_parent = nullptr);
  ~TestWindowServiceDelegate() override;

  void set_top_level_parent(aura::Window* parent) {
    top_level_parent_ = parent;
  }

  void set_delegate_for_next_top_level(aura::WindowDelegate* delegate) {
    delegate_for_next_top_level_ = delegate;
  }

  // WindowServiceDelegate:
  std::unique_ptr<aura::Window> NewTopLevel(
      aura::PropertyConverter* property_converter,
      const base::flat_map<std::string, std::vector<uint8_t>>& properties)
      override;

 private:
  aura::Window* top_level_parent_;
  aura::WindowDelegate* delegate_for_next_top_level_ = nullptr;

  DISALLOW_COPY_AND_ASSIGN(TestWindowServiceDelegate);
};

}  // namespace ws2
}  // namespace ui

#endif  // SERVICES_UI_WS2_TEST_WINDOW_SERVICE_DELEGATE_H_
