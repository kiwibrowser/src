// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MEDIA_GPU_ANDROID_AVDA_STATE_PROVIDER_H_
#define MEDIA_GPU_ANDROID_AVDA_STATE_PROVIDER_H_

#include "base/compiler_specific.h"
#include "base/threading/thread_checker.h"
#include "gpu/command_buffer/service/texture_manager.h"
#include "media/gpu/android/promotion_hint_aggregator.h"
#include "media/video/video_decode_accelerator.h"

namespace gpu {
namespace gles2 {
class ContextGroup;
}
}  // namespace gpu

namespace media {

// Helper class that provides AVDAPictureBufferManager with enough state
// to do useful work.
class AVDAStateProvider {
 public:
  // Various handy getters.
  virtual const gfx::Size& GetSize() const = 0;
  virtual gpu::gles2::ContextGroup* GetContextGroup() const = 0;

  // Report a fatal error. This will post NotifyError(), and transition to the
  // error state.
  virtual void NotifyError(VideoDecodeAccelerator::Error error) = 0;

  // Return a callback that may be used to signal promotion hint info.
  virtual PromotionHintAggregator::NotifyPromotionHintCB
  GetPromotionHintCB() = 0;

 protected:
  ~AVDAStateProvider() = default;
};

}  // namespace media

#endif  // MEDIA_GPU_ANDROID_AVDA_STATE_PROVIDER_H_
