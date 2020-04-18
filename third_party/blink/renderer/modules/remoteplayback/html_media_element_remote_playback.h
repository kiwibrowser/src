// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_MODULES_REMOTEPLAYBACK_HTML_MEDIA_ELEMENT_REMOTE_PLAYBACK_H_
#define THIRD_PARTY_BLINK_RENDERER_MODULES_REMOTEPLAYBACK_HTML_MEDIA_ELEMENT_REMOTE_PLAYBACK_H_

#include "third_party/blink/renderer/core/html/media/html_media_element.h"
#include "third_party/blink/renderer/modules/modules_export.h"
#include "third_party/blink/renderer/platform/heap/handle.h"
#include "third_party/blink/renderer/platform/supplementable.h"

namespace blink {

class HTMLMediaElement;
class QualifiedName;
class RemotePlayback;

// Class used to implement the Remote Playback API. It is a supplement to
// HTMLMediaElement.
class MODULES_EXPORT HTMLMediaElementRemotePlayback final
    : public GarbageCollected<HTMLMediaElementRemotePlayback>,
      public Supplement<HTMLMediaElement> {
  USING_GARBAGE_COLLECTED_MIXIN(HTMLMediaElementRemotePlayback);

 public:
  static const char kSupplementName[];

  static bool FastHasAttribute(const QualifiedName&, const HTMLMediaElement&);
  static void SetBooleanAttribute(const QualifiedName&,
                                  HTMLMediaElement&,
                                  bool);

  static HTMLMediaElementRemotePlayback& From(HTMLMediaElement&);
  static RemotePlayback* remote(HTMLMediaElement&);

  void Trace(blink::Visitor*) override;

 private:
  Member<RemotePlayback> remote_;
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_MODULES_REMOTEPLAYBACK_HTML_MEDIA_ELEMENT_REMOTE_PLAYBACK_H_
