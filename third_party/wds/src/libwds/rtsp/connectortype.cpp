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


#include "libwds/rtsp/connectortype.h"

#include "libwds/rtsp/macros.h"

namespace wds {
namespace rtsp {

ConnectorType::ConnectorType()
  : Property(ConnectorTypePropertyType, true),
    connector_type_() {
}

ConnectorType::ConnectorType(unsigned char connector_type)
  : Property(ConnectorTypePropertyType),
    connector_type_(connector_type) {
}

ConnectorType::ConnectorType(wds::ConnectorType connector_type)
  : Property(ConnectorTypePropertyType, connector_type == wds::ConnectorTypeNone) {
  connector_type_ = is_none() ? 0  // Ignored.
                              : static_cast<unsigned char>(connector_type);
}

ConnectorType::~ConnectorType() {
}

std::string ConnectorType::ToString() const {
  std::string ret = PropertyName::wfd_connector_type + std::string(SEMICOLON)
    + std::string(SPACE);

  if (is_none()) {
    ret += NONE;
  } else {
    MAKE_HEX_STRING_2(connector_type, connector_type_);
    ret += connector_type;
  }

  return ret;
}

}  // namespace rtsp
}  // namespace wds
