// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/renderer/text_input_client_observer.h"

#include <stddef.h>

#include <memory>

#include "build/build_config.h"
#include "content/common/text_input_client_messages.h"
#include "content/renderer/pepper/pepper_plugin_instance_impl.h"
#include "content/renderer/render_frame_impl.h"
#include "content/renderer/render_view_impl.h"
#include "content/renderer/render_widget.h"
#include "ipc/ipc_message.h"
#include "ppapi/buildflags/buildflags.h"
#include "third_party/blink/public/platform/web_point.h"
#include "third_party/blink/public/platform/web_rect.h"
#include "third_party/blink/public/platform/web_string.h"
#include "third_party/blink/public/web/web_local_frame.h"
#include "third_party/blink/public/web/web_view.h"
#include "ui/gfx/geometry/rect.h"

#if defined(OS_MACOSX)
#include "third_party/blink/public/web/mac/web_substring_util.h"
#endif

namespace content {

namespace {
uint32_t GetCurrentCursorPositionInFrame(blink::WebLocalFrame* localFrame) {
  blink::WebRange range = localFrame->SelectionRange();
  return range.IsNull() ? 0U : static_cast<uint32_t>(range.StartOffset());
}
}

TextInputClientObserver::TextInputClientObserver(RenderWidget* render_widget)
    : render_widget_(render_widget) {}

TextInputClientObserver::~TextInputClientObserver() {
}

bool TextInputClientObserver::OnMessageReceived(const IPC::Message& message) {
  bool handled = true;
  IPC_BEGIN_MESSAGE_MAP(TextInputClientObserver, message)
    IPC_MESSAGE_HANDLER(TextInputClientMsg_StringAtPoint,
                        OnStringAtPoint)
    IPC_MESSAGE_HANDLER(TextInputClientMsg_CharacterIndexForPoint,
                        OnCharacterIndexForPoint)
    IPC_MESSAGE_HANDLER(TextInputClientMsg_FirstRectForCharacterRange,
                        OnFirstRectForCharacterRange)
    IPC_MESSAGE_HANDLER(TextInputClientMsg_StringForRange, OnStringForRange)
    IPC_MESSAGE_UNHANDLED(handled = false)
  IPC_END_MESSAGE_MAP()
  return handled;
}

bool TextInputClientObserver::Send(IPC::Message* message) {
  return render_widget_->Send(message);
}

blink::WebFrameWidget* TextInputClientObserver::GetWebFrameWidget() const {
  blink::WebWidget* widget = render_widget_->GetWebWidget();
  if (!widget->IsWebFrameWidget()) {
    // When a page navigation occurs, for a brief period
    // RenderViewImpl::GetWebWidget() will return a WebViewImpl instead of a
    // WebViewFrameWidget. Therefore, casting to WebFrameWidget is invalid and
    // could cause crashes. Also, WebView::mainFrame() could be a remote frame
    // which will yield a nullptr for localRoot() (https://crbug.com/664890).
    return nullptr;
  }
  return static_cast<blink::WebFrameWidget*>(widget);
}

blink::WebLocalFrame* TextInputClientObserver::GetFocusedFrame() const {
  if (auto* frame_widget = GetWebFrameWidget()) {
    blink::WebLocalFrame* localRoot = frame_widget->LocalRoot();
    RenderFrameImpl* render_frame = RenderFrameImpl::FromWebFrame(localRoot);
    if (!render_frame) {
      // TODO(ekaramad): Can this ever be nullptr? (https://crbug.com/664890).
      return nullptr;
    }
    blink::WebLocalFrame* focused =
        render_frame->render_view()->webview()->FocusedFrame();
    return focused->LocalRoot() == localRoot ? focused : nullptr;
  }
  return nullptr;
}

#if BUILDFLAG(ENABLE_PLUGINS)
PepperPluginInstanceImpl* TextInputClientObserver::GetFocusedPepperPlugin()
    const {
  blink::WebLocalFrame* focusedFrame = GetFocusedFrame();
  return focusedFrame
             ? RenderFrameImpl::FromWebFrame(focusedFrame)
                   ->focused_pepper_plugin()
             : nullptr;
}
#endif

void TextInputClientObserver::OnStringAtPoint(gfx::Point point) {
#if defined(OS_MACOSX)
  blink::WebPoint baselinePoint;
  NSAttributedString* string = nil;

  if (auto* frame_widget = GetWebFrameWidget()) {
    string = blink::WebSubstringUtil::AttributedWordAtPoint(frame_widget, point,
                                                            baselinePoint);
  }

  std::unique_ptr<const mac::AttributedStringCoder::EncodedString> encoded(
      mac::AttributedStringCoder::Encode(string));
  Send(new TextInputClientReplyMsg_GotStringAtPoint(
      render_widget_->routing_id(), *encoded.get(), baselinePoint));
#else
  NOTIMPLEMENTED();
#endif
}

void TextInputClientObserver::OnCharacterIndexForPoint(gfx::Point point) {
  blink::WebPoint web_point(point);
  uint32_t index = 0U;
  if (auto* frame = GetFocusedFrame())
    index = static_cast<uint32_t>(frame->CharacterIndexForPoint(web_point));

  Send(new TextInputClientReplyMsg_GotCharacterIndexForPoint(
      render_widget_->routing_id(), index));
}

void TextInputClientObserver::OnFirstRectForCharacterRange(gfx::Range range) {
  gfx::Rect rect;
#if BUILDFLAG(ENABLE_PLUGINS)
  PepperPluginInstanceImpl* focused_plugin = GetFocusedPepperPlugin();
  if (focused_plugin) {
    rect = focused_plugin->GetCaretBounds();
  } else
#endif
  {
    blink::WebLocalFrame* frame = GetFocusedFrame();
    // TODO(yabinh): Null check should not be necessary.
    // See crbug.com/304341
    if (frame) {
      blink::WebRect web_rect;
      // When request range is invalid we will try to obtain it from current
      // frame selection. The fallback value will be 0.
      uint32_t start = range.IsValid() ? range.start()
                                       : GetCurrentCursorPositionInFrame(frame);
      frame->FirstRectForCharacterRange(start, range.length(), web_rect);
      rect = web_rect;
    }
  }
  Send(new TextInputClientReplyMsg_GotFirstRectForRange(
      render_widget_->routing_id(), rect));
}

void TextInputClientObserver::OnStringForRange(gfx::Range range) {
#if defined(OS_MACOSX)
  blink::WebPoint baselinePoint;
  NSAttributedString* string = nil;
  blink::WebLocalFrame* frame = GetFocusedFrame();
  // TODO(yabinh): Null check should not be necessary.
  // See crbug.com/304341
  if (frame) {
    string = blink::WebSubstringUtil::AttributedSubstringInRange(
        frame, range.start(), range.length(), &baselinePoint);
  }
  std::unique_ptr<const mac::AttributedStringCoder::EncodedString> encoded(
      mac::AttributedStringCoder::Encode(string));
  Send(new TextInputClientReplyMsg_GotStringForRange(
      render_widget_->routing_id(), *encoded.get(), baselinePoint));
#else
  NOTIMPLEMENTED();
#endif
}

}  // namespace content
