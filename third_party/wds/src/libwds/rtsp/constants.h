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


#ifndef LIBWDS_RTSP_CONSTANTS_H_
#define LIBWDS_RTSP_CONSTANTS_H_

namespace wds {
namespace rtsp {

const char SEMICOLON[] = ":";
const char SPACE[] = " ";
const char NONE[] = "none";
const char CRLF[] = "\r\n";
const char RTSP_END[] = "RTSP/1.0";

enum PropertyType {
  AVFormatChangeTimingPropertyType,
  AudioCodecsPropertyType,
  ClientRTPPortsPropertyType,
  ConnectorTypePropertyType,
  ContentProtectionPropertyType,
  CoupledSinkPropertyType,
  DisplayEdidPropertyType,
  GenericPropertyType,
  I2CPropertyType,
  IDRRequestPropertyType,
  PreferredDisplayModePropertyType,
  PresentationURLPropertyType,
  RoutePropertyType,
  StandbyPropertyType,
  StandbyResumeCapabilityPropertyType,
  TriggerMethodPropertyType,
  UIBCCapabilityPropertyType,
  UIBCSettingPropertyType,
  Video3DFormatsPropertyType,
  VideoFormatsPropertyType
};

namespace PropertyName {
  const char wfd_audio_codecs[] = "wfd_audio_codecs";
  const char wfd_video_formats[] = "wfd_video_formats";
  const char wfd_3d_video_formats[] = "wfd_3d_video_formats";
  const char wfd_content_protection[] = "wfd_content_protection";
  const char wfd_display_edid[] = "wfd_display_edid";
  const char wfd_coupled_sink[] = "wfd_coupled_sink";
  const char wfd_trigger_method[] = "wfd_trigger_method";
  const char wfd_presentation_url[] = "wfd_presentation_URL";
  const char wfd_client_rtp_ports[] = "wfd_client_rtp_ports";
  const char wfd_route[] = "wfd_route";
  const char wfd_I2C[] = "wfd_I2C";
  const char wfd_av_format_change_timing[] = "wfd_av_format_change_timing";
  const char wfd_preferred_display_mode[] = "wfd_preferred_display_mode";
  const char wfd_uibc_capability[] = "wfd_uibc_capability";
  const char wfd_uibc_setting[] = "wfd_uibc_setting";
  const char wfd_standby_resume_capability[] = "wfd_standby_resume_capability";
  const char wfd_standby[] = "wfd_standby";
  const char wfd_connector_type[] = "wfd_connector_type";
  const char wfd_idr_request[] = "wfd_idr_request";
}  // namespace PropertyName

enum Method {
  OPTIONS,
  SET_PARAMETER,
  GET_PARAMETER,
  SETUP,
  PLAY,
  TEARDOWN,
  PAUSE,
  ORG_WFA_WFD_1_0
};

namespace MethodName {
  const char OPTIONS[] = "OPTIONS";
  const char SET_PARAMETER[] = "SET_PARAMETER";
  const char GET_PARAMETER[] = "GET_PARAMETER";
  const char SETUP[] = "SETUP";
  const char PLAY[] = "PLAY";
  const char TEARDOWN[] = "TEARDOWN";
  const char PAUSE[] = "PAUSE";
  const char ORG_WFA_WFD1_0[] = "org.wfa.wfd1.0";

  const char* const name[] = { OPTIONS,
                              SET_PARAMETER,
                              GET_PARAMETER,
                              SETUP,
                              PLAY,
                              TEARDOWN,
                              PAUSE,
                              ORG_WFA_WFD1_0 };
}

enum RTSPStatusCode {
  STATUS_OK = 200,
  STATUS_SeeOther = 303,
  STATUS_NotAcceptable = 406,
  STATUS_UnsupportedMediaType = 415,
  STATUS_NotImplemented = 501
};

} // namespace rtsp
} // namespace wds

#endif // LIBWDS_RTSP_CONSTANTS_H_
