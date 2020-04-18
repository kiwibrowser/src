// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_RENDERER_RESIZING_MODE_SELECTOR_H_
#define CONTENT_RENDERER_RESIZING_MODE_SELECTOR_H_

#include "base/macros.h"

namespace content {

class RenderWidget;
struct VisualProperties;

// Enables switching between two modes of resizing:
// 1) The "normal" (asynchronous) resizing, which involves sending messages to
//    and receiving them from host; and
// 2) The synchronous mode, which short-circuits the resizing logic to operate
//    strictly inside renderer.
// The latter is necessary to support a handful of layout tests that were
// written with the expectation of a synchronous resize, and we're going to
// eventually rewrite or remove all of them. See http://crbug.com/309760 for
// details.
class ResizingModeSelector {
 public:
  ResizingModeSelector();
  bool NeverUsesSynchronousResize() const;
  bool ShouldAbortOnResize(RenderWidget* widget,
                           const VisualProperties& visual_properties);

  void set_is_synchronous_mode(bool mode);
  bool is_synchronous_mode() const;

 private:
  bool is_synchronous_mode_;

  DISALLOW_COPY_AND_ASSIGN(ResizingModeSelector);
};

}  // namespace content

#endif  // CONTENT_RENDERER_RESIZING_MODE_SELECTOR_H_
