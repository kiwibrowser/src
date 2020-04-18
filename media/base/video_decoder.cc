// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "media/base/video_decoder.h"

#include "media/base/video_frame.h"

namespace media {

VideoDecoder::VideoDecoder() = default;

void VideoDecoder::Destroy() {
  delete this;
}

VideoDecoder::~VideoDecoder() = default;

bool VideoDecoder::NeedsBitstreamConversion() const {
  return false;
}

bool VideoDecoder::CanReadWithoutStalling() const {
  return true;
}

int VideoDecoder::GetMaxDecodeRequests() const {
  return 1;
}

}  // namespace media

namespace std {

void default_delete<media::VideoDecoder>::operator()(
    media::VideoDecoder* ptr) const {
  ptr->Destroy();
}

}  // namespace std
