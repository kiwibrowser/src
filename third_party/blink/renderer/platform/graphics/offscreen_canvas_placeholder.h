// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_PLATFORM_GRAPHICS_OFFSCREEN_CANVAS_PLACEHOLDER_H_
#define THIRD_PARTY_BLINK_RENDERER_PLATFORM_GRAPHICS_OFFSCREEN_CANVAS_PLACEHOLDER_H_

#include <memory>
#include "base/memory/scoped_refptr.h"
#include "base/memory/weak_ptr.h"
#include "base/single_thread_task_runner.h"
#include "components/viz/common/resources/resource_id.h"
#include "third_party/blink/renderer/platform/platform_export.h"

namespace blink {

class OffscreenCanvasFrameDispatcher;
class StaticBitmapImage;

class PLATFORM_EXPORT OffscreenCanvasPlaceholder {
 public:
  ~OffscreenCanvasPlaceholder();

  virtual void SetPlaceholderFrame(
      scoped_refptr<StaticBitmapImage>,
      base::WeakPtr<OffscreenCanvasFrameDispatcher>,
      scoped_refptr<base::SingleThreadTaskRunner>,
      viz::ResourceId resource_id);
  void ReleasePlaceholderFrame();

  void SetSuspendOffscreenCanvasAnimation(bool);

  static OffscreenCanvasPlaceholder* GetPlaceholderById(
      unsigned placeholder_id);

  void RegisterPlaceholder(unsigned placeholder_id);
  void UnregisterPlaceholder();
  const scoped_refptr<StaticBitmapImage>& PlaceholderFrame() const {
    return placeholder_frame_;
  }

  bool IsPlaceholderRegistered() const {
    return placeholder_id_ != kNoPlaceholderId;
  }

 private:
  bool PostSetSuspendAnimationToOffscreenCanvasThread(bool suspend);

  scoped_refptr<StaticBitmapImage> placeholder_frame_;
  base::WeakPtr<OffscreenCanvasFrameDispatcher> frame_dispatcher_;
  scoped_refptr<base::SingleThreadTaskRunner> frame_dispatcher_task_runner_;
  viz::ResourceId placeholder_frame_resource_id_ = 0;

  enum {
    kNoPlaceholderId = -1,
  };
  int placeholder_id_ = kNoPlaceholderId;

  enum AnimationState {
    kActiveAnimation,
    kSuspendedAnimation,
    kShouldSuspendAnimation,
    kShouldActivateAnimation,
  };
  AnimationState animation_state_ = kActiveAnimation;
};

}  // blink

#endif
