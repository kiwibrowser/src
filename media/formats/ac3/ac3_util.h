// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MEDIA_FORMATS_AC3_AC3_UTIL_H_
#define MEDIA_FORMATS_AC3_AC3_UTIL_H_

#include <stddef.h>
#include <stdint.h>

#include "base/macros.h"
#include "media/base/media_export.h"

namespace media {

class MEDIA_EXPORT Ac3Util {
 public:
  // Returns the total number of audio samples in the given buffer, which
  // contains several complete AC3 syncframes.
  static int ParseTotalAc3SampleCount(const uint8_t* data, size_t size);

  // Returns the total number of audio samples in the given buffer, which
  // contains several complete E-AC3 syncframes.
  static int ParseTotalEac3SampleCount(const uint8_t* data, size_t size);

 private:
  DISALLOW_COPY_AND_ASSIGN(Ac3Util);
};

}  // namespace media

#endif  // MEDIA_FORMATS_AC3_AC3_UTIL_H_
