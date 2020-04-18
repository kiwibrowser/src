// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_COMMON_MEDIA_MEDIA_SESSION_STRUCT_TRAITS_H_
#define CONTENT_COMMON_MEDIA_MEDIA_SESSION_STRUCT_TRAITS_H_

#include "third_party/blink/public/platform/modules/mediasession/media_session.mojom.h"

namespace mojo {

template <>
struct StructTraits<blink::mojom::MediaImageDataView,
                    content::MediaMetadata::MediaImage> {
  static const GURL& src(const content::MediaMetadata::MediaImage& image) {
    return image.src;
  }

  static const base::string16& type(
      const content::MediaMetadata::MediaImage& image) {
    return image.type;
  }

  static const std::vector<gfx::Size>& sizes(
      const content::MediaMetadata::MediaImage& image) {
    return image.sizes;
  }

  static bool Read(blink::mojom::MediaImageDataView data,
                   content::MediaMetadata::MediaImage* out) {
    if (!data.ReadSrc(&out->src))
      return false;
    if (!data.ReadType(&out->type))
      return false;
    if (!data.ReadSizes(&out->sizes))
      return false;

    return true;
  }
};

template <>
struct StructTraits<blink::mojom::MediaMetadataDataView,
                    content::MediaMetadata> {
  static const base::string16& title(const content::MediaMetadata& metadata) {
    return metadata.title;
  }

  static const base::string16& artist(const content::MediaMetadata& metadata) {
    return metadata.artist;
  }

  static const base::string16& album(const content::MediaMetadata& metadata) {
    return metadata.album;
  }

  static const std::vector<content::MediaMetadata::MediaImage>& artwork(
      const content::MediaMetadata& metadata) {
    return metadata.artwork;
  }

  static bool Read(blink::mojom::MediaMetadataDataView data,
                   content::MediaMetadata* out) {
    if (!data.ReadTitle(&out->title))
      return false;
    if (!data.ReadArtist(&out->artist))
      return false;
    if (!data.ReadAlbum(&out->album))
      return false;
    if (!data.ReadArtwork(&out->artwork))
      return false;

    return true;
  }
};

}  // namespace mojo

#endif  // CONTENT_COMMON_MEDIA_MEDIA_SESSION_STRUCT_TRAITS_H_
