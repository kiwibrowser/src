// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef PPAPI_THUNK_PPB_BROKER_API_H_
#define PPAPI_THUNK_PPB_BROKER_API_H_

#include "base/memory/ref_counted.h"
#include "ppapi/c/pp_bool.h"
#include "ppapi/c/pp_completion_callback.h"
#include "ppapi/c/pp_stdint.h"
#include "ppapi/shared_impl/singleton_resource_id.h"

namespace ppapi {

class TrackedCallback;

namespace thunk {

class PPB_Broker_API {
 public:
  virtual ~PPB_Broker_API() {}

  virtual int32_t Connect(scoped_refptr<TrackedCallback> connect_callback) = 0;
  virtual int32_t GetHandle(int32_t* handle) = 0;
};

// TODO(raymes): Merge this into PPB_Broker_API when the PPB_Broker proxy is
// refactored to the new resource model. The IsAllowed function should be
// attached to the resource implementing the PPB_Broker_API. However in order to
// implement this quickly, the function is added in a new instance API.
class PPB_Broker_Instance_API {
 public:
  virtual ~PPB_Broker_Instance_API() {}

  virtual PP_Bool IsAllowed() = 0;

  static const SingletonResourceID kSingletonResourceID = BROKER_SINGLETON_ID;
};

}  // namespace thunk
}  // namespace ppapi

#endif  // PPAPI_THUNK_PPB_BROKER_API_H_
