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


#include <algorithm>
#include <iostream>
#include <list>

#include "libwds/rtsp/audiocodecs.h"
#include "libwds/rtsp/avformatchangetiming.h"
#include "libwds/rtsp/clientrtpports.h"
#include "libwds/rtsp/connectortype.h"
#include "libwds/rtsp/constants.h"
#include "libwds/rtsp/contentprotection.h"
#include "libwds/rtsp/displayedid.h"
#include "libwds/rtsp/driver.h"
#include "libwds/rtsp/formats3d.h"
#include "libwds/rtsp/i2c.h"
#include "libwds/rtsp/presentationurl.h"
#include "libwds/rtsp/propertyerrors.h"
#include "libwds/rtsp/reply.h"
#include "libwds/rtsp/route.h"
#include "libwds/rtsp/triggermethod.h"
#include "libwds/rtsp/uibcsetting.h"
#include "libwds/rtsp/videoformats.h"

using wds::rtsp::Driver;

typedef bool (*TestFunc)(void);

#define ASSERT_EQUAL(value, expected) \
  if ((value) != (expected)) { \
    std::cout << __func__ << " (" << __FILE__ << ":" << __LINE__ << "): " \
              << #value << ": " \
              << "expected '" << (expected) \
              << "', got '" << (value) << "'" \
              << std::endl; \
    return 0; \
  }

#define ASSERT(assertion) \
  if (!(assertion)) { \
    std::cout << __func__ << " (" << __FILE__ << ":" << __LINE__ << "): " \
              << "assertion failed: " << #assertion \
              << std::endl; \
    return 0; \
  }

#define ASSERT_NO_EXCEPTION(method_call) \
  try { \
    method_call; \
  } catch (std::exception &x) { \
    std::cout << __func__ << " (" << __FILE__ << ":" << __LINE__ << "): " \
              << "unexpected exception: " << #method_call << ": " \
              << x.what() << std::endl; \
    return false; \
  }

#define ASSERT_EXCEPTION(method_call) \
  try { \
    method_call; \
    std::cout << __func__ << " (" << __FILE__ << ":" << __LINE__ << "): " \
              << "expected exception: " << #method_call << std::endl; \
    return false; \
  } catch (std::exception &x) { \
    ; \
  }

static bool property_type_exists (std::vector<std::string> properties,
                                  wds::rtsp::PropertyType type)
{
  return std::find (properties.begin(),
                    properties.end(),
                    wds::rtsp::GetPropertyName(type)) != properties.end();
}

static bool test_audio_codec (wds::AudioCodec codec, wds::AudioFormats format,
                              unsigned long int modes, unsigned char latency)
{
  ASSERT_EQUAL(codec.format, format);
  ASSERT_EQUAL(codec.modes.to_ulong(), modes);
  ASSERT_EQUAL(codec.latency, latency);

  return true;
}

static bool test_h264_codec_3d (wds::rtsp::H264Codec3d codec,
                                unsigned char profile, unsigned char level,
                                unsigned long long int video_capability_3d, unsigned char latency,
                                unsigned short min_slice_size, unsigned short slice_enc_params,
                                unsigned char frame_rate_control_support, int max_hres,
                                int max_vres)
{
  ASSERT_EQUAL(codec.profile_, profile);
  ASSERT_EQUAL(codec.level_, level);
  ASSERT_EQUAL(codec.video_capability_3d_, video_capability_3d);
  ASSERT_EQUAL(codec.latency_, latency);
  ASSERT_EQUAL(codec.min_slice_size_, min_slice_size);
  ASSERT_EQUAL(codec.slice_enc_params_, slice_enc_params);
  ASSERT_EQUAL(codec.frame_rate_control_support_, frame_rate_control_support);

  return true;
}

static bool test_valid_options ()
{
  std::string header("OPTIONS * RTSP/1.0\r\n"
                     "CSeq: 0\r\n"
                     "Require: org.wfa.wfd1.0\r\n\r\n");
  std::unique_ptr<wds::rtsp::Message> message;
  Driver::Parse(header, message);
  ASSERT(message != NULL);
  ASSERT(message->is_request());
  wds::rtsp::Request* request = wds::rtsp::ToRequest(message.get());
  ASSERT_EQUAL(request->method(), wds::rtsp::Request::MethodOptions);
  ASSERT_EQUAL(request->header().cseq(), 0);
  ASSERT_EQUAL(request->header().content_length(), 0);
  ASSERT_EQUAL(request->header().require_wfd_support(), true);
  ASSERT_EQUAL (request->ToString(), header);

  return true;
}

static bool test_valid_options_reply ()
{
  std::string header("RTSP/1.0 200 OK\r\n"
                     "CSeq: 1\r\n"
                     "Public: org.wfa.wfd1.0, SETUP, TEARDOWN, PLAY, PAUSE, GET_PARAMETER, SET_PARAMETER\r\n\r\n");
  std::unique_ptr<wds::rtsp::Message> message;
  Driver::Parse(header, message);
  ASSERT(message != NULL);
  ASSERT(message->is_reply());

  wds::rtsp::Reply* reply = static_cast<wds::rtsp::Reply*>(message.get());
  ASSERT(reply != NULL);
  ASSERT_EQUAL(reply->response_code(), 200);
  ASSERT_EQUAL(reply->header().cseq(), 1);
  ASSERT_EQUAL(reply->header().content_length(), 0);

  std::vector<wds::rtsp::Method> methods = message->header().supported_methods();
  static const wds::rtsp::Method expected[] = { wds::rtsp::Method::ORG_WFA_WFD_1_0,
                                                wds::rtsp::Method::SETUP,
                                                wds::rtsp::Method::TEARDOWN,
                                                wds::rtsp::Method::PLAY,
                                                wds::rtsp::Method::PAUSE,
                                                wds::rtsp::Method::GET_PARAMETER,
                                                wds::rtsp::Method::SET_PARAMETER };

  ASSERT_EQUAL(methods.size(), 7);
  for (int i = 0; i < 7; i++) {
    std::vector<wds::rtsp::Method>::iterator method = std::find(methods.begin(), methods.end(), expected[i]);
    ASSERT(method != methods.end());
  }

  ASSERT_EQUAL (reply->ToString(), header);

  return true;
}

