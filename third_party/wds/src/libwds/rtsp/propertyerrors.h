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


#ifndef LIBWDS_RTSP_PROPERTYERRORS_H_
#define LIBWDS_RTSP_PROPERTYERRORS_H_

#include <string>
#include <vector>
#include "libwds/rtsp/constants.h"

namespace wds {
namespace rtsp {

class PropertyErrors {
 public:
  PropertyErrors(PropertyType type, const std::vector<unsigned short>& error_codes);
  PropertyErrors(const std::string& generic_property_name, const std::vector<unsigned short>& error_codes);
  ~PropertyErrors();

  PropertyType type() const { return type_; }
  const std::vector<unsigned short>& error_codes() const { return error_codes_; }
  const std::string& generic_property_name() const { return generic_property_name_; }

  std::string ToString() const;

 private:
  PropertyType type_;
  std::string generic_property_name_;
  std::vector<unsigned short> error_codes_;
};

}  // namespace rtsp
}  // namespace wds

#endif  // LIBWDS_RTSP_PROPERTYERRORS_H_
