// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_ARC_VIDEO_ACCELERATOR_ARC_VIDEO_ACCELERATOR_UTIL_H_
#define COMPONENTS_ARC_VIDEO_ACCELERATOR_ARC_VIDEO_ACCELERATOR_UTIL_H_

#include "base/files/scoped_file.h"
#include "mojo/public/cpp/system/handle.h"
namespace arc {

// Creates ScopedFD from given mojo::ScopedHandle.
// Returns invalid ScopedFD on failure.
base::ScopedFD UnwrapFdFromMojoHandle(mojo::ScopedHandle handle);

}  // namespace arc
#endif  // COMPONENTS_ARC_VIDEO_ACCELERATOR_ARC_VIDEO_ACCELERATOR_UTIL_H_
