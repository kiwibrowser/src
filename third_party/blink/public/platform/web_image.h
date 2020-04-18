/*
 * Copyright (C) 2009 Google Inc. All rights reserved.
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

#ifndef THIRD_PARTY_BLINK_PUBLIC_PLATFORM_WEB_IMAGE_H_
#define THIRD_PARTY_BLINK_PUBLIC_PLATFORM_WEB_IMAGE_H_

#include "third_party/blink/public/platform/web_common.h"
#include "third_party/blink/public/platform/web_vector.h"

#include "base/time/time.h"
#include "third_party/skia/include/core/SkBitmap.h"

#if INSIDE_BLINK
#include "base/memory/scoped_refptr.h"
#include "third_party/blink/renderer/platform/graphics/image_orientation.h"
#endif

namespace blink {

class Image;
class WebData;
struct WebSize;

// A container for an ARGB bitmap.
class WebImage {
 public:
  // An image with a duration associated. An animation is a sequence of
  // AnimationFrames played in succession.
  struct AnimationFrame {
    SkBitmap bitmap;
    base::TimeDelta duration;
  };

  ~WebImage() { Reset(); }

  WebImage() { Init(); }
  WebImage(const WebImage& image) {
    Init();
    Assign(image);
  }

  WebImage& operator=(const WebImage& image) {
    Assign(image);
    return *this;
  }

  // Decodes the given image data. If the image has multiple frames,
  // then the frame whose size is desired_size is returned. Otherwise,
  // the first frame is returned.
  BLINK_PLATFORM_EXPORT static WebImage FromData(const WebData&,
                                                 const WebSize& desired_size);

  // Returns a list of all frames in the image. Only the first frame at each
  // pixel size will be returned.
  BLINK_PLATFORM_EXPORT static WebVector<WebImage> FramesFromData(
      const WebData&);

  // Returns a list of all animation frames in the image.
  BLINK_PLATFORM_EXPORT static WebVector<AnimationFrame> AnimationFromData(
      const WebData&);

  BLINK_PLATFORM_EXPORT void Reset();
  BLINK_PLATFORM_EXPORT void Assign(const WebImage&);

  BLINK_PLATFORM_EXPORT bool IsNull() const;
  BLINK_PLATFORM_EXPORT WebSize Size() const;

#if INSIDE_BLINK
  BLINK_PLATFORM_EXPORT WebImage(scoped_refptr<Image>,
                                 RespectImageOrientationEnum = kDoNotRespectImageOrientation);
#endif

  WebImage(const SkBitmap& bitmap) : bitmap_(bitmap) {}

  WebImage& operator=(const SkBitmap& bitmap) {
    bitmap_ = bitmap;
    return *this;
  }

  SkBitmap& GetSkBitmap() { return bitmap_; }
  const SkBitmap& GetSkBitmap() const { return bitmap_; }

 private:
  void Init() {}
  SkBitmap bitmap_;
};

}  // namespace blink

#endif
