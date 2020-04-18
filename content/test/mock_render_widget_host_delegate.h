// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_TEST_MOCK_RENDER_WIDGET_HOST_DELEGATE_H_
#define CONTENT_TEST_MOCK_RENDER_WIDGET_HOST_DELEGATE_H_

#include <memory>

#include "base/macros.h"
#include "content/browser/renderer_host/render_widget_host_delegate.h"
#include "content/browser/renderer_host/text_input_manager.h"
#include "content/public/browser/keyboard_event_processing_result.h"

namespace content {

class RenderWidgetHostImpl;

class MockRenderWidgetHostDelegate : public RenderWidgetHostDelegate {
 public:
  MockRenderWidgetHostDelegate();
  ~MockRenderWidgetHostDelegate() override;

  const NativeWebKeyboardEvent* last_event() const { return last_event_.get(); }
  void set_widget_host(RenderWidgetHostImpl* rwh) { rwh_ = rwh; }
  void set_is_fullscreen(bool is_fullscreen) { is_fullscreen_ = is_fullscreen; }
  void set_focused_widget(RenderWidgetHostImpl* focused_widget) {
    focused_widget_ = focused_widget;
  }
  void set_pre_handle_keyboard_event_result(
      KeyboardEventProcessingResult result) {
    pre_handle_keyboard_event_result_ = result;
  }

  // RenderWidgetHostDelegate:
  void ResizeDueToAutoResize(RenderWidgetHostImpl* render_widget_host,
                             const gfx::Size& new_size) override;
  KeyboardEventProcessingResult PreHandleKeyboardEvent(
      const NativeWebKeyboardEvent& event) override;
  void ExecuteEditCommand(const std::string& command,
                          const base::Optional<base::string16>& value) override;
  void Cut() override;
  void Copy() override;
  void Paste() override;
  void SelectAll() override;
  RenderWidgetHostImpl* GetFocusedRenderWidgetHost(
      RenderWidgetHostImpl* widget_host) override;
  void SendScreenRects() override;
  TextInputManager* GetTextInputManager() override;
  bool IsFullscreenForCurrentTab() const override;

 private:
  std::unique_ptr<NativeWebKeyboardEvent> last_event_;
  RenderWidgetHostImpl* rwh_ = nullptr;
  bool is_fullscreen_ = false;
  TextInputManager text_input_manager_;
  RenderWidgetHostImpl* focused_widget_ = nullptr;
  KeyboardEventProcessingResult pre_handle_keyboard_event_result_ =
      KeyboardEventProcessingResult::NOT_HANDLED;

  DISALLOW_COPY_AND_ASSIGN(MockRenderWidgetHostDelegate);
};

}  // namespace content

#endif  // CONTENT_TEST_MOCK_RENDER_WIDGET_HOST_DELEGATE_H_
