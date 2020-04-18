// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef PPAPI_SHARED_IMPL_PPB_FLASH_FONT_FILE_SHARED_H_
#define PPAPI_SHARED_IMPL_PPB_FLASH_FONT_FILE_SHARED_H_

#include "ppapi/c/pp_bool.h"

namespace ppapi {

class PPB_Flash_FontFile_Shared {
 public:
  static PP_Bool IsSupportedForWindows() { return PP_TRUE; }
};

}  // namespace ppapi

#endif   // PPAPI_SHARED_IMPL_PPB_FLASH_FONT_FILE_SHARED_H_