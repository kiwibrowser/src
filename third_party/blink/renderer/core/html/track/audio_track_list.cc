// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/core/html/track/audio_track_list.h"

namespace blink {

AudioTrackList* AudioTrackList::Create(HTMLMediaElement& media_element) {
  return new AudioTrackList(media_element);
}

AudioTrackList::~AudioTrackList() = default;

AudioTrackList::AudioTrackList(HTMLMediaElement& media_element)
    : TrackListBase<AudioTrack>(&media_element) {}

bool AudioTrackList::HasEnabledTrack() const {
  for (unsigned i = 0; i < length(); ++i) {
    if (AnonymousIndexedGetter(i)->enabled())
      return true;
  }

  return false;
}

const AtomicString& AudioTrackList::InterfaceName() const {
  return EventTargetNames::AudioTrackList;
}

}  // namespace blink
