// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/platform/testing/empty_web_media_player.h"

#include "third_party/blink/public/platform/web_size.h"
#include "third_party/blink/public/platform/web_time_range.h"

namespace blink {

WebTimeRanges EmptyWebMediaPlayer::Buffered() const {
  return WebTimeRanges();
}

WebTimeRanges EmptyWebMediaPlayer::Seekable() const {
  return WebTimeRanges();
}

WebSize EmptyWebMediaPlayer::NaturalSize() const {
  return WebSize(0, 0);
}

WebSize EmptyWebMediaPlayer::VisibleRect() const {
  return WebSize();
}

WebString EmptyWebMediaPlayer::GetErrorMessage() const {
  return WebString();
}

}  // namespace blink
