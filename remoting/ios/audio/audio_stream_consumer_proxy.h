// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef REMOTING_IOS_AUDIO_AUDIO_STREAM_CONSUMER_PROXY_H_
#define REMOTING_IOS_AUDIO_AUDIO_STREAM_CONSUMER_PROXY_H_

#include <cstdint>
#include <list>
#include <memory>

#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "base/threading/thread_checker.h"
#include "remoting/client/audio/audio_stream_consumer.h"
#include "remoting/proto/audio.pb.h"

namespace base {
class SingleThreadTaskRunner;
}

namespace remoting {

class AudioStreamConsumer;

// Takes an instance of |AudioConsumer| and runs it on the |audio_task_runner|
// thread.
class AudioStreamConsumerProxy : public AudioStreamConsumer {
 public:
  static std::unique_ptr<AudioStreamConsumerProxy> Create(
      scoped_refptr<base::SingleThreadTaskRunner> audio_task_runner,
      base::WeakPtr<AudioStreamConsumer> audio_stream_consumer);

  ~AudioStreamConsumerProxy() override;

  // AudioStreamConsumer overrides.
  void AddAudioPacket(std::unique_ptr<AudioPacket> packet) override;
  base::WeakPtr<AudioStreamConsumer> AudioStreamConsumerAsWeakPtr() override;

 private:
  class Core;
  AudioStreamConsumerProxy(
      scoped_refptr<base::SingleThreadTaskRunner> audio_task_runner,
      std::unique_ptr<Core> core);

  // Task runner on which |core_| is called.
  scoped_refptr<base::SingleThreadTaskRunner> audio_task_runner_;

  // All thread switching logic is implemented in the |Core| class.
  std::unique_ptr<Core> core_;

  base::WeakPtrFactory<AudioStreamConsumerProxy> weak_factory_;
};

}  // namespace remoting

#endif  // REMOTING_IOS_AUDIO_AUDIO_STREAM_CONSUMER_PROXY_H_
