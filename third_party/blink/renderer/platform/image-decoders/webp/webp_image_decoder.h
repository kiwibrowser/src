/*
 * Copyright (C) 2010 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 * 3.  Neither the name of Apple Computer, Inc. ("Apple") nor the names of
 *     its contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE AND ITS CONTRIBUTORS "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef THIRD_PARTY_BLINK_RENDERER_PLATFORM_IMAGE_DECODERS_WEBP_WEBP_IMAGE_DECODER_H_
#define THIRD_PARTY_BLINK_RENDERER_PLATFORM_IMAGE_DECODERS_WEBP_WEBP_IMAGE_DECODER_H_

#include "third_party/blink/renderer/platform/image-decoders/image_decoder.h"
#include "third_party/blink/renderer/platform/wtf/time.h"
#include "third_party/blink/renderer/platform/wtf/vector.h"
#include "webp/decode.h"
#include "webp/demux.h"

class SkData;

namespace blink {

class PLATFORM_EXPORT WEBPImageDecoder final : public ImageDecoder {
  WTF_MAKE_NONCOPYABLE(WEBPImageDecoder);

 public:
  WEBPImageDecoder(AlphaOption, const ColorBehavior&, size_t max_decoded_bytes);
  ~WEBPImageDecoder() override;

  // ImageDecoder:
  String FilenameExtension() const override { return "webp"; }
  void OnSetData(SegmentReader* data) override;
  int RepetitionCount() const override;
  bool FrameIsReceivedAtIndex(size_t) const override;
  TimeDelta FrameDurationAtIndex(size_t) const override;

 private:
  // ImageDecoder:
  void DecodeSize() override { UpdateDemuxer(); }
  size_t DecodeFrameCount() override;
  void InitializeNewFrame(size_t) override;
  void Decode(size_t) override;

  bool DecodeSingleFrame(const uint8_t* data_bytes,
                         size_t data_size,
                         size_t frame_index);

  // For WebP images, the frame status needs to be FrameComplete to decode
  // subsequent frames that depend on frame |index|. The reason for this is that
  // WebP uses the previous frame for alpha blending, in ApplyPostProcessing().
  //
  // Before calling this, verify that frame |index| exists by checking that
  // |index| is smaller than |frame_buffer_cache_|.size().
  bool FrameStatusSufficientForSuccessors(size_t index) override {
    DCHECK(index < frame_buffer_cache_.size());
    return frame_buffer_cache_[index].GetStatus() == ImageFrame::kFrameComplete;
  }

  WebPIDecoder* decoder_;
  WebPDecBuffer decoder_buffer_;
  int format_flags_;
  bool frame_background_has_alpha_;

  void ReadColorProfile();
  bool UpdateDemuxer();

  // Set |frame_background_has_alpha_| based on this frame's characteristics.
  // Before calling this method, the caller must verify that the frame exists.
  void OnInitFrameBuffer(size_t frame_index) override;

  // When the blending method of this frame is BlendAtopPreviousFrame, the
  // previous frame's buffer is necessary to decode this frame in
  // ApplyPostProcessing, so we can't take over the data. Before calling this
  // method, the caller must verify that the frame exists.
  bool CanReusePreviousFrameBuffer(size_t frame_index) const override;

  void ApplyPostProcessing(size_t frame_index);
  void ClearFrameBuffer(size_t frame_index) override;

  WebPDemuxer* demux_;
  WebPDemuxState demux_state_;
  bool have_already_parsed_this_data_;
  int repetition_count_;
  int decoded_height_;

  typedef void (*AlphaBlendFunction)(ImageFrame&, ImageFrame&, int, int, int);
  AlphaBlendFunction blend_function_;

  void Clear();
  void ClearDecoder();

  // This will point to one of three things:
  // - the SegmentReader's data, if contiguous.
  // - its own copy, if not, and all data was received initially.
  // - |buffer_|, if streaming.
  sk_sp<SkData> consolidated_data_;
  Vector<char> buffer_;
};

}  // namespace blink

#endif
