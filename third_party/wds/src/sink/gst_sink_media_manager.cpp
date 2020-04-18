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

#include "gst_sink_media_manager.h"

GstSinkMediaManager::GstSinkMediaManager(const std::string& hostname)
  : gst_pipeline_(new MiracGstSink(hostname, 0)) {
}

void GstSinkMediaManager::Play() {
  gst_pipeline_->Play();
}

void GstSinkMediaManager::Pause() {
  gst_pipeline_->Pause();
}

void GstSinkMediaManager::Teardown() {
  gst_pipeline_->Teardown();
}

bool GstSinkMediaManager::IsPaused() const {
  return gst_pipeline_->IsPaused();
}

std::pair<int,int> GstSinkMediaManager::GetLocalRtpPorts() const {
  return std::pair<int,int>(gst_pipeline_->sink_udp_port(), 0);
}

void GstSinkMediaManager::SetPresentationUrl(const std::string& url) {
  presentation_url_ = url;
}

std::string GstSinkMediaManager::GetPresentationUrl() const {
  return presentation_url_;
}

void GstSinkMediaManager::SetSessionId(const std::string& session) {
  session_ = session;
}

std::string GstSinkMediaManager::GetSessionId() const {
  return session_;
}

std::vector<wds::H264VideoCodec>
GstSinkMediaManager::GetSupportedH264VideoCodecs() const {
  wds::RateAndResolutionsBitmap cea_rr;
  wds::RateAndResolutionsBitmap vesa_rr;
  wds::RateAndResolutionsBitmap hh_rr;
  wds::RateAndResolution i;
  // declare that we support all resolutions, CHP and level 4.2
  // gstreamer should handle all of it :)
  for (i = wds::CEA640x480p60; i <= wds::CEA1920x1080p24; ++i)
    cea_rr.set(i);
  for (i = wds::VESA800x600p30; i <= wds::VESA1920x1200p30; ++i)
    vesa_rr.set(i);
  for (i = wds::HH800x480p30; i <= wds::HH848x480p60; ++i)
    hh_rr.set(i);
  return {wds::H264VideoCodec(wds::CHP, wds::k4_2, cea_rr, vesa_rr, hh_rr),
          wds::H264VideoCodec(wds::CBP, wds::k4_2, cea_rr, vesa_rr, hh_rr)};
}

wds::NativeVideoFormat GstSinkMediaManager::GetNativeVideoFormat() const {
  // pick the maximum possible resolution, let gstreamer deal with it
  // TODO: get the actual screen size of the system
  return wds::NativeVideoFormat(wds::CEA1920x1080p60);
}

bool GstSinkMediaManager::SetOptimalVideoFormat(const wds::H264VideoFormat& optimal_format) {
  return true;
}

wds::ConnectorType GstSinkMediaManager::GetConnectorType() const {
  return wds::ConnectorTypeNone;
}
