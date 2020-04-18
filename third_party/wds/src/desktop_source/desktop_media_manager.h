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
#ifndef DESKTOP_MEDIA_MANAGER_H_
#define DESKTOP_MEDIA_MANAGER_H_

#include <memory>

#include "libwds/public/media_manager.h"
#include "mirac-gst-test-source.hpp"

class DesktopMediaManager : public wds::SourceMediaManager {
 public:
  explicit DesktopMediaManager(const std::string& hostname);
  void Play() override;
  void Pause() override;
  void Teardown() override;
  bool IsPaused() const override;
  std::string GetSessionId() const override;
  void SetSinkRtpPorts(int port1, int port2) override;
  std::pair<int,int> GetSinkRtpPorts() const override;
  int GetLocalRtpPort() const override;
  wds::SessionType GetSessionType() const override;

  bool InitOptimalVideoFormat(const wds::NativeVideoFormat& sink_native_format,
      const std::vector<wds::H264VideoCodec>& sink_supported_codecs) override;
  wds::H264VideoFormat GetOptimalVideoFormat() const override;
  bool InitOptimalAudioFormat(const std::vector<wds::AudioCodec>& sink_supported_codecs) override;
  wds::AudioCodec GetOptimalAudioFormat() const override;
  void SendIDRPicture() override;

 private:
  std::string hostname_;
  std::unique_ptr<MiracGstTestSource> gst_pipeline_;
  int sink_port1_;
  int sink_port2_;
  wds::H264VideoFormat format_;
};

#endif // DESKTOP_MEDIA_MANAGER_H_