static bool test_valid_extra_properties ()
{
  std::string header("RTSP/1.0 200 OK\r\n"
                     "CSeq: 2\r\n"
                     "Content-Type: text/parameters\r\n"
                     "Content-Length: 72\r\n"
                     "My-Header: 123 testing testing\r\n\r\n");
  std::unique_ptr<wds::rtsp::Message> message;
  Driver::Parse(header, message);
  ASSERT(message != NULL);
  ASSERT(message->is_reply());

  GenericHeaderMap gen_headers = message->header().generic_headers();
  auto extra_header_it = gen_headers.find("My-Header");
  ASSERT(extra_header_it != gen_headers.end())
  ASSERT_EQUAL(extra_header_it->second, "123 testing testing");
  ASSERT_EQUAL(message->header().content_length(), 72);

  std::string payload_buffer("nonstandard_property: 1!!1! non standard value\r\n"
                      "wfd_audio_codecs: none\r\n");
  Driver::Parse(payload_buffer, message);

  auto payload = ToPropertyMapPayload(message->payload());
  ASSERT(payload);
  std::shared_ptr<wds::rtsp::Property> property;

  ASSERT_NO_EXCEPTION (property =
      payload->GetProperty(wds::rtsp::AudioCodecsPropertyType));
  ASSERT(property->is_none());

  ASSERT_NO_EXCEPTION (property =
      payload->GetProperty("nonstandard_property"));
  auto extra_property = std::static_pointer_cast<wds::rtsp::GenericProperty>(property);
  ASSERT_EQUAL(extra_property->value(), "1!!1! non standard value");

  ASSERT_EQUAL(message->ToString(), header + payload_buffer);

  return true;
}

static bool test_valid_extra_errors ()
{
  std::string header("RTSP/1.0 303 OK\r\n"
                     "CSeq: 0\r\n"
                     "Content-Type: text/parameters\r\n"
                     "Content-Length: 55\r\n\r\n");

  std::unique_ptr<wds::rtsp::Message> message;
  Driver::Parse(header, message);
  ASSERT(message != NULL);
  ASSERT(message->is_reply());

  std::string payload_buffer("wfd_audio_codecs: 103\r\n"
                      "nonstandard_property: 101, 102\r\n");
  Driver::Parse(payload_buffer, message);
  ASSERT(message != NULL);

  auto payload = ToPropertyErrorPayload(message->payload());
  ASSERT(payload);

  std::shared_ptr<wds::rtsp::PropertyErrors> error;

  ASSERT_EQUAL(payload->property_errors().size(), 2);

  ASSERT_NO_EXCEPTION(error =
      payload->GetPropertyError(wds::rtsp::AudioCodecsPropertyType));
  ASSERT_EQUAL(error->error_codes().size(), 1);
  ASSERT_EQUAL(error->error_codes()[0], 103);

  ASSERT_NO_EXCEPTION(error =
      payload->GetPropertyError("nonstandard_property"));
  ASSERT_EQUAL(error->error_codes().size(), 2);
  ASSERT_EQUAL(error->error_codes()[0], 101);
  ASSERT_EQUAL(error->error_codes()[1], 102);

  ASSERT_EQUAL(message->ToString(), header + payload_buffer);

  return true;
}

static bool test_valid_extra_properties_in_get ()
{
  std::string header("GET_PARAMETER rtsp://localhost/wfd1.0 RTSP/1.0\r\n"
                     "CSeq: 2\r\n"
                     "Content-Type: text/parameters\r\n"
                     "Content-Length: 40\r\n\r\n");

  std::unique_ptr<wds::rtsp::Message> message;
  Driver::Parse(header, message);
  ASSERT(message != NULL);
  ASSERT(message->is_request());
  wds::rtsp::Request* request = wds::rtsp::ToRequest(message.get());
  ASSERT_EQUAL(request->method(), wds::rtsp::Request::MethodGetParameter);

  std::string payload_buffer("nonstandard_property\r\n"
                             "wfd_audio_codecs\r\n");
  Driver::Parse(payload_buffer, message);
  auto payload = ToGetParameterPayload(message->payload());
  ASSERT(payload);
  auto properties = payload->properties();
  ASSERT(property_type_exists (properties, wds::rtsp::AudioCodecsPropertyType));
  ASSERT_EQUAL(properties.size(), 2);
  ASSERT_EQUAL(properties[0], "nonstandard_property");
  ASSERT_EQUAL(properties[1], "wfd_audio_codecs");

  ASSERT_EQUAL(message->ToString(), header + payload_buffer);

  return true;
}

