// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MEDIA_BASE_DECODE_STATUS_H_
#define MEDIA_BASE_DECODE_STATUS_H_

#include <iosfwd>

#include "media/base/media_export.h"

namespace media {

enum class DecodeStatus {
  OK = 0,        // Everything went as planned.
  ABORTED,       // Read aborted due to Reset() during pending read.
  DECODE_ERROR,  // Decoder returned decode error. Note: Prefixed by DECODE_
                 // since ERROR is a reserved name (special macro) on Windows.
  DECODE_STATUS_MAX = DECODE_ERROR
};

// Helper function so that DecodeStatus can be printed easily.
MEDIA_EXPORT std::ostream& operator<<(std::ostream& os,
                                      const DecodeStatus& status);

}  // namespace media

#endif  // MEDIA_BASE_DECODE_STATUS_H_
