// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_MEDIA_SESSION_AUDIO_FOCUS_MANAGER_H_
#define CONTENT_BROWSER_MEDIA_SESSION_AUDIO_FOCUS_MANAGER_H_

#include <list>
#include <unordered_map>

#include "base/memory/singleton.h"
#include "content/common/content_export.h"
#include "content/public/browser/web_contents_observer.h"

namespace content {

class MediaSessionImpl;

class CONTENT_EXPORT AudioFocusManager {
 public:
  enum class AudioFocusType {
    Gain,
    GainTransientMayDuck,
  };

  // Returns Chromium's internal AudioFocusManager.
  static AudioFocusManager* GetInstance();

  void RequestAudioFocus(MediaSessionImpl* media_session, AudioFocusType type);

  void AbandonAudioFocus(MediaSessionImpl* media_session);

 private:
  friend struct base::DefaultSingletonTraits<AudioFocusManager>;
  friend class AudioFocusManagerTest;

  AudioFocusManager();
  ~AudioFocusManager();

  void MaybeRemoveFocusEntry(MediaSessionImpl* media_session);

  // Weak reference of managed MediaSessions. A MediaSession must abandon audio
  // foucs before its destruction.
  std::list<MediaSessionImpl*> audio_focus_stack_;
};

}  // namespace content

#endif  // CONTENT_BROWSER_MEDIA_SESSION_AUDIO_FOCUS_MANAGER_H_
