// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef PPAPI_CPP_PRIVATE_FLASH_MENU_H_
#define PPAPI_CPP_PRIVATE_FLASH_MENU_H_

#include <stdint.h>

#include "ppapi/c/private/ppb_flash_menu.h"
#include "ppapi/cpp/resource.h"

namespace pp {

class CompletionCallback;
class InstanceHandle;
class Point;

namespace flash {

class Menu : public Resource {
 public:
  // TODO(viettrungluu): Write a proper C++ wrapper of |PP_Flash_Menu|.
  Menu(const InstanceHandle& instance, const struct PP_Flash_Menu* menu_data);

  int32_t Show(const Point& location,
               int32_t* selected_id,
               const CompletionCallback& cc);
};

}  // namespace flash
}  // namespace pp

#endif  // PPAPI_CPP_PRIVATE_FLASH_H_