static bool test_valid_get_parameter ()
{
  std::string header("GET_PARAMETER rtsp://localhost/wfd1.0 RTSP/1.0\r\n"
                     "CSeq: 2\r\n"
                     "Content-Type: text/parameters\r\n"
                     "Content-Length: 213\r\n\r\n");
  std::string payload_buffer("wfd_client_rtp_ports\r\n"
                      "wfd_audio_codecs\r\n"
                      "wfd_video_formats\r\n"
                      "wfd_3d_video_formats\r\n"
                      "wfd_coupled_sink\r\n"
                      "wfd_display_edid\r\n"
                      "wfd_connector_type\r\n"
                      "wfd_uibc_capability\r\n"
                      "wfd_standby_resume_capability\r\n"
                      "wfd_content_protection\r\n");

  std::unique_ptr<wds::rtsp::Message> message;
  Driver::Parse(header, message);
  ASSERT(message != NULL);
  ASSERT(message->is_request());
  Driver::Parse(payload_buffer, message);
  ASSERT(message != NULL);
  wds::rtsp::Request* request = wds::rtsp::ToRequest(message.get());
  ASSERT_EQUAL(request->method(), wds::rtsp::Request::MethodGetParameter);
  ASSERT_EQUAL(request->request_uri(), "rtsp://localhost/wfd1.0");
  ASSERT_EQUAL(message->header().cseq(), 2);
  ASSERT_EQUAL(message->header().content_length(), 213);
  ASSERT_EQUAL(message->header().content_type(), "text/parameters");
  ASSERT_EQUAL(message->header().require_wfd_support(), false);
  auto payload = ToGetParameterPayload(message->payload());
  ASSERT(payload);
  std::vector<std::string> properties = payload->properties();
  ASSERT(property_type_exists (properties, wds::rtsp::ClientRTPPortsPropertyType));
  ASSERT(property_type_exists (properties, wds::rtsp::ClientRTPPortsPropertyType));
  ASSERT(property_type_exists (properties, wds::rtsp::AudioCodecsPropertyType));
  ASSERT(property_type_exists (properties, wds::rtsp::VideoFormatsPropertyType));
  ASSERT(property_type_exists (properties, wds::rtsp::Video3DFormatsPropertyType));
  ASSERT(property_type_exists (properties, wds::rtsp::CoupledSinkPropertyType));
  ASSERT(property_type_exists (properties, wds::rtsp::DisplayEdidPropertyType));
  ASSERT(property_type_exists (properties, wds::rtsp::ConnectorTypePropertyType));
  ASSERT(property_type_exists (properties, wds::rtsp::UIBCCapabilityPropertyType));
  ASSERT(property_type_exists (properties, wds::rtsp::StandbyResumeCapabilityPropertyType));
  ASSERT(property_type_exists (properties, wds::rtsp::ContentProtectionPropertyType));

  ASSERT_EQUAL (message->ToString(), header + payload_buffer)

  return true;
}

