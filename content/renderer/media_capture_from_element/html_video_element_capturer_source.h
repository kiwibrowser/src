// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_RENDERER_MEDIA_CAPTURE_FROM_ELEMENT_HTML_VIDEO_ELEMENT_CAPTURER_SOURCE_H_
#define CONTENT_RENDERER_MEDIA_CAPTURE_FROM_ELEMENT_HTML_VIDEO_ELEMENT_CAPTURER_SOURCE_H_

#include "base/callback.h"
#include "base/memory/weak_ptr.h"
#include "base/threading/thread_checker.h"
#include "base/time/time.h"
#include "content/common/content_export.h"
#include "media/base/video_frame_pool.h"
#include "media/base/video_types.h"
#include "media/capture/video_capturer_source.h"
#include "third_party/blink/public/platform/web_size.h"
#include "third_party/skia/include/core/SkBitmap.h"
#include "third_party/skia/include/core/SkRefCnt.h"

namespace base{
class SingleThreadTaskRunner;
}

namespace blink {
class WebMediaPlayer;
}  // namespace blink

namespace cc {
class PaintCanvas;
}  // namespace cc

namespace content {

// This class is a VideoCapturerSource taking video snapshots of the ctor-passed
// blink::WebMediaPlayer on Render Main thread. The captured data is converted
// and sent back to |io_task_runner_| via the registered |new_frame_callback_|.
class CONTENT_EXPORT HtmlVideoElementCapturerSource final
    : public media::VideoCapturerSource {
 public:
  static std::unique_ptr<HtmlVideoElementCapturerSource>
  CreateFromWebMediaPlayerImpl(
      blink::WebMediaPlayer* player,
      const scoped_refptr<base::SingleThreadTaskRunner>& io_task_runner,
      scoped_refptr<base::SingleThreadTaskRunner> task_runner);

  HtmlVideoElementCapturerSource(
      const base::WeakPtr<blink::WebMediaPlayer>& player,
      const scoped_refptr<base::SingleThreadTaskRunner>& io_task_runner,
      scoped_refptr<base::SingleThreadTaskRunner> task_runner);
  ~HtmlVideoElementCapturerSource() override;

  // media::VideoCapturerSource Implementation.
  media::VideoCaptureFormats GetPreferredFormats() override;
  void StartCapture(const media::VideoCaptureParams& params,
                    const VideoCaptureDeliverFrameCB& new_frame_callback,
                    const RunningCallback& running_callback) override;
  void StopCapture() override;

 private:
  // This method includes collecting data from the WebMediaPlayer and converting
  // it into a format suitable for MediaStreams.
  void sendNewFrame();

  media::VideoFramePool frame_pool_;
  SkBitmap bitmap_;
  std::unique_ptr<cc::PaintCanvas> canvas_;

  const base::WeakPtr<blink::WebMediaPlayer> web_media_player_;
  const scoped_refptr<base::SingleThreadTaskRunner> io_task_runner_;
  const scoped_refptr<base::SingleThreadTaskRunner> task_runner_;

  // These three configuration items are passed on StartCapture();
  RunningCallback running_callback_;
  VideoCaptureDeliverFrameCB new_frame_callback_;
  double capture_frame_rate_;

  // TimeTicks on which the first captured VideoFrame is produced.
  base::TimeTicks start_capture_time_;

  // Target time for the next frame.
  base::TimeTicks next_capture_time_;

  // Bound to the main render thread.
  base::ThreadChecker thread_checker_;

  // Used on main render thread to schedule future capture events.
  base::WeakPtrFactory<HtmlVideoElementCapturerSource> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(HtmlVideoElementCapturerSource);
};

}  // namespace content

#endif  // CONTENT_RENDERER_MEDIA_CAPTURE_FROM_ELEMENT_HTML_VIDEO_ELEMENT_CAPTURER_SOURCE_H_
