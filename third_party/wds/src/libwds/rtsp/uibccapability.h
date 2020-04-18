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


#ifndef LIBWDS_RTSP_UIBCCAPABILITY_H_
#define LIBWDS_RTSP_UIBCCAPABILITY_H_

#include "libwds/rtsp/property.h"

#include <vector>
#include <utility>

namespace wds {
namespace rtsp {

class UIBCCapability: public Property {
 public:
  enum InputCategory {
    GENERIC,
    HIDC
  };

  enum InputType {
    KEYBOARD,
    MOUSE,
    SINGLE_TOUCH,
    MULTI_TOUCH,
    JOYSTICK,
    CAMERA,
    GESTURE,
    REMOTE_CONTROL
  };

  enum InputPath {
    INFRARED,
    USB,
    BT,
    ZIGBEE,
    WI_FI,
    NO_SP
  };

 typedef std::pair<InputType, InputPath> DetailedCapability;

 public:
  UIBCCapability();
  UIBCCapability(const std::vector<InputCategory>& input_categories,
      const std::vector<InputType>& generic_capabilities,
      const std::vector<DetailedCapability> hidc_capabilities,
      int tcp_port);
  ~UIBCCapability() override;

  std::string ToString() const override;

 private:

  std::vector<InputCategory> input_categories_;
  std::vector<InputType> generic_capabilities_;
  std::vector<DetailedCapability> hidc_capabilities_;
  int tcp_port_;
};

}  // namespace rtsp
}  // namespace wds

#endif  // LIBWDS_RTSP_UIBCCAPABILITY_H_
