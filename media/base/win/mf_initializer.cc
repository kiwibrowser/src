// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "media/base/win/mf_initializer.h"

#include <mfapi.h>

#include "base/logging.h"

namespace media {

bool InitializeMediaFoundation() {
  static const bool success = MFStartup(MF_VERSION, MFSTARTUP_LITE) == S_OK;
  DVLOG_IF(1, !success)
      << "Media Foundation unavailable or it failed to initialize";
  return success;
}

}  // namespace media
