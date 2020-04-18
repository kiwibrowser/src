// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "media/base/key_system_properties.h"

#include "base/logging.h"
#include "media/media_buildflags.h"

namespace media {

#if defined(OS_ANDROID)
SupportedCodecs KeySystemProperties::GetSupportedSecureCodecs() const {
  return EME_CODEC_NONE;
}
#endif

bool KeySystemProperties::UseAesDecryptor() const {
  return false;
}

}  // namespace media
