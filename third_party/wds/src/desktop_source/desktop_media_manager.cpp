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
#include "desktop_media_manager.h"
#include "mirac-glib-logging.hpp"
#include <cassert>

DesktopMediaManager::DesktopMediaManager(const std::string& hostname)
  : hostname_(hostname),
    format_() {
}

void DesktopMediaManager::Play() {
  assert(gst_pipeline_);
  gst_pipeline_->SetState(GST_STATE_PLAYING);
}

void DesktopMediaManager::Pause() {
  assert(gst_pipeline_);
  gst_pipeline_->SetState(GST_STATE_PAUSED);
}

void DesktopMediaManager::Teardown() {
  if (gst_pipeline_)
    gst_pipeline_->SetState(GST_STATE_READY);
}

bool DesktopMediaManager::IsPaused() const {
  return (gst_pipeline_->GetState() != GST_STATE_PLAYING);
}

std::string DesktopMediaManager::GetSessionId() const {
  return "abcdefg123456";
}

wds::SessionType DesktopMediaManager::GetSessionType() const {
  return wds::VideoSession;
}

void DesktopMediaManager::SetSinkRtpPorts(int port1, int port2) {
  sink_port1_ = port1;
  sink_port2_ = port2;
  gst_pipeline_.reset(new MiracGstTestSource(WFD_DESKTOP, hostname_, port1));
  gst_pipeline_->SetState(GST_STATE_READY);
}

std::pair<int, int> DesktopMediaManager::GetSinkRtpPorts() const {
  return std::pair<int, int>(sink_port1_, sink_port2_);
}

int DesktopMediaManager::GetLocalRtpPort() const {
  return gst_pipeline_->UdpSourcePort();
}

namespace {

std::vector<wds::H264VideoCodec> GetH264VideoCodecs() {
  static std::vector<wds::H264VideoCodec> codecs;
  if (codecs.empty()) {
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

    wds::H264VideoCodec codec(wds::CHP, wds::k4_2, cea_rr, vesa_rr, hh_rr);
    codecs.push_back(codec);
  }

  return codecs;
}

}

bool DesktopMediaManager::InitOptimalVideoFormat(
    const wds::NativeVideoFormat& sink_native_format,
    const std::vector<wds::H264VideoCodec>& sink_supported_codecs) {

  format_ = wds::FindOptimalVideoFormat(sink_native_format,
                                        GetH264VideoCodecs(),
                                        sink_supported_codecs);
  return true;
}

wds::H264VideoFormat DesktopMediaManager::GetOptimalVideoFormat() const {
  return format_;
}

bool DesktopMediaManager::InitOptimalAudioFormat(const std::vector<wds::AudioCodec>& sink_codecs) {
  for (const auto& codec : sink_codecs) {
     if (codec.format == wds::AAC && codec.modes.test(wds::AAC_48K_16B_2CH))
       return true;
  }
  return false;
}

wds::AudioCodec DesktopMediaManager::GetOptimalAudioFormat() const {
  wds::AudioModes audio_modes;
  audio_modes.set(wds::AAC_48K_16B_2CH);

  return wds::AudioCodec(wds::AAC, audio_modes, 0);
}

void DesktopMediaManager::SendIDRPicture() {
  WDS_WARNING("Unimplemented IDR picture request");
}
