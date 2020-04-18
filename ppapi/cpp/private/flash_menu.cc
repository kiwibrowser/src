// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ppapi/cpp/private/flash_menu.h"

#include "ppapi/c/pp_errors.h"
#include "ppapi/cpp/completion_callback.h"
#include "ppapi/cpp/instance_handle.h"
#include "ppapi/cpp/module.h"
#include "ppapi/cpp/module_impl.h"
#include "ppapi/cpp/point.h"

namespace pp {

namespace {

template <> const char* interface_name<PPB_Flash_Menu>() {
  return PPB_FLASH_MENU_INTERFACE;
}

}  // namespace

namespace flash {

Menu::Menu(const InstanceHandle& instance,
           const struct PP_Flash_Menu* menu_data) {
  if (has_interface<PPB_Flash_Menu>()) {
    PassRefFromConstructor(get_interface<PPB_Flash_Menu>()->Create(
        instance.pp_instance(), menu_data));
  }
}

int32_t Menu::Show(const Point& location,
                   int32_t* selected_id,
                   const CompletionCallback& cc) {
  if (!has_interface<PPB_Flash_Menu>())
    return cc.MayForce(PP_ERROR_NOINTERFACE);
  return get_interface<PPB_Flash_Menu>()->Show(
      pp_resource(),
      &location.pp_point(),
      selected_id,
      cc.pp_completion_callback());
}

}  // namespace flash
}  // namespace pp
