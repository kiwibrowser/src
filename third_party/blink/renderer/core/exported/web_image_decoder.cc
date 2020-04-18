/*
 * Copyright (C) 2010 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *     * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "third_party/blink/public/web/web_image_decoder.h"

#include "base/memory/scoped_refptr.h"
#include "third_party/blink/public/platform/platform.h"
#include "third_party/blink/public/platform/web_data.h"
#include "third_party/blink/public/platform/web_image.h"
#include "third_party/blink/public/platform/web_size.h"
#include "third_party/blink/renderer/platform/image-decoders/bmp/bmp_image_decoder.h"
#include "third_party/blink/renderer/platform/image-decoders/ico/ico_image_decoder.h"
#include "third_party/blink/renderer/platform/shared_buffer.h"

namespace blink {

void WebImageDecoder::Reset() {
  delete private_;
}

void WebImageDecoder::Init(Type type) {
  size_t max_decoded_bytes = Platform::Current()->MaxDecodedImageBytes();

  switch (type) {
    case kTypeBMP:
      private_ = new BMPImageDecoder(ImageDecoder::kAlphaPremultiplied,
                                     ColorBehavior::TransformToSRGB(),
                                     max_decoded_bytes);
      break;
    case kTypeICO:
      private_ = new ICOImageDecoder(ImageDecoder::kAlphaPremultiplied,
                                     ColorBehavior::TransformToSRGB(),
                                     max_decoded_bytes);
      break;
  }
}

void WebImageDecoder::SetData(const WebData& data, bool all_data_received) {
  DCHECK(private_);
  private_->SetData(data, all_data_received);
}

bool WebImageDecoder::IsFailed() const {
  DCHECK(private_);
  return private_->Failed();
}

bool WebImageDecoder::IsSizeAvailable() const {
  DCHECK(private_);
  return private_->IsSizeAvailable();
}

WebSize WebImageDecoder::Size() const {
  DCHECK(private_);
  return private_->Size();
}

size_t WebImageDecoder::FrameCount() const {
  DCHECK(private_);
  return private_->FrameCount();
}

bool WebImageDecoder::IsFrameCompleteAtIndex(int index) const {
  DCHECK(private_);
  ImageFrame* const frame_buffer = private_->DecodeFrameBufferAtIndex(index);
  if (!frame_buffer)
    return false;
  return frame_buffer->GetStatus() == ImageFrame::kFrameComplete;
}

WebImage WebImageDecoder::GetFrameAtIndex(int index = 0) const {
  DCHECK(private_);
  ImageFrame* const frame_buffer = private_->DecodeFrameBufferAtIndex(index);
  if (!frame_buffer)
    return WebImage();
  return WebImage(frame_buffer->Bitmap());
}

}  // namespace blink
