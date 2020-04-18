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

#include "libwds/rtsp/property.h"

#include "libwds/public/logging.h"

namespace wds {
namespace rtsp {

Property::Property(PropertyType type)
    : type_(type),
      is_none_(false) {
}

Property::Property(PropertyType type, bool is_none)
    : type_(type),
      is_none_(is_none) {
}

Property::~Property() {
}

std::string Property::ToString() const {
  return std::string();
}

std::string Property::GetName() const {
  if (type_ == GenericPropertyType)
    return std::string();
  return GetPropertyName(type_);
}

std::string GetPropertyName(PropertyType type) {
  switch(type) {
    case AVFormatChangeTimingPropertyType:
      return PropertyName::wfd_av_format_change_timing;
    case AudioCodecsPropertyType:
      return PropertyName::wfd_audio_codecs;
    case ClientRTPPortsPropertyType:
      return PropertyName::wfd_client_rtp_ports;
    case ConnectorTypePropertyType:
      return PropertyName::wfd_connector_type;
    case ContentProtectionPropertyType:
      return PropertyName::wfd_content_protection;
    case CoupledSinkPropertyType:
      return PropertyName::wfd_coupled_sink;
    case DisplayEdidPropertyType:
      return PropertyName::wfd_display_edid;
    case GenericPropertyType:
      WDS_ERROR("Generic property does not have a defined name");
      return std::string();
    case I2CPropertyType:
      return PropertyName::wfd_I2C;
    case IDRRequestPropertyType:
      return PropertyName::wfd_idr_request;
    case PreferredDisplayModePropertyType:
      return PropertyName::wfd_preferred_display_mode;
    case PresentationURLPropertyType:
      return PropertyName::wfd_presentation_url;
    case RoutePropertyType:
      return PropertyName::wfd_route;
    case StandbyPropertyType:
      return PropertyName::wfd_standby;
    case StandbyResumeCapabilityPropertyType:
      return PropertyName::wfd_standby_resume_capability;
    case TriggerMethodPropertyType:
      return PropertyName::wfd_trigger_method;
    case UIBCCapabilityPropertyType:
      return PropertyName::wfd_uibc_capability;
    case UIBCSettingPropertyType:
      return PropertyName::wfd_uibc_setting;
    case Video3DFormatsPropertyType:
      return PropertyName::wfd_3d_video_formats;
    case VideoFormatsPropertyType:
      return PropertyName::wfd_video_formats;
    default:
      WDS_ERROR("Unknown property type %d", type);
      return std::string();
  }
}

}  // namespace rtsp
}  // namespace wds
