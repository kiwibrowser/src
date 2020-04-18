// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef PPAPI_THUNK_PPB_VIDEO_DESTINATION_PRIVATE_API_H_
#define PPAPI_THUNK_PPB_VIDEO_DESTINATION_PRIVATE_API_H_

#include <stdint.h>

#include "base/memory/ref_counted.h"
#include "ppapi/thunk/ppapi_thunk_export.h"

struct PP_VideoFrame_Private;

namespace ppapi {

class TrackedCallback;

namespace thunk {

class PPAPI_THUNK_EXPORT PPB_VideoDestination_Private_API {
 public:
  virtual ~PPB_VideoDestination_Private_API() {}

  virtual int32_t Open(const PP_Var& stream_url,
                       scoped_refptr<TrackedCallback> callback) = 0;
  virtual int32_t PutFrame(const PP_VideoFrame_Private& frame) = 0;
  virtual void Close() = 0;
};

}  // namespace thunk
}  // namespace ppapi

#endif  // PPAPI_THUNK_PPB_VIDEO_DESTINATION_PRIVATE_API_H_
