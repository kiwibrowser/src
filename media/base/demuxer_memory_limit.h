// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MEDIA_BASE_DEMUXER_MEMORY_LIMIT_H_
#define MEDIA_BASE_DEMUXER_MEMORY_LIMIT_H_

#include <stddef.h>

#include "build/build_config.h"
#include "media/base/media_export.h"

namespace media {

// The maximum amount of data (in bytes) a demuxer can keep in memory, for a
// particular type of stream.
MEDIA_EXPORT size_t GetDemuxerStreamAudioMemoryLimit();
MEDIA_EXPORT size_t GetDemuxerStreamVideoMemoryLimit();

// The maximum amount of data (in bytes) a demuxer can keep in memory overall.
MEDIA_EXPORT size_t GetDemuxerMemoryLimit();

namespace internal {

// These values should not be used directly, they are selected by functions
// above based on platform capabilities.

// Default audio memory limit: 12MB (5 minutes of 320Kbps content).
// Low audio memory limit: 2MB (1 minute of 256Kbps content).
constexpr size_t kDemuxerStreamAudioMemoryLimitDefault = 12 * 1024 * 1024;
constexpr size_t kDemuxerStreamAudioMemoryLimitLow = 2 * 1024 * 1024;

// Default video memory limit: 150MB (5 minutes of 4Mbps content).
// Low video memory limit: 30MB (1 minute of 4Mbps content).
constexpr size_t kDemuxerStreamVideoMemoryLimitDefault = 150 * 1024 * 1024;
constexpr size_t kDemuxerStreamVideoMemoryLimitLow = 30 * 1024 * 1024;

#if defined(OS_ANDROID)
// Special "very low" settings for 512MiB Android Go devices:
// * audio memory limit: 1MB (30 seconds of 256Kbps content).
// * video memory limit: 15MB (30 seconds of 4Mbps content).
constexpr size_t kDemuxerStreamAudioMemoryLimitVeryLow = 1 * 1024 * 1024;
constexpr size_t kDemuxerStreamVideoMemoryLimitVeryLow = 15 * 1024 * 1024;
#endif

}  // namespace internal

}  // namespace media

#endif  // MEDIA_BASE_DEMUXER_MEMORY_LIMIT_H_
