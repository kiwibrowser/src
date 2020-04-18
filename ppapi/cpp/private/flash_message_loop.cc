// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ppapi/cpp/private/flash_message_loop.h"

#include "ppapi/c/pp_errors.h"
#include "ppapi/c/private/ppb_flash_message_loop.h"
#include "ppapi/cpp/instance_handle.h"
#include "ppapi/cpp/module_impl.h"

namespace pp {

namespace {

template <> const char* interface_name<PPB_Flash_MessageLoop>() {
  return PPB_FLASH_MESSAGELOOP_INTERFACE;
}

}  // namespace

namespace flash {

MessageLoop::MessageLoop(const InstanceHandle& instance) {
  if (has_interface<PPB_Flash_MessageLoop>()) {
    PassRefFromConstructor(get_interface<PPB_Flash_MessageLoop>()->Create(
        instance.pp_instance()));
  }
}

MessageLoop::~MessageLoop() {
}

// static
bool MessageLoop::IsAvailable() {
  return has_interface<PPB_Flash_MessageLoop>();
}

int32_t MessageLoop::Run() {
  if (!has_interface<PPB_Flash_MessageLoop>())
    return PP_ERROR_NOINTERFACE;
  return get_interface<PPB_Flash_MessageLoop>()->Run(pp_resource());
}

void MessageLoop::Quit() {
  if (has_interface<PPB_Flash_MessageLoop>())
    get_interface<PPB_Flash_MessageLoop>()->Quit(pp_resource());
}

}  // namespace flash
}  // namespace pp
