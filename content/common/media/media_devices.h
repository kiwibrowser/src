// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_COMMON_MEDIA_MEDIA_DEVICES_H_
#define CONTENT_COMMON_MEDIA_MEDIA_DEVICES_H_

#include <string>
#include <vector>

#include "content/common/content_export.h"
#include "media/base/video_facing.h"

namespace media {
struct AudioDeviceDescription;
struct VideoCaptureDeviceDescriptor;
}  // namespace media

namespace content {

enum MediaDeviceType {
  MEDIA_DEVICE_TYPE_AUDIO_INPUT,
  MEDIA_DEVICE_TYPE_VIDEO_INPUT,
  MEDIA_DEVICE_TYPE_AUDIO_OUTPUT,
  NUM_MEDIA_DEVICE_TYPES,
};

struct CONTENT_EXPORT MediaDeviceInfo {
  MediaDeviceInfo();
  MediaDeviceInfo(const MediaDeviceInfo& other);
  MediaDeviceInfo(MediaDeviceInfo&& other);
  MediaDeviceInfo(
      const std::string& device_id,
      const std::string& label,
      const std::string& group_id,
      media::VideoFacingMode video_facing = media::MEDIA_VIDEO_FACING_NONE);
  explicit MediaDeviceInfo(const media::AudioDeviceDescription& description);
  explicit MediaDeviceInfo(
      const media::VideoCaptureDeviceDescriptor& descriptor);
  ~MediaDeviceInfo();
  MediaDeviceInfo& operator=(const MediaDeviceInfo& other);
  MediaDeviceInfo& operator=(MediaDeviceInfo&& other);

  std::string device_id;
  std::string label;
  std::string group_id;
  media::VideoFacingMode video_facing;
};

using MediaDeviceInfoArray = std::vector<MediaDeviceInfo>;

bool operator==(const MediaDeviceInfo& first, const MediaDeviceInfo& second);

inline bool IsValidMediaDeviceType(MediaDeviceType type) {
  return type >= 0 && type < NUM_MEDIA_DEVICE_TYPES;
}

}  // namespace content

#endif  // CONTENT_COMMON_MEDIA_MEDIA_DEVICES_H_
