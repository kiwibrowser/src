// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_CORE_FRAME_REMOTE_DOM_WINDOW_H_
#define THIRD_PARTY_BLINK_RENDERER_CORE_FRAME_REMOTE_DOM_WINDOW_H_

#include "third_party/blink/renderer/core/frame/dom_window.h"
#include "third_party/blink/renderer/core/frame/remote_frame.h"
#include "third_party/blink/renderer/platform/wtf/assertions.h"

namespace blink {

class RemoteDOMWindow final : public DOMWindow {
 public:
  static RemoteDOMWindow* Create(RemoteFrame& frame) {
    return new RemoteDOMWindow(frame);
  }

  RemoteFrame* GetFrame() const { return ToRemoteFrame(DOMWindow::GetFrame()); }

  // EventTarget overrides:
  ExecutionContext* GetExecutionContext() const override;

  // DOMWindow overrides:
  void Trace(blink::Visitor*) override;
  void blur() override;

  void FrameDetached();

 protected:
  // Protected DOMWindow overrides:
  void SchedulePostMessage(MessageEvent*,
                           scoped_refptr<const SecurityOrigin> target,
                           Document* source) override;

 private:
  explicit RemoteDOMWindow(RemoteFrame&);

  // Intentionally private to prevent redundant checks when the type is
  // already RemoteDOMWindow.
  bool IsLocalDOMWindow() const override { return false; }
  bool IsRemoteDOMWindow() const override { return true; }

  void ForwardPostMessage(MessageEvent*,
                          scoped_refptr<const SecurityOrigin> target,
                          Document* source,
                          bool has_user_gesture);
};

DEFINE_TYPE_CASTS(RemoteDOMWindow,
                  DOMWindow,
                  x,
                  x->IsRemoteDOMWindow(),
                  x.IsRemoteDOMWindow());

}  // namespace blink

#endif  // DOMWindow_h
