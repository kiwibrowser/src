// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MEDIA_GPU_WINDOWS_RETURN_ON_FAILURE_H_
#define MEDIA_GPU_WINDOWS_RETURN_ON_FAILURE_H_

#define RETURN_ON_FAILURE(result, log, ret) \
  do {                                      \
    if (!(result)) {                        \
      DLOG(ERROR) << log;                   \
      return ret;                           \
    }                                       \
  } while (0)

#endif  // MEDIA_GPU_D3D11_WINDOWS_RETURN_ON_FAILURE_H_
