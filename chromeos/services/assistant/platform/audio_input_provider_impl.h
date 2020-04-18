// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROMEOS_SERVICES_ASSISTANT_PLATFORM_AUDIO_INPUT_PROVIDER_IMPL_H_
#define CHROMEOS_SERVICES_ASSISTANT_PLATFORM_AUDIO_INPUT_PROVIDER_IMPL_H_

#include <memory>
#include <vector>

#include "base/macros.h"
#include "base/synchronization/lock.h"
#include "base/time/time.h"
#include "chromeos/services/assistant/public/mojom/assistant.mojom.h"
#include "libassistant/shared/public/platform_audio_input.h"
#include "mojo/public/cpp/bindings/binding.h"

namespace chromeos {
namespace assistant {

class AudioInputImpl : public assistant_client::AudioInput,
                       public mojom::AudioInputObserver {
 public:
  explicit AudioInputImpl(mojom::AudioInputPtr audio_input);
  ~AudioInputImpl() override;

  // mojom::AudioInputObserver overrides:
  void OnAudioInputFramesAvailable(const std::vector<int32_t>& buffer,
                                   uint32_t frame_count,
                                   base::TimeTicks timestamp) override;
  void OnAudioInputClosed() override;

  // assistant_client::AudioInput overrides:
  assistant_client::BufferFormat GetFormat() const override;
  void AddObserver(assistant_client::AudioInput::Observer* observer) override;
  void RemoveObserver(
      assistant_client::AudioInput::Observer* observer) override;

 private:
  // Protects |observers_| access becase AssistantManager calls
  // Add/RemoveObserver on its own thread.
  base::Lock lock_;
  std::vector<assistant_client::AudioInput::Observer*> observers_;
  mojo::Binding<mojom::AudioInputObserver> binding_;
  DISALLOW_COPY_AND_ASSIGN(AudioInputImpl);
};

class AudioInputProviderImpl : public assistant_client::AudioInputProvider {
 public:
  explicit AudioInputProviderImpl(mojom::AudioInputPtr audio_input);
  ~AudioInputProviderImpl() override;

  // assistant_client::AudioInputProvider overrides:
  assistant_client::AudioInput& GetAudioInput() override;
  int64_t GetCurrentAudioTime() override;

 private:
  AudioInputImpl audio_input_;

  DISALLOW_COPY_AND_ASSIGN(AudioInputProviderImpl);
};

}  // namespace assistant
}  // namespace chromeos

#endif  // CHROMEOS_SERVICES_ASSISTANT_PLATFORM_AUDIO_INPUT_PROVIDER_IMPL_H_
