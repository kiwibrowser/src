// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_AUDIO_GROUP_MEMBER_H_
#define SERVICES_AUDIO_GROUP_MEMBER_H_

#include "base/time/time.h"

namespace base {
class UnguessableToken;
}

namespace media {
class AudioBus;
class AudioParameters;
}  // namespace media

namespace audio {

// Interface for accessing signal data and controlling a members of an audio
// group. A group is defined by a common group identifier that all members
// share.
//
// The purpose of the grouping concept is to allow a feature to identify all
// audio flows that come from the same logical unit, such as a browser tab. The
// audio flows can then be duplicated, or other group-wide control exercised on
// all members (such as audio muting).
class GroupMember {
 public:
  class Snooper {
   public:
    // Provides read-only access to the data flowing through a GroupMember.
    virtual void OnData(const media::AudioBus& audio_bus,
                        base::TimeTicks reference_time,
                        double volume) = 0;

   protected:
    virtual ~Snooper() = default;
  };

  // Returns the string identifier of the group. This must not change for the
  // lifetime of this group member.
  virtual const base::UnguessableToken& GetGroupId() = 0;

  // Returns the audio parameters of the snoopable audio data. The parameters
  // must not change for the lifetime of this group member, but can be different
  // than those of other members.
  virtual const media::AudioParameters& GetAudioParameters() = 0;

  // Starts/Stops snooping on the audio data flowing through this group member.
  virtual void StartSnooping(Snooper* snooper) = 0;
  virtual void StopSnooping(Snooper* snooper) = 0;

  // Starts/Stops muting of the outbound audio signal from this group member.
  // However, the audio data being sent to Snoopers should be the original,
  // unmuted audio. Note that an equal number of start versus stop calls here is
  // not required, and the implementation should ignore redundant calls.
  virtual void StartMuting() = 0;
  virtual void StopMuting() = 0;

 protected:
  virtual ~GroupMember() = default;
};

}  // namespace audio

#endif  // SERVICES_AUDIO_GROUP_MEMBER_H_
