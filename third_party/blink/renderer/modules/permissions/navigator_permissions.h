// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_MODULES_PERMISSIONS_NAVIGATOR_PERMISSIONS_H_
#define THIRD_PARTY_BLINK_RENDERER_MODULES_PERMISSIONS_NAVIGATOR_PERMISSIONS_H_

#include "third_party/blink/renderer/core/frame/navigator.h"
#include "third_party/blink/renderer/platform/heap/handle.h"
#include "third_party/blink/renderer/platform/supplementable.h"

namespace blink {

class Navigator;
class Permissions;

class NavigatorPermissions final
    : public GarbageCollected<NavigatorPermissions>,
      public Supplement<Navigator> {
  USING_GARBAGE_COLLECTED_MIXIN(NavigatorPermissions);

 public:
  static const char kSupplementName[];

  static NavigatorPermissions& From(Navigator&);
  static Permissions* permissions(Navigator&);

  void Trace(blink::Visitor*) override;

 private:
  NavigatorPermissions();

  Member<Permissions> permissions_;
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_MODULES_PERMISSIONS_NAVIGATOR_PERMISSIONS_H_
