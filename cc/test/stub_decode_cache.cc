// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "cc/test/stub_decode_cache.h"

namespace cc {

ImageDecodeCache::TaskResult StubDecodeCache::GetTaskForImageAndRef(
    const DrawImage& image,
    const TracingInfo& tracing_info) {
  return TaskResult(true);
}

ImageDecodeCache::TaskResult
StubDecodeCache::GetOutOfRasterDecodeTaskForImageAndRef(
    const DrawImage& image) {
  return TaskResult(true);
}

DecodedDrawImage StubDecodeCache::GetDecodedImageForDraw(
    const DrawImage& image) {
  return DecodedDrawImage();
}

size_t StubDecodeCache::GetMaximumMemoryLimitBytes() const {
  return 0u;
}

}  // namespace cc
