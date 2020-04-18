// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_COMMON_MEDIA_MEDIA_DEVICES_TYPEMAP_TRAITS_H_
#define CONTENT_COMMON_MEDIA_MEDIA_DEVICES_TYPEMAP_TRAITS_H_

#include "content/common/media/media_devices.h"
#include "third_party/blink/public/platform/modules/mediastream/media_devices.mojom.h"

namespace mojo {

template <>
struct EnumTraits<blink::mojom::MediaDeviceType, content::MediaDeviceType> {
  static blink::mojom::MediaDeviceType ToMojom(content::MediaDeviceType type);

  static bool FromMojom(blink::mojom::MediaDeviceType input,
                        content::MediaDeviceType* out);
};

template <>
struct EnumTraits<blink::mojom::FacingMode, media::VideoFacingMode> {
  static blink::mojom::FacingMode ToMojom(media::VideoFacingMode facing_mode);

  static bool FromMojom(blink::mojom::FacingMode input,
                        media::VideoFacingMode* out);
};

template <>
struct StructTraits<blink::mojom::MediaDeviceInfoDataView,
                    content::MediaDeviceInfo> {
  static const std::string& device_id(const content::MediaDeviceInfo& info) {
    return info.device_id;
  }

  static const std::string& label(const content::MediaDeviceInfo& info) {
    return info.label;
  }

  static const std::string& group_id(const content::MediaDeviceInfo& info) {
    return info.group_id;
  }

  static bool Read(blink::mojom::MediaDeviceInfoDataView input,
                   content::MediaDeviceInfo* out);
};

}  // namespace mojo

#endif  // CONTENT_COMMON_MEDIA_MEDIA_DEVICES_TYPEMAP_TRAITS_H_