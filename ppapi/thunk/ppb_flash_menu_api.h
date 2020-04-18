// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef PPAPI_THUNK_PPB_FLASH_MENU_API_H_
#define PPAPI_THUNK_PPB_FLASH_MENU_API_H_

#include <stdint.h>

#include "base/memory/ref_counted.h"
#include "ppapi/c/private/ppb_flash_menu.h"

namespace ppapi {

class TrackedCallback;

namespace thunk {

class PPB_Flash_Menu_API {
 public:
  virtual ~PPB_Flash_Menu_API() {}

  virtual int32_t Show(const PP_Point* location,
                       int32_t* selected_id,
                       scoped_refptr<TrackedCallback> callback) = 0;
};

}  // namespace thunk
}  // namespace ppapi

#endif  // PPAPI_THUNK_PPB_FLASH_MENU_API_H_
