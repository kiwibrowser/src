// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MEDIA_BASE_WIN_MF_INITIALIZER_H_
#define MEDIA_BASE_WIN_MF_INITIALIZER_H_

#include "media/base/win/mf_initializer_export.h"

namespace media {

// Makes sure MFStartup() is called exactly once.  Returns true if Media
// Foundation is available and has been initialized successfully.  Note that it
// is expected to return false on an "N" edition of Windows, see
// https://en.wikipedia.org/wiki/Windows_7_editions#Special-purpose_editions.
MF_INITIALIZER_EXPORT bool InitializeMediaFoundation();

}  // namespace media

#endif  // MEDIA_BASE_WIN_MF_INITIALIZER_H_
