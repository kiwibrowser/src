// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "media/base/demuxer_memory_limit.h"

namespace media {

size_t GetDemuxerStreamAudioMemoryLimit() {
  return internal::kDemuxerStreamAudioMemoryLimitDefault;
}

size_t GetDemuxerStreamVideoMemoryLimit() {
  return internal::kDemuxerStreamVideoMemoryLimitDefault;
}

size_t GetDemuxerMemoryLimit() {
  return GetDemuxerStreamAudioMemoryLimit() +
         GetDemuxerStreamVideoMemoryLimit();
}

}  // namespace media
