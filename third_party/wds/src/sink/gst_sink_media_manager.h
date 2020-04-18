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

#ifndef GST_SINK_MEDIA_MANAGER_H_
#define GST_SINK_MEDIA_MANAGER_H_

#include <memory>

#include "libwds/public/media_manager.h"
#include "mirac-gst-sink.hpp"

class GstSinkMediaManager : public wds::SinkMediaManager {
 public:
  explicit GstSinkMediaManager(const std::string& hostname);

  void Play() override;
  void Pause() override;
  void Teardown() override;
  bool IsPaused() const override;
  std::pair<int,int> GetLocalRtpPorts() const override;
  void SetPresentationUrl(const std::string& url) override;
  std::string GetPresentationUrl() const override;
  void SetSessionId(const std::string& session) override;
  std::string GetSessionId() const override;

  std::vector<wds::H264VideoCodec> GetSupportedH264VideoCodecs() const override;
  wds::NativeVideoFormat GetNativeVideoFormat() const override;
  bool SetOptimalVideoFormat(const wds::H264VideoFormat& optimal_format) override;
  wds::ConnectorType GetConnectorType() const override;

 private:
  std::string hostname_;
  std::string presentation_url_;
  std::string session_;
  std::unique_ptr<MiracGstSink> gst_pipeline_;
};

#endif // GST_SINK_MEDIA_MANAGER_H_