static bool test_valid_get_parameter_reply_with_all_none ()
{
  std::string header("RTSP/1.0 200 OK\r\n"
                     "CSeq: 2\r\n"
                     "Content-Type: text/parameters\r\n"
                     "Content-Length: 483\r\n\r\n");
  // not a real-world message, just a collection of all properties
  std::string payload_buffer("wfd_3d_video_formats: none\r\n"
                      "wfd_I2C: none\r\n"
                      "wfd_audio_codecs: none\r\n"
                      "wfd_av_format_change_timing: 000000000F 00000000FF\r\n"
                      "wfd_client_rtp_ports: RTP/AVP/UDP;unicast 19000 0 mode=play\r\n"
                      "wfd_connector_type: none\r\n"
                      "wfd_content_protection: none\r\n"
                      "wfd_coupled_sink: none\r\n"
                      "wfd_display_edid: none\r\n"
                      "wfd_presentation_URL: none none\r\n"
                      "wfd_route: primary\r\n"
                      "wfd_standby_resume_capability: none\r\n"
                      "wfd_trigger_method: TEARDOWN\r\n"
                      "wfd_uibc_capability: none\r\n"
                      "wfd_uibc_setting: disable\r\n"
                      "wfd_video_formats: none\r\n");

  std::unique_ptr<wds::rtsp::Message> message;
  Driver::Parse(header, message);
  ASSERT(message != NULL);
  ASSERT(message->is_reply());
  Driver::Parse(payload_buffer, message);
  ASSERT(message != NULL);

  wds::rtsp::Reply* reply = static_cast<wds::rtsp::Reply*>(message.get());
  ASSERT(reply != NULL);
  ASSERT_EQUAL(reply->response_code(), 200);
  ASSERT_EQUAL(reply->header().cseq(), 2);
  ASSERT_EQUAL(reply->header().content_length(), 483);
  ASSERT_EQUAL(reply->header().content_type(), "text/parameters");
  ASSERT_EQUAL(reply->header().supported_methods().size(), 0);

  auto payload = ToPropertyMapPayload(message->payload());
  ASSERT(payload);
  std::shared_ptr<wds::rtsp::Property> prop;

  ASSERT_NO_EXCEPTION (prop =
      payload->GetProperty(wds::rtsp::AudioCodecsPropertyType));
  ASSERT(prop->is_none());
  ASSERT_NO_EXCEPTION (prop =
      payload->GetProperty(wds::rtsp::VideoFormatsPropertyType));
  ASSERT(prop->is_none());
  ASSERT_NO_EXCEPTION (prop =
      payload->GetProperty(wds::rtsp::Video3DFormatsPropertyType));
  ASSERT(prop->is_none());
  ASSERT_NO_EXCEPTION (prop =
      payload->GetProperty(wds::rtsp::ContentProtectionPropertyType));
  ASSERT(prop->is_none());
  ASSERT_NO_EXCEPTION (prop =
      payload->GetProperty(wds::rtsp::DisplayEdidPropertyType));
  ASSERT(prop->is_none());
  ASSERT_NO_EXCEPTION (prop =
      payload->GetProperty(wds::rtsp::CoupledSinkPropertyType));
  ASSERT(prop->is_none());
  ASSERT_NO_EXCEPTION (prop =
      payload->GetProperty(wds::rtsp::UIBCCapabilityPropertyType));
  ASSERT(prop->is_none());
  ASSERT_NO_EXCEPTION (prop =
      payload->GetProperty(wds::rtsp::ConnectorTypePropertyType));
  ASSERT(prop->is_none());
  ASSERT_NO_EXCEPTION (prop =
      payload->GetProperty(wds::rtsp::StandbyResumeCapabilityPropertyType));
  ASSERT(prop->is_none());

  ASSERT_NO_EXCEPTION (prop =
      payload->GetProperty(wds::rtsp::AVFormatChangeTimingPropertyType));
  auto av_format_change_timing = std::static_pointer_cast<wds::rtsp::AVFormatChangeTiming> (prop);
  ASSERT_EQUAL(av_format_change_timing->pts(), 0x000000000F);
  ASSERT_EQUAL(av_format_change_timing->dts(), 0x00000000FF);

  ASSERT_NO_EXCEPTION (prop =
      payload->GetProperty(wds::rtsp::ClientRTPPortsPropertyType));
  auto client_rtp_ports = std::static_pointer_cast<wds::rtsp::ClientRtpPorts> (prop);
  ASSERT_EQUAL(client_rtp_ports->rtp_port_0(), 19000);
  ASSERT_EQUAL(client_rtp_ports->rtp_port_1(), 0);

  ASSERT_NO_EXCEPTION (prop =
      payload->GetProperty(wds::rtsp::TriggerMethodPropertyType));
  auto trigger_method = std::static_pointer_cast<wds::rtsp::TriggerMethod> (prop);
  ASSERT_EQUAL(trigger_method->method(), wds::rtsp::TriggerMethod::TEARDOWN);

  ASSERT_NO_EXCEPTION (prop =
      payload->GetProperty(wds::rtsp::PresentationURLPropertyType));
  auto presentation_url = std::static_pointer_cast<wds::rtsp::PresentationUrl> (prop);
  ASSERT_EQUAL(presentation_url->presentation_url_1(), "");
  ASSERT_EQUAL(presentation_url->presentation_url_2(), "");

  ASSERT_NO_EXCEPTION (prop =
      payload->GetProperty(wds::rtsp::RoutePropertyType));
  auto route = std::static_pointer_cast<wds::rtsp::Route> (prop);
  ASSERT_EQUAL(route->destination(), wds::rtsp::Route::PRIMARY);

  ASSERT_NO_EXCEPTION (prop =
      payload->GetProperty(wds::rtsp::I2CPropertyType));
  auto i2c = std::static_pointer_cast<wds::rtsp::I2C> (prop);
  ASSERT_EQUAL(i2c->is_supported(), false);

  ASSERT_NO_EXCEPTION (prop =
      payload->GetProperty(wds::rtsp::UIBCSettingPropertyType));
  auto uibc_setting = std::static_pointer_cast<wds::rtsp::UIBCSetting> (prop);
  ASSERT_EQUAL(uibc_setting->is_enabled(), false);

  ASSERT_EQUAL(message->ToString(), header + payload_buffer);

  return true;
}

