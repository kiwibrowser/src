// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <stdint.h>

#include "base/memory/ref_counted.h"
#include "ppapi/thunk/ppapi_thunk_export.h"

namespace ppapi {

class TrackedCallback;

namespace thunk {

class PPAPI_THUNK_EXPORT PPB_Flash_DRM_API {
 public:
  virtual ~PPB_Flash_DRM_API() {}

  virtual int32_t GetDeviceID(PP_Var* id,
                              scoped_refptr<TrackedCallback> callback) = 0;
  virtual PP_Bool GetHmonitor(int64_t* hmonitor) = 0;
  virtual int32_t GetVoucherFile(PP_Resource* file_ref,
                                 scoped_refptr<TrackedCallback> callback) = 0;
  virtual int32_t MonitorIsExternal(
      PP_Bool* is_external,
      scoped_refptr<TrackedCallback> callback) = 0;
};

}  // namespace thunk
}  // namespace ppapi

