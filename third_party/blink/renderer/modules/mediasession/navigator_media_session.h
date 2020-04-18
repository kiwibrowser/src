// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_MODULES_MEDIASESSION_NAVIGATOR_MEDIA_SESSION_H_
#define THIRD_PARTY_BLINK_RENDERER_MODULES_MEDIASESSION_NAVIGATOR_MEDIA_SESSION_H_

#include "third_party/blink/renderer/core/frame/navigator.h"
#include "third_party/blink/renderer/modules/mediasession/media_session.h"
#include "third_party/blink/renderer/platform/supplementable.h"

namespace blink {

class Navigator;
class ScriptState;

// Provides MediaSession as a supplement of Navigator as an attribute.
class NavigatorMediaSession final
    : public GarbageCollected<NavigatorMediaSession>,
      public Supplement<Navigator> {
  USING_GARBAGE_COLLECTED_MIXIN(NavigatorMediaSession);

 public:
  static const char kSupplementName[];

  static NavigatorMediaSession& From(Navigator&);
  static MediaSession* mediaSession(ScriptState*, Navigator&);

  void Trace(blink::Visitor*) override;

 private:
  explicit NavigatorMediaSession(Navigator&);

  // The MediaSession instance of this Navigator.
  Member<MediaSession> session_;
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_MODULES_MEDIASESSION_NAVIGATOR_MEDIA_SESSION_H_
