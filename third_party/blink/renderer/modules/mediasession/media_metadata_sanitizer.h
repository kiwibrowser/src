// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_MODULES_MEDIASESSION_MEDIA_METADATA_SANITIZER_H_
#define THIRD_PARTY_BLINK_RENDERER_MODULES_MEDIASESSION_MEDIA_METADATA_SANITIZER_H_

#include "third_party/blink/public/platform/modules/mediasession/media_session.mojom-blink.h"

namespace blink {

class ExecutionContext;
class MediaMetadata;

class MediaMetadataSanitizer {
 public:
  // Produce the sanitized metadata, which will later be sent to the
  // MediaSession mojo service.
  static blink::mojom::blink::MediaMetadataPtr SanitizeAndConvertToMojo(
      const MediaMetadata*,
      ExecutionContext*);
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_MODULES_MEDIASESSION_MEDIA_METADATA_SANITIZER_H_