static bool test_valid_get_parameter_reply ()
{
  std::string header("RTSP/1.0 200 OK\r\n"
                     "CSeq: 2\r\n"
                     "Content-Type: text/parameters\r\n"
                     "Content-Length: 535\r\n\r\n");
  std::string payload_buffer("wfd_3d_video_formats: 80 00 03 0F 0000000000000005 00 0001 1401 13 none none\r\n"
                      "wfd_I2C: 404\r\n"
                      "wfd_audio_codecs: LPCM 00000003 00, AAC 00000001 00\r\n"
                      "wfd_client_rtp_ports: RTP/AVP/UDP;unicast 19000 0 mode=play\r\n"
                      "wfd_connector_type: 05\r\n"
                      "wfd_content_protection: HDCP2.1 port=1189\r\n"
                      "wfd_coupled_sink: none\r\n"
                      "wfd_display_edid: none\r\n"
                      "wfd_standby_resume_capability: supported\r\n"
                      "wfd_uibc_capability: none\r\n"
                      "wfd_video_formats: 40 01 02 04 0001DEFF 053C7FFF 00000FFF 00 0000 0000 11 0400 0300, 01 04 0001DEFF 053C7FFF 00000FFF 00 0000 0000 11 0400 0300\r\n");
  std::unique_ptr<wds::rtsp::Message> message;
  Driver::Parse(header, message);
  ASSERT(message != NULL);
  ASSERT(message->is_reply());
  Driver::Parse(payload_buffer, message);
  ASSERT(message != NULL);

  wds::rtsp::Reply* reply = static_cast<wds::rtsp::Reply*>(message.get());
  ASSERT(reply != NULL);
  ASSERT_EQUAL(reply->response_code(), 200);
  ASSERT_EQUAL(reply->header().cseq(), 2);
  ASSERT_EQUAL(reply->header().content_length(), 535);
  ASSERT_EQUAL(reply->header().content_type(), "text/parameters");
  ASSERT_EQUAL(reply->header().supported_methods().size(), 0);

  auto payload = ToPropertyMapPayload(message->payload());
  ASSERT(payload);
  std::shared_ptr<wds::rtsp::Property> prop;

  ASSERT_NO_EXCEPTION (prop =
      payload->GetProperty(wds::rtsp::AudioCodecsPropertyType));

  // Test that all properties exist
  ASSERT_NO_EXCEPTION (prop =
      payload->GetProperty(wds::rtsp::AudioCodecsPropertyType));
  std::shared_ptr<wds::rtsp::AudioCodecs> audio_codecs = std::static_pointer_cast<wds::rtsp::AudioCodecs> (prop);
  ASSERT_EQUAL(audio_codecs->audio_codecs().size(), 2);
  ASSERT(test_audio_codec (audio_codecs->audio_codecs()[0],
                           wds::LPCM, 3, 0));
  ASSERT(test_audio_codec (audio_codecs->audio_codecs()[1],
                           wds::AAC, 1, 0));
  ASSERT_NO_EXCEPTION (prop =
      payload->GetProperty(wds::rtsp::VideoFormatsPropertyType));
  std::shared_ptr<wds::rtsp::VideoFormats> video_formats = std::static_pointer_cast<wds::rtsp::VideoFormats> (prop);
  ASSERT_EQUAL(video_formats->GetNativeFormat().rate_resolution, 8);
  ASSERT_EQUAL(video_formats->GetNativeFormat().type, 0);
  ASSERT_EQUAL(video_formats->GetH264Formats().size(), 96);

  ASSERT_NO_EXCEPTION (prop =
      payload->GetProperty(wds::rtsp::Video3DFormatsPropertyType));
  std::shared_ptr<wds::rtsp::Formats3d> formats_3d = std::static_pointer_cast<wds::rtsp::Formats3d> (prop);

  ASSERT_EQUAL(formats_3d->native_resolution(), 0x80);
  ASSERT_EQUAL(formats_3d->preferred_display_mode(), 0);
  ASSERT_EQUAL(formats_3d->codecs().size(), 1);
  ASSERT(test_h264_codec_3d (formats_3d->codecs()[0],
                             0x03, 0x0F, 0x0000000000000005, 0, 0x0001, 0x1401, 0x13, 0, 0));

  ASSERT_NO_EXCEPTION (prop =
      payload->GetProperty(wds::rtsp::ContentProtectionPropertyType));
  std::shared_ptr<wds::rtsp::ContentProtection> content_protection = std::static_pointer_cast<wds::rtsp::ContentProtection> (prop);
  ASSERT_EQUAL(content_protection->hdcp_spec(), wds::rtsp::ContentProtection::HDCP_SPEC_2_1);
  ASSERT_EQUAL(content_protection->port(), 1189);

  ASSERT_NO_EXCEPTION (prop =
      payload->GetProperty(wds::rtsp::DisplayEdidPropertyType));
  ASSERT(prop->is_none());

  ASSERT_NO_EXCEPTION (prop =
      payload->GetProperty(wds::rtsp::CoupledSinkPropertyType));
  ASSERT(prop->is_none());

  ASSERT_NO_EXCEPTION (prop =
      payload->GetProperty(wds::rtsp::ClientRTPPortsPropertyType));
  std::shared_ptr<wds::rtsp::ClientRtpPorts> client_rtp_ports = std::static_pointer_cast<wds::rtsp::ClientRtpPorts> (prop);
  ASSERT_EQUAL(client_rtp_ports->rtp_port_0(), 19000);
  ASSERT_EQUAL(client_rtp_ports->rtp_port_1(), 0);

  ASSERT_NO_EXCEPTION (prop =
      payload->GetProperty(wds::rtsp::UIBCCapabilityPropertyType));
  ASSERT(prop->is_none());

  ASSERT_NO_EXCEPTION (prop =
      payload->GetProperty(wds::rtsp::ConnectorTypePropertyType));
  std::shared_ptr<wds::rtsp::ConnectorType> connector_type = std::static_pointer_cast<wds::rtsp::ConnectorType> (prop);
  ASSERT_EQUAL(connector_type->connector_type(), 5);

  ASSERT_NO_EXCEPTION (prop =
      payload->GetProperty(wds::rtsp::I2CPropertyType));
  auto i2c = std::static_pointer_cast<wds::rtsp::I2C> (prop);
  ASSERT_EQUAL(i2c->is_supported(), true);
  ASSERT_EQUAL(i2c->port(), 404);

  ASSERT_NO_EXCEPTION (prop =
      payload->GetProperty(wds::rtsp::StandbyResumeCapabilityPropertyType));
  ASSERT(!prop->is_none());

  ASSERT_EQUAL(message->ToString(), header + payload_buffer);

  return true;
}

static bool test_invalid_property_value ()
{
  std::string header("RTSP/1.0 200 OK\r\n"
                     "CSeq: 2\r\n"
                     "Content-Type: text/parameters\r\n"
                     "Content-Length: 1187\r\n");
  std::string payload_buffer("wfd_uibc_capability: none and something completely different\r\n");

  std::unique_ptr<wds::rtsp::Message> message;
  Driver::Parse(header, message);
  ASSERT(message != NULL);
  ASSERT(message->is_reply());

  Driver::Parse(payload_buffer, message);
  ASSERT(message == NULL);

  return true;
}

