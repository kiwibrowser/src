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

#ifndef THIRD_PARTY_BLINK_PUBLIC_WEB_WEB_IMAGE_DECODER_H_
#define THIRD_PARTY_BLINK_PUBLIC_WEB_WEB_IMAGE_DECODER_H_

#include "third_party/blink/public/platform/web_common.h"
#include "third_party/blink/public/platform/web_image.h"

namespace blink {

class ImageDecoder;
class WebData;
typedef ImageDecoder WebImageDecoderPrivate;

class WebImageDecoder {
 public:
  enum Type { kTypeBMP, kTypeICO };

  ~WebImageDecoder() { Reset(); }

  explicit WebImageDecoder(Type type) { Init(type); }
  WebImageDecoder(const WebImageDecoder&) = delete;
  WebImageDecoder& operator=(const WebImageDecoder&) = delete;

  // Sets data contents for underlying decoder. All the API methods
  // require that setData() is called prior to their use.
  BLINK_EXPORT void SetData(const WebData& data, bool all_data_received);

  // Deletes owned decoder.
  BLINK_EXPORT void Reset();

  // Returns true if image decoding failed.
  BLINK_EXPORT bool IsFailed() const;

  // Returns true if size information is available for the decoder.
  BLINK_EXPORT bool IsSizeAvailable() const;

  // Returns the size of the image.
  BLINK_EXPORT WebSize Size() const;

  // Gives frame count for the image. For multiple frames, decoder scans the
  // image data for the count.
  BLINK_EXPORT size_t FrameCount() const;

  // Returns if the frame at given index is completely decoded.
  BLINK_EXPORT bool IsFrameCompleteAtIndex(int index) const;

  // Creates and returns WebImage from buffer at the index.
  BLINK_EXPORT WebImage GetFrameAtIndex(int index) const;

 private:
  // Creates type-specific decoder.
  BLINK_EXPORT void Init(Type type);

  WebImageDecoderPrivate* private_;
};

}  // namespace blink

#endif
