// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "remoting/protocol/audio_decode_scheduler.h"

#include <utility>

#include "base/bind.h"
#include "base/location.h"
#include "base/single_thread_task_runner.h"
#include "base/task_runner_util.h"
#include "remoting/codec/audio_decoder.h"
#include "remoting/proto/audio.pb.h"
#include "remoting/protocol/audio_stub.h"

namespace remoting {
namespace protocol {

AudioDecodeScheduler::AudioDecodeScheduler(
    scoped_refptr<base::SingleThreadTaskRunner> audio_decode_task_runner,
    base::WeakPtr<protocol::AudioStub> audio_consumer)
    : audio_decode_task_runner_(audio_decode_task_runner),
      audio_consumer_(audio_consumer),
      weak_factory_(this) {}

AudioDecodeScheduler::~AudioDecodeScheduler() {
  DCHECK(thread_checker_.CalledOnValidThread());
  audio_decode_task_runner_->DeleteSoon(FROM_HERE, decoder_.release());
}

void AudioDecodeScheduler::Initialize(const protocol::SessionConfig& config) {
  DCHECK(thread_checker_.CalledOnValidThread());
  DCHECK(!decoder_);
  decoder_ = AudioDecoder::CreateAudioDecoder(config);
}

void AudioDecodeScheduler::ProcessAudioPacket(
    std::unique_ptr<AudioPacket> packet,
    const base::Closure& done) {
  DCHECK(thread_checker_.CalledOnValidThread());

  base::PostTaskAndReplyWithResult(
      audio_decode_task_runner_.get(), FROM_HERE,
      base::Bind(&AudioDecoder::Decode, base::Unretained(decoder_.get()),
                 base::Passed(&packet)),
      base::Bind(&AudioDecodeScheduler::ProcessDecodedPacket,
                 weak_factory_.GetWeakPtr(), done));
}

void AudioDecodeScheduler::ProcessDecodedPacket(
    const base::Closure& done,
    std::unique_ptr<AudioPacket> packet) {
  DCHECK(thread_checker_.CalledOnValidThread());

  if (!packet || !audio_consumer_) {
    done.Run();
    return;
  }

  audio_consumer_->ProcessAudioPacket(std::move(packet), done);
}

}  // namespace protocol
}  // namespace remoting
