// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef PPAPI_THUNK_PPB_FLASH_FONT_FILE_API_H_
#define PPAPI_THUNK_PPB_FLASH_FONT_FILE_API_H_

#include "ppapi/c/pp_stdint.h"

namespace ppapi {
namespace thunk {

class PPB_Flash_FontFile_API {
 public:
  virtual ~PPB_Flash_FontFile_API() {}

  virtual PP_Bool GetFontTable(uint32_t table,
                               void* output,
                               uint32_t* output_length) = 0;
};

}  // namespace thunk
}  // namespace ppapi

#endif  // PPAPI_THUNK_PPB_FLASH_FONT_FILE_API_H_

