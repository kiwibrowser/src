// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/modules/presentation/navigator_presentation.h"

#include "third_party/blink/renderer/core/frame/navigator.h"
#include "third_party/blink/renderer/modules/presentation/presentation.h"

namespace blink {

NavigatorPresentation::NavigatorPresentation() = default;

// static
const char NavigatorPresentation::kSupplementName[] = "NavigatorPresentation";

// static
NavigatorPresentation& NavigatorPresentation::From(Navigator& navigator) {
  NavigatorPresentation* supplement =
      Supplement<Navigator>::From<NavigatorPresentation>(navigator);
  if (!supplement) {
    supplement = new NavigatorPresentation();
    ProvideTo(navigator, supplement);
  }
  return *supplement;
}

// static
Presentation* NavigatorPresentation::presentation(Navigator& navigator) {
  NavigatorPresentation& self = NavigatorPresentation::From(navigator);
  if (!self.presentation_) {
    if (!navigator.GetFrame())
      return nullptr;
    self.presentation_ = Presentation::Create(navigator.GetFrame());
  }
  return self.presentation_;
}

void NavigatorPresentation::Trace(blink::Visitor* visitor) {
  visitor->Trace(presentation_);
  Supplement<Navigator>::Trace(visitor);
}

}  // namespace blink
