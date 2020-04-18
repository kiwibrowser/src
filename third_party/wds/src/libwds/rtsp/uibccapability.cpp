/*
 * This file is part of Wireless Display Software for Linux OS
 *
 * Copyright (C) 2014 Intel Corporation.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
 * 02110-1301 USA
 */


#include "libwds/rtsp/uibccapability.h"

namespace wds {
namespace rtsp {

namespace {

const char kInputCategoryGeneric[] = "GENERIC";
const char kInputCategoryHIDC[] = "HIDC";
const char kInputCategoryList[] = "input_category_list=";
const char* kInputCategories[] = { kInputCategoryGeneric, kInputCategoryHIDC };

const char kInputTypeKeyboard[] = "Keyboard";
const char kInputTypeMouse[] = "Mouse";
const char kInputTypeSingleTouch[] = "SingleTouch";
const char kInputTypeMultiTouch[] = "MultiTouch";
const char kInputTypeJoystick[] = "Joystick";
const char kInputTypeCamera[] = "Camera";
const char kInputTypeGesture[] = "Gesture";
const char kInputTypeRemoteControl[] = "RemoteControl";
const char kInputGenericCapabilityList[] = "generic_cap_list=";

const char* kInputTypes[] =
  { kInputTypeKeyboard,
    kInputTypeMouse,
    kInputTypeSingleTouch,
    kInputTypeMultiTouch,
    kInputTypeJoystick,
    kInputTypeCamera,
    kInputTypeGesture,
    kInputTypeRemoteControl
  };

const char kInputPathInfrared[] = "Infrared";
const char kInputPathUSB[] = "USB";
const char kInputPathBT[] = "BT";
const char kInputPathZigbee[] = "Zigbee";
const char kInputPathWiFi[] = "Wi-Fi";
const char kInputPathNoSP[] = "No-SP";
const char kHIDCCapabilityList[] = "hidc_cap_list=";

const char* kInputPaths[] =
  { kInputPathInfrared,
    kInputPathUSB,
    kInputPathBT,
    kInputPathZigbee,
    kInputPathWiFi,
    kInputPathNoSP
  };

}

UIBCCapability::UIBCCapability()
  : Property(UIBCCapabilityPropertyType, true) {
}

UIBCCapability::UIBCCapability(
    const std::vector<InputCategory>& input_categories,
    const std::vector<InputType>& generic_capabilities,
    const std::vector<DetailedCapability> hidc_capabilities,
    int tcp_port)
  : Property(UIBCCapabilityPropertyType),
    input_categories_(input_categories),
    generic_capabilities_(generic_capabilities),
    hidc_capabilities_(hidc_capabilities),
    tcp_port_(tcp_port) {
}

UIBCCapability::~UIBCCapability() {
}

std::string UIBCCapability::ToString() const {
  std::string ret;
  ret = PropertyName::wfd_uibc_capability + std::string(SEMICOLON)
  + std::string(SPACE);

  if (is_none())
    return ret + NONE;

  ret += kInputCategoryList;
  auto inp_cat_i = input_categories_.begin();
  auto inp_cat_end = input_categories_.end();

  if (input_categories_.empty())
    ret += NONE;

  while (inp_cat_i != inp_cat_end) {
    ret += kInputCategories[*inp_cat_i];

    if(inp_cat_i != --input_categories_.end())
      ret += std::string(",") + std::string(SPACE);
    else
      ret += std::string(";");

    ++inp_cat_i;
  }

  ret += kInputGenericCapabilityList;
  auto gen_cap_i = generic_capabilities_.begin();
  auto gen_cap_end = generic_capabilities_.end();

  if (generic_capabilities_.empty())
    ret += NONE;

  while (gen_cap_i != gen_cap_end) {
    ret += kInputTypes[*gen_cap_i];

    if(gen_cap_i != --generic_capabilities_.end())
      ret += std::string(",") + std::string(SPACE);
    else
      ret += std::string(";");

    ++gen_cap_i;
  }

  ret += kHIDCCapabilityList;
  auto hidc_cap_i = hidc_capabilities_.begin();
  auto hidc_cap_end = hidc_capabilities_.end();

  if (hidc_capabilities_.empty())
    ret += NONE;

  while (hidc_cap_i != hidc_cap_end) {
    ret += kInputTypes[(*hidc_cap_i).first]
         + std::string("/") + kInputPaths[(*hidc_cap_i).second];

    if(hidc_cap_i != --hidc_capabilities_.end())
      ret += std::string(",") + std::string(SPACE);
    else
      ret += std::string(";");

    ++hidc_cap_i;
  }

  ret += tcp_port_ > 0 ? std::string("port=") + std::to_string(tcp_port_)
                       : NONE;

  return ret;
}

}  // namespace rtsp
}  // namespace wds
