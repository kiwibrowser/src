// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_AUDIO_TEST_FAKE_GROUP_MEMBER_H_
#define SERVICES_AUDIO_TEST_FAKE_GROUP_MEMBER_H_

#include <memory>
#include <vector>

#include "base/macros.h"
#include "base/time/time.h"
#include "base/unguessable_token.h"
#include "media/base/audio_parameters.h"
#include "services/audio/group_member.h"

namespace media {
class AudioBus;
}

namespace audio {

// An implementation of GroupMember that can be snooped upon. It generates sine
// wave tones, configurable per channel. Test procedures call RenderMoreAudio()
// to push more data to the Snooper.
//
// This class is not thread-safe. The caller must guarantee method calls are not
// being made simultaneously in multithreaded tests.
class FakeGroupMember : public GroupMember {
 public:
  FakeGroupMember(const base::UnguessableToken& group_id,
                  const media::AudioParameters& params);

  ~FakeGroupMember() override;

  // Sets the sine wave |frequency| rendered into channel |ch|. Note that
  // setting the frequency to zero will zero-out the channel signal.
  void SetChannelTone(int ch, double frequency);

  // Sets the volume of this FakeGroupMember. This simulates the current output
  // volume of an audio::OutputStream.
  void SetVolume(double volume);

  // Renders a continuation of the sine wave signal, attaching
  // |output_timestamp| as the timestamp associated with the first frame in the
  // AudioBus being delivered to the Snooper.
  void RenderMoreAudio(base::TimeTicks output_timestamp);

  // GroupMember implementation.
  const base::UnguessableToken& GetGroupId() override;
  const media::AudioParameters& GetAudioParameters() override;
  void StartSnooping(Snooper* snooper) override;
  void StopSnooping(Snooper* snooper) override;
  void StartMuting() override;
  void StopMuting() override;

 private:
  const base::UnguessableToken group_id_;
  const media::AudioParameters params_;
  const std::unique_ptr<media::AudioBus> audio_bus_;

  std::vector<double> frequency_by_channel_;
  double volume_ = 0.0;

  int64_t at_frame_ = 0;

  Snooper* snooper_ = nullptr;

  DISALLOW_COPY_AND_ASSIGN(FakeGroupMember);
};

}  // namespace audio

#endif  // SERVICES_AUDIO_TEST_FAKE_GROUP_MEMBER_H_
