// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MEDIA_GPU_ANDROID_VIDEO_FRAME_FACTORY_
#define MEDIA_GPU_ANDROID_VIDEO_FRAME_FACTORY_

#include <memory>

#include "base/memory/ref_counted.h"
#include "base/single_thread_task_runner.h"
#include "base/time/time.h"
#include "media/base/video_decoder.h"
#include "media/gpu/android/promotion_hint_aggregator.h"
#include "media/gpu/media_gpu_export.h"
#include "ui/gfx/geometry/size.h"

namespace gpu {
class CommandBufferStub;
}  // namespace gpu

namespace media {

struct AVDASurfaceBundle;
class CodecOutputBuffer;
class TextureOwner;
class VideoFrame;

// VideoFrameFactory creates CodecOutputBuffer backed VideoFrames. Not thread
// safe. Virtual for testing; see VideoFrameFactoryImpl.
class MEDIA_GPU_EXPORT VideoFrameFactory {
 public:
  using GetStubCb = base::Callback<gpu::CommandBufferStub*()>;
  using InitCb = base::RepeatingCallback<void(scoped_refptr<TextureOwner>)>;

  VideoFrameFactory() = default;
  virtual ~VideoFrameFactory() = default;

  // Initializes the factory and runs |init_cb| on the current thread when it's
  // complete. If initialization fails, the returned texture owner will be
  // null.  |wants_promotion_hint| tells us whether to mark VideoFrames for
  // compositor overlay promotion hints or not.
  virtual void Initialize(bool wants_promotion_hint, InitCb init_cb) = 0;

  // Notify us about the current surface bundle that subsequent video frames
  // should use.
  virtual void SetSurfaceBundle(
      scoped_refptr<AVDASurfaceBundle> surface_bundle) = 0;

  // Creates a new VideoFrame backed by |output_buffer| and |texture_owner|.
  // |texture_owner| may be null if the buffer is backed by an overlay
  // instead. Runs |output_cb| on the calling sequence to return the frame.
  // TODO(liberato): update the comment.
  virtual void CreateVideoFrame(
      std::unique_ptr<CodecOutputBuffer> output_buffer,
      base::TimeDelta timestamp,
      gfx::Size natural_size,
      PromotionHintAggregator::NotifyPromotionHintCB promotion_hint_cb,
      VideoDecoder::OutputCB output_cb) = 0;

  // Runs |closure| on the calling sequence after all previous
  // CreateVideoFrame() calls have completed.
  virtual void RunAfterPendingVideoFrames(base::OnceClosure closure) = 0;
};

}  // namespace media

#endif  // MEDIA_GPU_ANDROID_VIDEO_FRAME_FACTORY_
