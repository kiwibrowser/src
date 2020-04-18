// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_RENDERER_RENDER_WIDGET_OWNER_DELEGATE_H_
#define CONTENT_RENDERER_RENDER_WIDGET_OWNER_DELEGATE_H_

#include "content/common/content_export.h"

namespace blink {
class WebMouseEvent;
}

namespace content {

//
// RenderWidgetOwnerDelegate
//
//  An interface implemented by an object owning a RenderWidget. This is
//  intended to be temporary until the RenderViewImpl and RenderWidget classes
//  are disentangled; see https://crbug.com/583347 and https://crbug.com/478281.
class CONTENT_EXPORT RenderWidgetOwnerDelegate {
 public:
  // As in RenderWidgetInputHandlerDelegate.
  virtual bool RenderWidgetWillHandleMouseEvent(
      const blink::WebMouseEvent& event) = 0;

 protected:
  virtual ~RenderWidgetOwnerDelegate() {}
};

}  // namespace content

#endif  // CONTENT_RENDERER_RENDER_WIDGET_OWNER_DELEGATE_H_
