// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_MODULES_MEDIA_CONTROLS_MEDIA_CONTROLS_MEDIA_EVENT_LISTENER_H_
#define THIRD_PARTY_BLINK_RENDERER_MODULES_MEDIA_CONTROLS_MEDIA_CONTROLS_MEDIA_EVENT_LISTENER_H_

#include "base/optional.h"
#include "third_party/blink/renderer/core/dom/events/event_listener.h"

namespace blink {

class HTMLMediaElement;
class MediaControlsImpl;
class RemotePlayback;

class MediaControlsMediaEventListener final : public EventListener {
 public:
  explicit MediaControlsMediaEventListener(MediaControlsImpl*);

  // Called by MediaControls when the HTMLMediaElement is added to a document
  // document. All event listeners should be added.
  void Attach();

  // Called by MediaControls when the HTMLMediaElement is no longer in the
  // document. All event listeners should be removed in order to prepare the
  // object to be garbage collected.
  void Detach();

  bool operator==(const EventListener&) const override;

  void Trace(blink::Visitor*) override;

 private:
  HTMLMediaElement& GetMediaElement();
  RemotePlayback* GetRemotePlayback();

  void handleEvent(ExecutionContext*, Event*) override;

  void OnRemotePlaybackAvailabilityChanged();

  Member<MediaControlsImpl> media_controls_;
  base::Optional<int> remote_playback_availability_callback_id_;
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_MODULES_MEDIA_CONTROLS_MEDIA_CONTROLS_MEDIA_EVENT_LISTENER_H_
