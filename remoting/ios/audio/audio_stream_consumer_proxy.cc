// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "remoting/ios/audio/audio_stream_consumer_proxy.h"

#include <utility>

#include "base/bind.h"
#include "base/callback_helpers.h"
#include "base/location.h"
#include "base/logging.h"
#include "base/macros.h"
#include "base/single_thread_task_runner.h"

namespace remoting {

// Runs an instance of |HostWindow| on the |audio_task_runner_| thread.
class AudioStreamConsumerProxy::Core {
 public:
  Core(base::WeakPtr<AudioStreamConsumer> audio_stream_consumer);
  ~Core();

  void AddAudioPacket(std::unique_ptr<AudioPacket> packet);
  base::WeakPtr<AudioStreamConsumer> AudioStreamConsumerAsWeakPtr();
  base::WeakPtr<Core> GetWeakPtr();

 private:
  // The wrapped |AudioStreamConsumer| instance running on the
  // |audio_task_runner_| thread.
  base::WeakPtr<AudioStreamConsumer> audio_stream_consumer_;

  base::WeakPtrFactory<Core> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(Core);
};

std::unique_ptr<AudioStreamConsumerProxy> AudioStreamConsumerProxy::Create(
    scoped_refptr<base::SingleThreadTaskRunner> audio_task_runner,
    base::WeakPtr<AudioStreamConsumer> audio_stream_consumer) {
  std::unique_ptr<Core> core(new Core(std::move(audio_stream_consumer)));
  std::unique_ptr<AudioStreamConsumerProxy> result(
      new AudioStreamConsumerProxy(audio_task_runner, std::move(core)));
  return result;
}

AudioStreamConsumerProxy::AudioStreamConsumerProxy(
    scoped_refptr<base::SingleThreadTaskRunner> audio_task_runner,
    std::unique_ptr<Core> core)
    : audio_task_runner_(audio_task_runner),
      core_(std::move(core)),
      weak_factory_(this) {}

AudioStreamConsumerProxy::~AudioStreamConsumerProxy() {
  audio_task_runner_->DeleteSoon(FROM_HERE, core_.release());
}

void AudioStreamConsumerProxy::AddAudioPacket(
    std::unique_ptr<AudioPacket> packet) {
  audio_task_runner_->PostTask(
      FROM_HERE, base::Bind(&Core::AddAudioPacket, core_->GetWeakPtr(),
                            base::Passed(&packet)));
}

base::WeakPtr<AudioStreamConsumer>
AudioStreamConsumerProxy::AudioStreamConsumerAsWeakPtr() {
  return weak_factory_.GetWeakPtr();
}

AudioStreamConsumerProxy::Core::Core(
    base::WeakPtr<AudioStreamConsumer> audio_stream_consumer)
    : audio_stream_consumer_(audio_stream_consumer), weak_factory_(this) {}

AudioStreamConsumerProxy::Core::~Core() {}

void AudioStreamConsumerProxy::Core::AddAudioPacket(
    std::unique_ptr<AudioPacket> packet) {
  if (audio_stream_consumer_) {
    audio_stream_consumer_->AddAudioPacket(std::move(packet));
  }
}

base::WeakPtr<AudioStreamConsumerProxy::Core>
AudioStreamConsumerProxy::Core::GetWeakPtr() {
  return weak_factory_.GetWeakPtr();
}

}  // namespace remoting
