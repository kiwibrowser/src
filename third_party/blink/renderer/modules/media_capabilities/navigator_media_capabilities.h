// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_MODULES_MEDIA_CAPABILITIES_NAVIGATOR_MEDIA_CAPABILITIES_H_
#define THIRD_PARTY_BLINK_RENDERER_MODULES_MEDIA_CAPABILITIES_NAVIGATOR_MEDIA_CAPABILITIES_H_

#include "third_party/blink/renderer/core/frame/navigator.h"
#include "third_party/blink/renderer/platform/supplementable.h"

namespace blink {

class MediaCapabilities;
class Navigator;

// Provides MediaCapabilities as a supplement of Navigator as an attribute.
class NavigatorMediaCapabilities final
    : public GarbageCollected<NavigatorMediaCapabilities>,
      public Supplement<Navigator> {
  USING_GARBAGE_COLLECTED_MIXIN(NavigatorMediaCapabilities);

 public:
  static const char kSupplementName[];

  static MediaCapabilities* mediaCapabilities(Navigator&);

  void Trace(blink::Visitor*) override;

 private:
  explicit NavigatorMediaCapabilities(Navigator&);

  static NavigatorMediaCapabilities& From(Navigator&);

  // The MediaCapabilities instance of this Navigator.
  Member<MediaCapabilities> capabilities_;
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_MODULES_MEDIA_CAPABILITIES_NAVIGATOR_MEDIA_CAPABILITIES_H_
