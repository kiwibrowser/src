// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_PUBLIC_COMMON_MEDIA_METADATA_H_
#define CONTENT_PUBLIC_COMMON_MEDIA_METADATA_H_

#include <vector>

#include "base/strings/string16.h"
#include "content/common/content_export.h"
#include "ui/gfx/geometry/size.h"
#include "url/gurl.h"

namespace content {

// The MediaMetadata is a structure carrying information associated to a
// content::MediaSession.
struct CONTENT_EXPORT MediaMetadata {
  // Structure representing an MediaImage as per the MediaSession API, see:
  // https://wicg.github.io/mediasession/#dictdef-mediaimage
  struct CONTENT_EXPORT MediaImage {
    MediaImage();
    MediaImage(const MediaImage& other);
    ~MediaImage();

    bool operator==(const MediaImage& other) const;

    // MUST be a valid url. If an icon doesn't have a valid URL, it will not be
    // successfully parsed, thus will not be represented in the Manifest.
    GURL src;

    // Empty if the parsing failed or the field was not present. The type can be
    // any string and doesn't have to be a valid image MIME type at this point.
    // It is up to the consumer of the object to check if the type matches a
    // supported type.
    base::string16 type;

    // Empty if the parsing failed, the field was not present or empty.
    // The special value "any" is represented by gfx::Size(0, 0).
    std::vector<gfx::Size> sizes;
  };

  MediaMetadata();
  ~MediaMetadata();

  MediaMetadata(const MediaMetadata& other);

  bool operator==(const MediaMetadata& other) const;
  bool operator!=(const MediaMetadata& other) const;

  // Title associated to the MediaSession.
  base::string16 title;

  // Artist associated to the MediaSession.
  base::string16 artist;

  // Album associated to the MediaSession.
  base::string16 album;

  // Artwork associated to the MediaSession.
  std::vector<MediaImage> artwork;
};

}  // namespace content

#endif // CONTENT_PUBLIC_COMMON_MEDIA_METADATA_H_
