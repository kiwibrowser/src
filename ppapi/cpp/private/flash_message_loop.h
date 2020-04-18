// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef PPAPI_CPP_PRIVATE_FLASH_MESSAGE_LOOP_H_
#define PPAPI_CPP_PRIVATE_FLASH_MESSAGE_LOOP_H_

#include "ppapi/c/pp_stdint.h"
#include "ppapi/cpp/resource.h"

namespace pp {

class InstanceHandle;

namespace flash {

class MessageLoop : public Resource {
 public:
  explicit MessageLoop(const InstanceHandle& instance);
  virtual ~MessageLoop();

  static bool IsAvailable();

  int32_t Run();
  void Quit();
};

}  // namespace flash
}  // namespace pp

#endif  // PPAPI_CPP_PRIVATE_FLASH_MESSAGE_LOOP_H_
