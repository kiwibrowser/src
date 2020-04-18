// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_RENDERER_IME_EVENT_GUARD_H_
#define CONTENT_RENDERER_IME_EVENT_GUARD_H_

namespace content {
class RenderWidget;

// Simple RAII object for guarding IME updates. Calls OnImeGuardStart on
// construction and OnImeGuardFinish on destruction.
class ImeEventGuard {
 public:
  explicit ImeEventGuard(RenderWidget* widget);
  ~ImeEventGuard();

  bool show_virtual_keyboard() const { return show_virtual_keyboard_; }
  bool reply_to_request() const { return reply_to_request_; }
  void set_show_virtual_keyboard(bool show) { show_virtual_keyboard_ = show; }

 private:
  RenderWidget* widget_;
  bool show_virtual_keyboard_;
  bool reply_to_request_;
};
}

#endif  // CONTENT_RENDERER_IME_EVENT_GUARD_H_