static bool test_case_insensitivity ()
{
  std::string invalid_header("OptionS * RTSP/1.0\r\n"
                             "CSeq: 0\r\n"
                             "Require: org.wfa.wfd1.0\r\n\r\n");
  std::unique_ptr<wds::rtsp::Message> message;
  Driver::Parse(invalid_header, message);
  ASSERT(message == NULL);

  std::string header("RTSP/1.0 200 OK\r\n"
                     "CSEQ: 2\r\n"
                     "Content-Type: tEXT/parameters\r\n"
                     "Content-LENGTH: 1187\r\n");
  std::string payload_buffer("wfd_uibc_capABILITY: noNE\r\n\r\n");

  Driver::Parse(header, message);
  ASSERT(message != NULL);
  ASSERT(message->is_reply());
  Driver::Parse(payload_buffer, message);
  ASSERT(message != NULL);

  ASSERT_EQUAL(message->header().cseq(), 2);
  ASSERT_EQUAL(message->header().content_type(), "tEXT/parameters");
  ASSERT_EQUAL(message->header().content_length(), 1187);

  auto payload = ToPropertyMapPayload(message->payload());
  ASSERT(payload);
  std::shared_ptr<wds::rtsp::Property> prop;

  ASSERT_NO_EXCEPTION (prop =
      payload->GetProperty(wds::rtsp::UIBCCapabilityPropertyType));
  ASSERT(prop->is_none());

  // TODO test insensitivity of triggers and method list

  return true;
}

static bool test_valid_get_parameter_reply_with_errors ()
{
  std::string header("RTSP/1.0 303 OK\r\n"
                     "CSeq: 2\r\n"
                     "Content-Type: text/parameters\r\n"
                     "Content-Length: 42\r\n\r\n");
  std::string payload_buffer("wfd_audio_codecs: 415, 457\r\n"
                      "wfd_I2C: 404\r\n");

  std::unique_ptr<wds::rtsp::Message> message;
  Driver::Parse(header, message);
  ASSERT(message != NULL);
  ASSERT(message->is_reply());
  Driver::Parse(payload_buffer, message);
  ASSERT(message != NULL);

  wds::rtsp::Reply* reply = static_cast<wds::rtsp::Reply*>(message.get());
  ASSERT(reply != NULL);
  ASSERT_EQUAL(reply->response_code(), 303);

  auto payload = ToPropertyErrorPayload(message->payload());
  ASSERT(payload);
  std::shared_ptr<wds::rtsp::PropertyErrors> error;
  ASSERT_EQUAL (payload->property_errors().size(), 2);
  ASSERT_NO_EXCEPTION(error =
      payload->GetPropertyError(wds::rtsp::AudioCodecsPropertyType));
  ASSERT_EQUAL(error->error_codes().size(), 2);
  ASSERT_EQUAL(error->error_codes()[0], 415);
  ASSERT_EQUAL(error->error_codes()[1], 457);

  ASSERT_NO_EXCEPTION(error =
      payload->GetPropertyError(wds::rtsp::I2CPropertyType));
  ASSERT_EQUAL(error->error_codes().size(), 1);
  ASSERT_EQUAL(error->error_codes()[0], 404);

  ASSERT_EQUAL(message->ToString(), header + payload_buffer);

  return true;
}

static bool test_valid_set_parameter ()
{
  std::string header("SET_PARAMETER rtsp://localhost/wfd1.0 RTSP/1.0\r\n"
                     "CSeq: 3\r\n"
                     "Content-Type: text/parameters\r\n"
                     "Content-Length: 275\r\n\r\n");
  std::string payload_buffer("wfd_audio_codecs: AAC 00000001 00\r\n"
                      "wfd_client_rtp_ports: RTP/AVP/UDP;unicast 19000 0 mode=play\r\n"
                      "wfd_presentation_URL: rtsp://192.168.173.1/wfd1.0/streamid=0 none\r\n"
                      "wfd_trigger_method: SETUP\r\n"
                      "wfd_video_formats: 5A 00 02 04 00000020 00000000 00000000 00 0000 0000 11 none none\r\n");

  std::unique_ptr<wds::rtsp::Message> message;
  Driver::Parse(header, message);
  ASSERT(message != NULL);
  ASSERT(message->is_request());
  Driver::Parse(payload_buffer, message);
  ASSERT(message != NULL);
  wds::rtsp::Request* request = wds::rtsp::ToRequest(message.get());
  ASSERT_EQUAL(request->method(), wds::rtsp::Request::MethodSetParameter);
  ASSERT_EQUAL(request->request_uri(), "rtsp://localhost/wfd1.0");
  ASSERT_EQUAL(request->header().cseq(), 3);
  ASSERT_EQUAL(request->header().content_length(), 275);
  ASSERT_EQUAL(request->header().require_wfd_support(), false);

  auto payload = ToPropertyMapPayload(message->payload());
  ASSERT(payload);
  std::shared_ptr<wds::rtsp::Property> prop;

  ASSERT_NO_EXCEPTION (prop =
      payload->GetProperty(wds::rtsp::AudioCodecsPropertyType));
  std::shared_ptr<wds::rtsp::AudioCodecs> audio_codecs = std::static_pointer_cast<wds::rtsp::AudioCodecs> (prop);
  ASSERT_EQUAL(audio_codecs->audio_codecs().size(), 1);
  ASSERT(test_audio_codec (audio_codecs->audio_codecs()[0],
                           wds::AAC, 1, 0));

  ASSERT_NO_EXCEPTION (prop =
      payload->GetProperty(wds::rtsp::VideoFormatsPropertyType));
  std::shared_ptr<wds::rtsp::VideoFormats> video_formats = std::static_pointer_cast<wds::rtsp::VideoFormats> (prop);
  ASSERT_EQUAL(video_formats->GetNativeFormat().rate_resolution, 11);
  ASSERT_EQUAL(video_formats->GetNativeFormat().type, 2);
  ASSERT_EQUAL(video_formats->GetH264Formats().size(), 1);

  ASSERT_NO_EXCEPTION (prop =
      payload->GetProperty(wds::rtsp::ClientRTPPortsPropertyType));
  std::shared_ptr<wds::rtsp::ClientRtpPorts> client_rtp_ports = std::static_pointer_cast<wds::rtsp::ClientRtpPorts> (prop);
  ASSERT_EQUAL(client_rtp_ports->rtp_port_0(), 19000);
  ASSERT_EQUAL(client_rtp_ports->rtp_port_1(), 0);

  ASSERT_NO_EXCEPTION (prop =
      payload->GetProperty(wds::rtsp::TriggerMethodPropertyType));
  std::shared_ptr<wds::rtsp::TriggerMethod> trigger_method = std::static_pointer_cast<wds::rtsp::TriggerMethod> (prop);
  ASSERT_EQUAL(trigger_method->method(), wds::rtsp::TriggerMethod::SETUP);

  ASSERT_NO_EXCEPTION (prop =
      payload->GetProperty(wds::rtsp::PresentationURLPropertyType));

  ASSERT_EQUAL(request->ToString(), header + payload_buffer);

  return true;
}

