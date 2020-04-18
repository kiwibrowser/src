/*
 *  Copyright (c) 2019 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#ifndef TEST_FRAME_CHANGE_EXTRACTOR_H_
#define TEST_FRAME_CHANGE_EXTRACTOR_H_

#include "api/video/video_frame.h"
#include "api/video/video_sink_interface.h"
#include "api/video/video_source_interface.h"
#include "rtc_base/critical_section.h"

namespace webrtc {

// This class is used to test partial video frame interface.
// It receives a full frame, detects changed region, and passes forward only
// the changed part with correct PartialDescription struct set.
// Currently only works with I420 buffers.
class FrameChangeExtractor : public rtc::VideoSinkInterface<VideoFrame>,
                             public rtc::VideoSourceInterface<VideoFrame> {
 public:
  FrameChangeExtractor();
  ~FrameChangeExtractor();
  void SetSource(rtc::VideoSourceInterface<VideoFrame>* video_source);

  // Implements rtc::VideoSinkInterface<VideoFrame>.
  void OnFrame(const VideoFrame& frame) override;

  // Implements rtc::VideoSourceInterface<VideoFrame>.
  void AddOrUpdateSink(rtc::VideoSinkInterface<VideoFrame>* sink,
                       const rtc::VideoSinkWants& wants) override;
  void RemoveSink(rtc::VideoSinkInterface<VideoFrame>* sink) override;

 private:
  rtc::CriticalSection source_crit_;
  rtc::VideoSourceInterface<VideoFrame>* source_ RTC_GUARDED_BY(source_crit_);
  rtc::CriticalSection sink_crit_;
  rtc::VideoSinkInterface<VideoFrame>* sink_ RTC_GUARDED_BY(sink_crit_);
  rtc::scoped_refptr<webrtc::I420BufferInterface> last_frame_buffer_
      RTC_GUARDED_BY(sink_crit_);
};

}  // namespace webrtc
#endif  // TEST_FRAME_CHANGE_EXTRACTOR_H_
