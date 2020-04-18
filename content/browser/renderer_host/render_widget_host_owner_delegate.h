// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_RENDERER_HOST_RENDER_WIDGET_HOST_OWNER_DELEGATE_H_
#define CONTENT_BROWSER_RENDERER_HOST_RENDER_WIDGET_HOST_OWNER_DELEGATE_H_

#include "content/common/content_export.h"

namespace IPC {
class Message;
}

namespace blink {
class WebMouseEvent;
}

namespace content {

struct NativeWebKeyboardEvent;

//
// RenderWidgetHostOwnerDelegate
//
//  An interface implemented by an object owning a RenderWidgetHost. This is
//  intended to be temporary until the RenderViewHostImpl and
//  RenderWidgetHostImpl classes are disentangled; see http://crbug.com/542477
//  and http://crbug.com/478281.
class CONTENT_EXPORT RenderWidgetHostOwnerDelegate {
 public:
  // The RenderWidgetHost received an IPC message. Return true if this delegate
  // handles it.
  virtual bool OnMessageReceived(const IPC::Message& msg) = 0;

  // The RenderWidgetHost has been initialized.
  virtual void RenderWidgetDidInit() = 0;

  // The RenderWidgetHost will be setting its loading state.
  virtual void RenderWidgetWillSetIsLoading(bool is_loading) = 0;

  // The RenderWidgetHost got the focus.
  virtual void RenderWidgetGotFocus() = 0;

  // The RenderWidgetHost lost the focus.
  virtual void RenderWidgetLostFocus() = 0;

  // The RenderWidgetHost forwarded a mouse event.
  virtual void RenderWidgetDidForwardMouseEvent(
      const blink::WebMouseEvent& mouse_event) = 0;

  // The RenderWidgetHost wants to forward a keyboard event; returns whether
  // it's allowed to do so.
  virtual bool MayRenderWidgetForwardKeyboardEvent(
      const NativeWebKeyboardEvent& key_event) = 0;

  // Allow OwnerDelegate to control whether its RenderWidgetHost contributes
  // priority to the RenderProcessHost.
  virtual bool ShouldContributePriorityToProcess() = 0;

 protected:
  virtual ~RenderWidgetHostOwnerDelegate() {}
};

}  // namespace content

#endif  // CONTENT_BROWSER_RENDERER_HOST_RENDER_WIDGET_HOST_OWNER_DELEGATE_H_