static bool test_valid_set_parameter_url_with_port ()
{
  std::string header("SET_PARAMETER rtsp://localhost/wfd1.0 RTSP/1.0\r\n"
                     "CSeq: 3\r\n"
                     "Content-Type: text/parameters\r\n"
                     "Content-Length: 111\r\n\r\n");
  std::string payload_buffer("wfd_presentation_URL: rtsp://192.168.173.1:3921/wfd1.0/streamid=0 rtsp://192.168.173.1:3922/wfd1.0/streamid=1\r\n");

  std::unique_ptr<wds::rtsp::Message> message;
  Driver::Parse(header, message);
  ASSERT(message != NULL);
  ASSERT(message->is_request());
  Driver::Parse(payload_buffer, message);
  ASSERT(message != NULL);
  wds::rtsp::Request* request = wds::rtsp::ToRequest(message.get());
  ASSERT_EQUAL(request->method(), wds::rtsp::Request::MethodSetParameter);
  ASSERT_EQUAL(request->request_uri(), "rtsp://localhost/wfd1.0");
  ASSERT_EQUAL(request->header().cseq(), 3);
  ASSERT_EQUAL(request->header().content_length(), 111);
  ASSERT_EQUAL(request->header().require_wfd_support(), false);

  auto payload = ToPropertyMapPayload(message->payload());
  ASSERT(payload);
  std::shared_ptr<wds::rtsp::Property> prop;

  ASSERT_NO_EXCEPTION (prop =
      payload->GetProperty(wds::rtsp::PresentationURLPropertyType));

  ASSERT_EQUAL(request->ToString(), header + payload_buffer);

  return true;
}

static bool test_valid_setup ()
{
  std::string header("SETUP rtsp://10.82.24.140/wfd1.0/streamid=0 RTSP/1.0\r\n"
                      "CSeq: 4\r\n"
                      "Transport: RTP/AVP/UDP;unicast;client_port=19000\r\n"
                      "User-Agent: SEC-WDH/ME29\r\n"
                      "\r\n");

  std::unique_ptr<wds::rtsp::Message> message;
  Driver::Parse(header, message);
  ASSERT(message != NULL);
  ASSERT(message->is_request());
  wds::rtsp::Request* request = wds::rtsp::ToRequest(message.get());
  ASSERT_EQUAL(request->method(), wds::rtsp::Request::MethodSetup);
  ASSERT_EQUAL(request->request_uri(), "rtsp://10.82.24.140/wfd1.0/streamid=0");
  ASSERT_EQUAL(request->header().cseq(), 4);
  ASSERT_EQUAL(request->header().content_length(), 0);
  ASSERT_EQUAL(request->header().require_wfd_support(), false);

  ASSERT_EQUAL(request->header().transport().client_port(), 19000);
  ASSERT_EQUAL(request->header().transport().client_supports_rtcp(), false);
  ASSERT_EQUAL(request->header().transport().server_port(), 0);
  ASSERT_EQUAL(request->header().transport().server_supports_rtcp(), false);

  ASSERT_EQUAL(request->ToString(), header);

  return true;
}

static bool test_valid_setup_reply ()
{
  std::string header("RTSP/1.0 200 OK\r\n"
                     "CSeq: 4\r\n"
                     "Session: 6B8B4567;timeout=30\r\n"
                     "Transport: RTP/AVP/UDP;unicast;client_port=19000;server_port=5000-5001\r\n\r\n");
  std::unique_ptr<wds::rtsp::Message> message;
  Driver::Parse(header, message);
  ASSERT(message != NULL);
  ASSERT(message->is_reply());

  wds::rtsp::Reply* reply = static_cast<wds::rtsp::Reply*>(message.get());
  ASSERT(reply != NULL);

  ASSERT_EQUAL(reply->response_code(), 200);
  ASSERT_EQUAL(reply->header().cseq(), 4);
  ASSERT_EQUAL(reply->header().content_length(), 0);
  ASSERT_EQUAL(reply->header().session(), "6B8B4567");
  ASSERT_EQUAL(reply->header().timeout(), 30);

  ASSERT_EQUAL(reply->header().transport().client_port(), 19000);
  ASSERT_EQUAL(reply->header().transport().client_supports_rtcp(), false);
  ASSERT_EQUAL(reply->header().transport().server_port(), 5000);
  ASSERT_EQUAL(reply->header().transport().server_supports_rtcp(), true);

  ASSERT_EQUAL(reply->ToString(), header);

  return true;
}

