// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_MODULES_MEDIASTREAM_NAVIGATOR_USER_MEDIA_H_
#define THIRD_PARTY_BLINK_RENDERER_MODULES_MEDIASTREAM_NAVIGATOR_USER_MEDIA_H_

#include "third_party/blink/renderer/core/frame/navigator.h"
#include "third_party/blink/renderer/platform/heap/handle.h"
#include "third_party/blink/renderer/platform/supplementable.h"

namespace blink {

class Navigator;
class MediaDevices;

class NavigatorUserMedia final : public GarbageCollected<NavigatorUserMedia>,
                                 public Supplement<Navigator> {
  USING_GARBAGE_COLLECTED_MIXIN(NavigatorUserMedia)
 public:
  static const char kSupplementName[];

  static MediaDevices* mediaDevices(Navigator&);
  void Trace(blink::Visitor*) override;

 private:
  explicit NavigatorUserMedia(Navigator&);
  MediaDevices* GetMediaDevices();
  static NavigatorUserMedia& From(Navigator&);

  Member<MediaDevices> media_devices_;
};

}  // namespace blink

#endif
