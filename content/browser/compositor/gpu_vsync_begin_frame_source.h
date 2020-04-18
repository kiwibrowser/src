// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_COMPOSITOR_GPU_VSYNC_BEGIN_FRAME_SOURCE_H_
#define CONTENT_BROWSER_COMPOSITOR_GPU_VSYNC_BEGIN_FRAME_SOURCE_H_

#include "base/memory/weak_ptr.h"
#include "base/time/time.h"
#include "components/viz/common/frame_sinks/begin_frame_source.h"
#include "content/common/content_export.h"

namespace content {

// This class is used to control VSync production on GPU side.
class CONTENT_EXPORT GpuVSyncControl {
 public:
  virtual void SetNeedsVSync(bool needs_vsync) = 0;
};

// This is a type of ExternalBeginFrameSource where VSync signals are
// generated externally on GPU side.
class CONTENT_EXPORT GpuVSyncBeginFrameSource
    : public viz::ExternalBeginFrameSource,
      public viz::ExternalBeginFrameSourceClient {
 public:
  explicit GpuVSyncBeginFrameSource(GpuVSyncControl* vsync_control);
  ~GpuVSyncBeginFrameSource() override;

  // viz::ExternalBeginFrameSourceClient implementation.
  void OnNeedsBeginFrames(bool needs_begin_frames) override;

  void OnVSync(base::TimeTicks timestamp, base::TimeDelta interval);

 protected:
  // Virtual for testing.
  virtual base::TimeTicks Now() const;

 private:
  viz::BeginFrameArgs GetMissedBeginFrameArgs(
      viz::BeginFrameObserver* obs) override;

  GpuVSyncControl* const vsync_control_;
  bool needs_begin_frames_;
  uint64_t next_sequence_number_;

  DISALLOW_COPY_AND_ASSIGN(GpuVSyncBeginFrameSource);
};

}  // namespace content

#endif  // CONTENT_BROWSER_COMPOSITOR_GPU_VSYNC_BEGIN_FRAME_SOURCE_H_
