// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef PPAPI_CPP_PRIVATE_FLASH_FULLSCREEN_H_
#define PPAPI_CPP_PRIVATE_FLASH_FULLSCREEN_H_

#include "ppapi/cpp/instance_handle.h"

namespace pp {

class Size;

class FlashFullscreen {
 public:
  FlashFullscreen(const InstanceHandle& instance);
  virtual ~FlashFullscreen();

  // PPB_FlashFullscreen methods.
  bool IsFullscreen();
  bool SetFullscreen(bool fullscreen);
  bool GetScreenSize(Size* size);

  bool MustRecreateContexts();

 private:
  InstanceHandle instance_;
};

}  // namespace pp

#endif  // PPAPI_CPP_PRIVATE_FLASH_FULLSCREEN_H_
