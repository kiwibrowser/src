// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/audio/public/cpp/device_factory.h"

#include <memory>

#include "base/bind.h"
#include "services/audio/public/cpp/input_ipc.h"

namespace audio {

scoped_refptr<media::AudioCapturerSource> CreateInputDevice(
    std::unique_ptr<service_manager::Connector> connector,
    const std::string& device_id,
    media::mojom::AudioLogPtr log) {
  std::unique_ptr<media::AudioInputIPC> ipc = std::make_unique<InputIPC>(
      std::move(connector), device_id, std::move(log));

  return base::MakeRefCounted<media::AudioInputDevice>(std::move(ipc));
}

scoped_refptr<media::AudioCapturerSource> CreateInputDevice(
    std::unique_ptr<service_manager::Connector> connector,
    const std::string& device_id) {
  std::unique_ptr<media::AudioInputIPC> ipc =
      std::make_unique<InputIPC>(std::move(connector), device_id, nullptr);

  return base::MakeRefCounted<media::AudioInputDevice>(std::move(ipc));
}

}  // namespace audio
