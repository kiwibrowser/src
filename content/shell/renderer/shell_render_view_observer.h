// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_SHELL_RENDERER_SHELL_RENDER_VIEW_OBSERVER_H_
#define CONTENT_SHELL_RENDERER_SHELL_RENDER_VIEW_OBSERVER_H_

#include "base/macros.h"
#include "content/public/renderer/render_view_observer.h"

namespace blink {
class WebLocalFrame;
}

namespace content {

class RenderView;


class ShellRenderViewObserver : public RenderViewObserver {
 public:
  explicit ShellRenderViewObserver(RenderView* render_view);
  ~ShellRenderViewObserver() override;

 private:
  // RenderViewObserver implementation.
  void DidClearWindowObject(blink::WebLocalFrame* frame) override;
  void OnDestruct() override;

  DISALLOW_COPY_AND_ASSIGN(ShellRenderViewObserver);
};

}  // namespace content

#endif  // CONTENT_SHELL_RENDERER_SHELL_RENDER_VIEW_OBSERVER_H_
