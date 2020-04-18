// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef PPAPI_THUNK_PPB_FLASH_MESSAGE_LOOP_API_H_
#define PPAPI_THUNK_PPB_FLASH_MESSAGE_LOOP_API_H_

#include <stdint.h>

#include "base/callback_forward.h"
#include "ppapi/c/private/ppb_flash_message_loop.h"

namespace ppapi {
namespace thunk {

class PPB_Flash_MessageLoop_API {
 public:
  virtual ~PPB_Flash_MessageLoop_API() {}

  virtual int32_t Run() = 0;
  virtual void Quit() = 0;

  // This is used by the proxy at the host side to call into the implementation.
  // |callback| is called when the message loop is signaled to quit but before
  // the method returns.
  typedef base::Callback<void (int32_t)> RunFromHostProxyCallback;
  virtual void RunFromHostProxy(
      const RunFromHostProxyCallback& callback) = 0;
};

}  // namespace thunk
}  // namespace ppapi

#endif  // PPAPI_THUNK_PPB_FLASH_MESSAGE_LOOP_API_H_
