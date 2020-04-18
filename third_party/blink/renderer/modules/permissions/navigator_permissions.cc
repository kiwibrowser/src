// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/modules/permissions/navigator_permissions.h"

#include "third_party/blink/renderer/core/frame/navigator.h"
#include "third_party/blink/renderer/modules/permissions/permissions.h"

namespace blink {

NavigatorPermissions::NavigatorPermissions() = default;

// static
const char NavigatorPermissions::kSupplementName[] = "NavigatorPermissions";

// static
NavigatorPermissions& NavigatorPermissions::From(Navigator& navigator) {
  NavigatorPermissions* supplement =
      Supplement<Navigator>::From<NavigatorPermissions>(navigator);
  if (!supplement) {
    supplement = new NavigatorPermissions();
    ProvideTo(navigator, supplement);
  }
  return *supplement;
}

// static
Permissions* NavigatorPermissions::permissions(Navigator& navigator) {
  NavigatorPermissions& self = NavigatorPermissions::From(navigator);
  if (!self.permissions_)
    self.permissions_ = new Permissions();
  return self.permissions_.Get();
}

void NavigatorPermissions::Trace(blink::Visitor* visitor) {
  visitor->Trace(permissions_);
  Supplement<Navigator>::Trace(visitor);
}

}  // namespace blink
