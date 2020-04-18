// Copyright (c) 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_DEVTOOLS_DEVTOOLS_FRAME_TRACE_RECORDER_H_
#define CONTENT_BROWSER_DEVTOOLS_DEVTOOLS_FRAME_TRACE_RECORDER_H_

#include <memory>

#include "base/compiler_specific.h"
#include "base/macros.h"
#include "base/memory/weak_ptr.h"

namespace viz {
class CompositorFrameMetadata;
}

namespace content {

class RenderFrameHostImpl;

class DevToolsFrameTraceRecorder {
 public:
  DevToolsFrameTraceRecorder();
  ~DevToolsFrameTraceRecorder();

  void OnSwapCompositorFrame(
      RenderFrameHostImpl* host,
      const viz::CompositorFrameMetadata& frame_metadata);

  void OnSynchronousSwapCompositorFrame(
      RenderFrameHostImpl* host,
      const viz::CompositorFrameMetadata& frame_metadata);

  static constexpr int kMaximumNumberOfScreenshots = 450;

 private:
  DISALLOW_COPY_AND_ASSIGN(DevToolsFrameTraceRecorder);
  std::unique_ptr<viz::CompositorFrameMetadata> last_metadata_;
};

}  // namespace content

#endif  // CONTENT_BROWSER_DEVTOOLS_DEVTOOLS_FRAME_TRACE_RECORDER_H_
