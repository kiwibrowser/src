// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/modules/media_capabilities/navigator_media_capabilities.h"

#include "third_party/blink/renderer/modules/media_capabilities/media_capabilities.h"
#include "third_party/blink/renderer/platform/supplementable.h"

namespace blink {

// static
const char NavigatorMediaCapabilities::kSupplementName[] =
    "NavigatorMediaCapabilities";

MediaCapabilities* NavigatorMediaCapabilities::mediaCapabilities(
    Navigator& navigator) {
  NavigatorMediaCapabilities& self =
      NavigatorMediaCapabilities::From(navigator);
  if (!self.capabilities_)
    self.capabilities_ = new MediaCapabilities();
  return self.capabilities_.Get();
}

void NavigatorMediaCapabilities::Trace(blink::Visitor* visitor) {
  visitor->Trace(capabilities_);
  Supplement<Navigator>::Trace(visitor);
}

NavigatorMediaCapabilities::NavigatorMediaCapabilities(Navigator& navigator)
    : Supplement<Navigator>(navigator) {}

NavigatorMediaCapabilities& NavigatorMediaCapabilities::From(
    Navigator& navigator) {
  NavigatorMediaCapabilities* supplement =
      Supplement<Navigator>::From<NavigatorMediaCapabilities>(navigator);
  if (!supplement) {
    supplement = new NavigatorMediaCapabilities(navigator);
    ProvideTo(navigator, supplement);
  }
  return *supplement;
}

}  // namespace blink
