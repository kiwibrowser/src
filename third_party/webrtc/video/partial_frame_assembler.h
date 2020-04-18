/*
 *  Copyright (c) 2019 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#ifndef VIDEO_PARTIAL_FRAME_ASSEMBLER_H_
#define VIDEO_PARTIAL_FRAME_ASSEMBLER_H_

#include "api/video/i420_buffer.h"
#include "api/video/video_frame.h"

namespace webrtc {

// Maintains cache of a full resolution frame buffer and applies partial
// updates to it.
// This class is not thread-safe.
class PartialFrameAssembler {
 public:
  PartialFrameAssembler();
  ~PartialFrameAssembler();

  // Applies |input_buffer| to the cached buffer and sets buffer for
  // |uncompresed_frame| to a full updated image.
  // Returns false on any error. In that case the buffer will be invalidated
  // and subsequent updates will also return error until full resolution frame
  // is processed.
  bool ApplyPartialUpdate(
      const rtc::scoped_refptr<VideoFrameBuffer>& buffer,
      VideoFrame* uncompressed_frame,
      const VideoFrame::PartialFrameDescription* partial_desc);

  // Clears internal buffer.
  void Reset();

 private:
  rtc::scoped_refptr<I420Buffer> cached_frame_buffer_;
};

}  // namespace webrtc
#endif  // VIDEO_PARTIAL_FRAME_ASSEMBLER_H_