static bool test_valid_play ()
{
  std::string header("PLAY rtsp://localhost/wfd1.0 RTSP/1.0\r\n"
                     "CSeq: 5\r\n"
                     "Session: 6B8B4567\r\n"
                     "User-Agent: SEC-WDH/ME29\r\n\r\n");
  std::unique_ptr<wds::rtsp::Message> message;
  Driver::Parse(header, message);
  ASSERT(message != NULL);
  ASSERT(message->is_request());
  wds::rtsp::Request* request = wds::rtsp::ToRequest(message.get());
  ASSERT_EQUAL(request->method(), wds::rtsp::Request::MethodPlay);
  ASSERT_EQUAL(request->request_uri(), "rtsp://localhost/wfd1.0");
  ASSERT_EQUAL(request->header().cseq(), 5);
  ASSERT_EQUAL(request->header().content_length(), 0);
  ASSERT_EQUAL(request->header().require_wfd_support(), false);
  ASSERT_EQUAL(request->header().session(), "6B8B4567");

  ASSERT_EQUAL(request->ToString(), header);

  return true;
}

static bool test_number_conversion_header()
{
  std::string header("OPTIONS * RTSP/1.0\r\n"
                     "CSeq: 92233720368547758079223372036854775807\r\n"
                     "Require: org.wfa.wfd1.0\r\n\r\n");
  std::unique_ptr<wds::rtsp::Message> message;
  Driver::Parse(header, message);
  ASSERT(message == NULL);
  return true;
}

static bool test_number_conversion_body()
{
  std::string header("RTSP/1.0 200 OK\r\n"
                     "CSeq: 2\r\n"
                     "Content-Type: text/parameters\r\n"
                     "Content-Length: 101\r\n\r\n");
  std::string payload_buffer("wfd_client_rtp_ports: RTP/AVP/UDP;unicast 92233720368547758079223372036854775807 0 mode=play\r\n");
  std::unique_ptr<wds::rtsp::Message> message;
  Driver::Parse(header, message);
  ASSERT(message != NULL);
  ASSERT(message->is_reply());
  Driver::Parse(payload_buffer, message);
  ASSERT(message == NULL);

  return true;
}

static bool test_hex_number_conversion_body()
{
  std::string header("RTSP/1.0 200 OK\r\n"
                     "CSeq: 2\r\n"
                     "Content-Type: text/parameters\r\n"
                     "Content-Length: 101\r\n\r\n");
  std::string payload_buffer("wfd_audio_codecs: AAC 92233720368547758079223372036854775807 00\r\n");
  std::unique_ptr<wds::rtsp::Message> message;
  Driver::Parse(header, message);
  ASSERT(message != NULL);
  ASSERT(message->is_reply());
  Driver::Parse(payload_buffer, message);
  ASSERT(message == NULL);

  return true;
}

static bool test_hex_number_conversion_body_2()
{
  std::string header("RTSP/1.0 200 OK\r\n"
                     "CSeq: 2\r\n"
                     "Content-Type: text/parameters\r\n"
                     "Content-Length: 101\r\n\r\n");
  std::string payload_buffer("wfd_audio_codecs: AAC FFFFFFFFFFFFFFFFF 00\r\n");
  std::unique_ptr<wds::rtsp::Message> message;
  Driver::Parse(header, message);
  ASSERT(message != NULL);
  ASSERT(message->is_reply());
  Driver::Parse(payload_buffer, message);
  ASSERT(message == NULL);

  return true;
}

static bool test_number_conversion_in_errors ()
{
  std::string header("RTSP/1.0 303 OK\r\n"
                     "CSeq: 0\r\n"
                     "Content-Type: text/parameters\r\n"
                     "Content-Length: 55\r\n\r\n");

  std::unique_ptr<wds::rtsp::Message> message;
  Driver::Parse(header, message);
  ASSERT(message != NULL);
  ASSERT(message->is_reply());

  std::string payload_buffer("wfd_audio_codecs: 92233720368547758079223372036854775807\r\n");
  Driver::Parse(payload_buffer, message);
  ASSERT(message == NULL);

  return true;
}

int main(const int argc, const char **argv)
{
  std::list<TestFunc> tests;
  int failures = 0;

  // Add tests
  tests.push_back(test_valid_options);
  tests.push_back(test_valid_options_reply);
  tests.push_back(test_valid_get_parameter);
  tests.push_back(test_valid_get_parameter_reply);
  tests.push_back(test_valid_get_parameter_reply_with_all_none);
  tests.push_back(test_valid_get_parameter_reply_with_errors);
  tests.push_back(test_valid_setup_reply);
  tests.push_back(test_valid_set_parameter);
  tests.push_back(test_valid_set_parameter_url_with_port);
  tests.push_back(test_valid_setup);
  tests.push_back(test_valid_play);
  tests.push_back(test_invalid_property_value);
  tests.push_back(test_case_insensitivity);
  tests.push_back(test_valid_extra_properties);
  tests.push_back(test_valid_extra_errors);
  tests.push_back(test_valid_extra_properties_in_get);
  tests.push_back(test_number_conversion_header);
  tests.push_back(test_number_conversion_body);
  tests.push_back(test_hex_number_conversion_body);
  tests.push_back(test_hex_number_conversion_body_2);
  tests.push_back(test_number_conversion_in_errors);

  // Run tests
  for (std::list<TestFunc>::iterator it=tests.begin(); it!=tests.end(); ++it) {
    TestFunc test = *it;
    if (!test())
      failures++;
  }

  if (failures > 0) {
    std::cout << std::endl << "Failed " << failures
              << " out of " << tests.size() << " tests" << std::endl;
    return 1;
  }

  std::cout << "Passed all " << tests.size() << " tests" << std::endl;
  return 0;
}
