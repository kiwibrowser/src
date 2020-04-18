// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CC_TREES_RENDER_FRAME_METADATA_OBSERVER_H_
#define CC_TREES_RENDER_FRAME_METADATA_OBSERVER_H_

#include "base/macros.h"
#include "cc/cc_export.h"
#include "cc/trees/render_frame_metadata.h"

namespace cc {

class FrameTokenAllocator;

// Observes RenderFrameMetadata associated with the submission of a frame.
// LayerTreeHostImpl will create the metadata when submitting a CompositorFrame.
//
// Calls to this will be done from the compositor thread.
class CC_EXPORT RenderFrameMetadataObserver {
 public:
  RenderFrameMetadataObserver() = default;
  virtual ~RenderFrameMetadataObserver() = default;

  // Binds on the current thread. This should only be called from the compositor
  // thread.
  virtual void BindToCurrentThread(
      FrameTokenAllocator* frame_token_allocator) = 0;

  // Notification of the RendarFrameMetadata for the frame being submitted to
  // the display compositor.
  virtual void OnRenderFrameSubmission(RenderFrameMetadata metadata) = 0;

 private:
  DISALLOW_COPY_AND_ASSIGN(RenderFrameMetadataObserver);
};

}  // namespace cc

#endif  // CC_TREES_RENDER_FRAME_METADATA_OBSERVER_H_
