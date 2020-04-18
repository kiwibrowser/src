// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_PUBLIC_RENDERER_KEY_SYSTEM_SUPPORT_H_
#define CONTENT_PUBLIC_RENDERER_KEY_SYSTEM_SUPPORT_H_

#include <string>
#include <vector>

#include "content/common/content_export.h"
#include "media/base/decrypt_config.h"
#include "media/base/video_codecs.h"

namespace content {

// Determines if |key_system| is supported by calling into the browser.
// If it is supported, return true and |supported_video_codecs|,
// |supports_persistent_license| and |supported_encryption_schemes| are updated
// to match what |key_system| supports. If not supported, false is returned.
CONTENT_EXPORT bool IsKeySystemSupported(
    const std::string& key_system,
    std::vector<media::VideoCodec>* supported_video_codecs,
    bool* supports_persistent_license,
    std::vector<media::EncryptionMode>* supported_encryption_schemes);

}  // namespace content

#endif  // CONTENT_PUBLIC_RENDERER_KEY_SYSTEM_SUPPORT_H_
