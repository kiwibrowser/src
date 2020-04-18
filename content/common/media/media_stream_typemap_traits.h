// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_COMMON_MEDIA_MEDIA_STREAM_TYPEMAP_TRAITS_H_
#define CONTENT_COMMON_MEDIA_MEDIA_STREAM_TYPEMAP_TRAITS_H_

#include "content/common/media/media_stream.mojom.h"
#include "content/common/media/media_stream_controls.h"
#include "content/public/common/media_stream_request.h"

namespace mojo {

template <>
struct EnumTraits<content::mojom::MediaStreamType, content::MediaStreamType> {
  static content::mojom::MediaStreamType ToMojom(content::MediaStreamType type);

  static bool FromMojom(content::mojom::MediaStreamType input,
                        content::MediaStreamType* out);
};

template <>
struct EnumTraits<content::mojom::MediaStreamRequestResult,
                  content::MediaStreamRequestResult> {
  static content::mojom::MediaStreamRequestResult ToMojom(
      content::MediaStreamRequestResult result);

  static bool FromMojom(content::mojom::MediaStreamRequestResult input,
                        content::MediaStreamRequestResult* out);
};

template <>
struct StructTraits<content::mojom::TrackControlsDataView,
                    content::TrackControls> {
  static bool requested(const content::TrackControls& controls) {
    return controls.requested;
  }

  static const std::string& stream_source(
      const content::TrackControls& controls) {
    return controls.stream_source;
  }

  static const std::string& device_id(const content::TrackControls& controls) {
    return controls.device_id;
  }

  static bool Read(content::mojom::TrackControlsDataView input,
                   content::TrackControls* out);
};

template <>
struct StructTraits<content::mojom::StreamControlsDataView,
                    content::StreamControls> {
  static const content::TrackControls& audio(
      const content::StreamControls& controls) {
    return controls.audio;
  }

  static const content::TrackControls& video(
      const content::StreamControls& controls) {
    return controls.video;
  }

  static bool hotword_enabled(const content::StreamControls& controls) {
    return controls.hotword_enabled;
  }

  static bool disable_local_echo(const content::StreamControls& controls) {
    return controls.disable_local_echo;
  }

  static bool Read(content::mojom::StreamControlsDataView input,
                   content::StreamControls* out);
};

}  // namespace mojo

#endif  // CONTENT_COMMON_MEDIA_MEDIA_STREAM_TYPEMAP_TRAITS_H_