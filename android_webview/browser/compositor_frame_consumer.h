// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ANDROID_WEBVIEW_BROWSER_COMPOSITOR_FRAME_CONSUMER_H_
#define ANDROID_WEBVIEW_BROWSER_COMPOSITOR_FRAME_CONSUMER_H_

#include <map>

#include "android_webview/browser/child_frame.h"
#include "android_webview/browser/compositor_id.h"
#include "android_webview/browser/parent_compositor_draw_constraints.h"
#include "components/viz/common/resources/returned_resource.h"
#include "content/public/browser/android/synchronous_compositor.h"
#include "ui/gfx/geometry/vector2d.h"

namespace android_webview {

class ChildFrame;
class CompositorFrameProducer;

class CompositorFrameConsumer {
 public:
  struct ReturnedResources {
    ReturnedResources();
    ~ReturnedResources();

    uint32_t layer_tree_frame_sink_id;
    std::vector<viz::ReturnedResource> resources;
  };
  using ReturnedResourcesMap =
      std::map<CompositorID, ReturnedResources, CompositorIDComparator>;

  // A CompositorFrameConsumer may be registered with at most one
  // CompositorFrameProducer.
  // The producer is responsible for managing the relationship with its
  // consumers. The only exception to this is when a consumer is explicitly
  // destroyed, at which point it needs to inform its producer.
  // In order to register a consumer with a new producer, the current producer
  // must unregister the consumer, and call SetCompositorProducer(nullptr).
  virtual void SetCompositorFrameProducer(
      CompositorFrameProducer* compositor_frame_producer) = 0;
  virtual void SetScrollOffsetOnUI(gfx::Vector2d scroll_offset) = 0;
  // Returns uncommitted frame to be returned, if any.
  virtual std::unique_ptr<ChildFrame> SetFrameOnUI(
      std::unique_ptr<ChildFrame> frame) = 0;
  virtual void InitializeHardwareDrawIfNeededOnUI() = 0;
  virtual ParentCompositorDrawConstraints GetParentDrawConstraintsOnUI()
      const = 0;
  virtual void SwapReturnedResourcesOnUI(
      ReturnedResourcesMap* returned_resource_map) = 0;
  virtual bool ReturnedResourcesEmptyOnUI() const = 0;
  virtual ChildFrameQueue PassUncommittedFrameOnUI() = 0;
  virtual void DeleteHardwareRendererOnUI() = 0;

 protected:
  virtual ~CompositorFrameConsumer() {}
};

}  // namespace android_webview

#endif  // ANDROID_WEBVIEW_BROWSER_COMPOSITOR_FRAME_CONSUMER_H_
