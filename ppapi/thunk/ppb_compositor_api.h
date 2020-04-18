// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef PPAPI_THUNK_PPB_COMPOSITOR_API_H_
#define PPAPI_THUNK_PPB_COMPOSITOR_API_H_

#include <stdint.h>

#include "base/memory/ref_counted.h"
#include "ppapi/c/ppb_compositor.h"
#include "ppapi/shared_impl/tracked_callback.h"

namespace ppapi {
namespace thunk {

class PPAPI_THUNK_EXPORT PPB_Compositor_API {
 public:
  virtual ~PPB_Compositor_API() {}
  virtual PP_Resource AddLayer() = 0;
  virtual int32_t CommitLayers(
      const scoped_refptr<ppapi::TrackedCallback>& callback) = 0;
  virtual int32_t ResetLayers() = 0;
};

}  // namespace thunk
}  // namespace ppapi

#endif  // PPAPI_THUNK_PPB_COMPOSITOR_API_H_
