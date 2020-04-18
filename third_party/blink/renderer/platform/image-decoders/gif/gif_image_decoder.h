/*
 * Copyright (C) 2006 Apple Computer, Inc.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE COMPUTER, INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE COMPUTER, INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef THIRD_PARTY_BLINK_RENDERER_PLATFORM_IMAGE_DECODERS_GIF_GIF_IMAGE_DECODER_H_
#define THIRD_PARTY_BLINK_RENDERER_PLATFORM_IMAGE_DECODERS_GIF_GIF_IMAGE_DECODER_H_

#include <memory>
#include "third_party/blink/renderer/platform/image-decoders/image_decoder.h"
#include "third_party/blink/renderer/platform/wtf/noncopyable.h"
#include "third_party/blink/renderer/platform/wtf/time.h"

namespace blink {

class GIFImageReader;

using GIFRow = Vector<unsigned char>;

// This class decodes the GIF image format.
class PLATFORM_EXPORT GIFImageDecoder final : public ImageDecoder {
  WTF_MAKE_NONCOPYABLE(GIFImageDecoder);

 public:
  GIFImageDecoder(AlphaOption, const ColorBehavior&, size_t max_decoded_bytes);
  ~GIFImageDecoder() override;

  enum GIFParseQuery { kGIFSizeQuery, kGIFFrameCountQuery };

  // ImageDecoder:
  String FilenameExtension() const override { return "gif"; }
  void OnSetData(SegmentReader* data) override;
  int RepetitionCount() const override;
  bool FrameIsReceivedAtIndex(size_t) const override;
  TimeDelta FrameDurationAtIndex(size_t) const override;
  // CAUTION: SetFailed() deletes |reader_|.  Be careful to avoid
  // accessing deleted memory, especially when calling this from inside
  // GIFImageReader!
  bool SetFailed() override;

  // Callbacks from the GIF reader.
  bool HaveDecodedRow(size_t frame_index,
                      GIFRow::const_iterator row_begin,
                      size_t width,
                      size_t row_number,
                      unsigned repeat_count,
                      bool write_transparent_pixels);
  bool FrameComplete(size_t frame_index);

  // For testing.
  bool ParseCompleted() const;

 private:
  // ImageDecoder:
  void ClearFrameBuffer(size_t frame_index) override;
  void DecodeSize() override { Parse(kGIFSizeQuery); }
  size_t DecodeFrameCount() override;
  void InitializeNewFrame(size_t) override;
  void Decode(size_t) override;

  // Parses as much as is needed to answer the query, ignoring bitmap
  // data. If parsing fails, sets the "decode failure" flag.
  void Parse(GIFParseQuery);

  // Reset the alpha tracker for this frame. Before calling this method, the
  // caller must verify that the frame exists.
  void OnInitFrameBuffer(size_t) override;

  // When the disposal method of the frame is DisposeOverWritePrevious, the
  // next frame will use the previous frame's buffer as its starting state, so
  // we can't take over the data in that case. Before calling this method, the
  // caller must verify that the frame exists.
  bool CanReusePreviousFrameBuffer(size_t) const override;

  bool current_buffer_saw_alpha_;
  mutable int repetition_count_;
  std::unique_ptr<GIFImageReader> reader_;
};

}  // namespace blink

#endif
