// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_AUDIO_PUBLIC_CPP_DEVICE_FACTORY_H_
#define SERVICES_AUDIO_PUBLIC_CPP_DEVICE_FACTORY_H_

#include "media/audio/audio_input_device.h"

#include "services/audio/public/mojom/stream_factory.mojom.h"
#include "services/service_manager/public/cpp/connector.h"

namespace audio {

scoped_refptr<media::AudioCapturerSource> CreateInputDevice(
    std::unique_ptr<service_manager::Connector> connector,
    const std::string& device_id);

scoped_refptr<media::AudioCapturerSource> CreateInputDevice(
    std::unique_ptr<service_manager::Connector> connector,
    const std::string& device_id,
    media::mojom::AudioLogPtr);

}  // namespace audio

#endif  // SERVICES_AUDIO_PUBLIC_CPP_DEVICE_FACTORY_H_
