// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/media/session/media_metadata_sanitizer.h"

#include <algorithm>
#include <string>

#include "content/public/common/media_metadata.h"

namespace content {

namespace {

// Maximum length for all the strings inside the MediaMetadata when it is sent
// over IPC. The renderer process should truncate the strings before sending
// the MediaMetadata and the browser process must do the same when receiving
// it.
const size_t kMaxIPCStringLength = 4 * 1024;

// Maximum type length of MediaImage, which conforms to RFC 4288
// (https://tools.ietf.org/html/rfc4288).
const size_t kMaxMediaImageTypeLength = 2 * 127 + 1;

// Maximum number of MediaImages inside the MediaMetadata.
const size_t kMaxNumberOfMediaImages = 10;

// Maximum of sizes in a MediaImage.
const size_t kMaxNumberOfMediaImageSizes = 10;

bool CheckMediaImageSrcSanity(const GURL& src) {
  if (!src.is_valid())
    return false;
  if (!src.SchemeIsHTTPOrHTTPS() &&
      !src.SchemeIs(url::kDataScheme) &&
      !src.SchemeIs(url::kBlobScheme))
    return false;
  if (src.spec().size() > url::kMaxURLChars)
    return false;

  return true;
}

bool CheckMediaImageSanity(const MediaMetadata::MediaImage& image) {
  if (!CheckMediaImageSrcSanity(image.src))
    return false;
  if (image.type.size() > kMaxMediaImageTypeLength)
    return false;
  if (image.sizes.size() > kMaxNumberOfMediaImageSizes)
    return false;

  return true;
}

// Sanitize MediaImage. The method should not be called if |image.src| is bad.
MediaMetadata::MediaImage SanitizeMediaImage(
    const MediaMetadata::MediaImage& image) {
  MediaMetadata::MediaImage sanitized_image;

  sanitized_image.src = image.src;
  sanitized_image.type = image.type.substr(0, kMaxMediaImageTypeLength);
  for (const auto& size : image.sizes) {
    sanitized_image.sizes.push_back(size);
    if (sanitized_image.sizes.size() == kMaxNumberOfMediaImageSizes)
      break;
  }

  return sanitized_image;
}

}  // anonymous namespace

bool MediaMetadataSanitizer::CheckSanity(const MediaMetadata& metadata) {
  if (metadata.title.size() > kMaxIPCStringLength)
    return false;
  if (metadata.artist.size() > kMaxIPCStringLength)
    return false;
  if (metadata.album.size() > kMaxIPCStringLength)
    return false;
  if (metadata.artwork.size() > kMaxNumberOfMediaImages)
    return false;

  for (const auto& image : metadata.artwork) {
    if (!CheckMediaImageSanity(image))
      return false;
  }

  return true;
}

MediaMetadata MediaMetadataSanitizer::Sanitize(const MediaMetadata& metadata) {
  MediaMetadata sanitized_metadata;

  sanitized_metadata.title = metadata.title.substr(0, kMaxIPCStringLength);
  sanitized_metadata.artist = metadata.artist.substr(0, kMaxIPCStringLength);
  sanitized_metadata.album = metadata.album.substr(0, kMaxIPCStringLength);

  for (const auto& image : metadata.artwork) {
    if (!CheckMediaImageSrcSanity(image.src))
      continue;

    sanitized_metadata.artwork.push_back(
        CheckMediaImageSanity(image) ? image : SanitizeMediaImage(image));

    if (sanitized_metadata.artwork.size() == kMaxNumberOfMediaImages)
      break;
  }

  return sanitized_metadata;
}

}  // namespace content
