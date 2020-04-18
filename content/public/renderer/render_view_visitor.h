// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_PUBLIC_RENDERER_RENDER_VIEW_VISITOR_H_
#define CONTENT_PUBLIC_RENDERER_RENDER_VIEW_VISITOR_H_

namespace content {

class RenderView;

class RenderViewVisitor {
 public:
  // Return true to continue visiting RenderViews or false to stop.
  virtual bool Visit(RenderView* render_view) = 0;

 protected:
  virtual ~RenderViewVisitor() {}
};

}  // namespace content

#endif  // CONTENT_PUBLIC_RENDERER_RENDER_VIEW_VISITOR_H_
