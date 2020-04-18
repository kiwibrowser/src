// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_MODULES_CREDENTIALMANAGER_NAVIGATOR_CREDENTIALS_H_
#define THIRD_PARTY_BLINK_RENDERER_MODULES_CREDENTIALMANAGER_NAVIGATOR_CREDENTIALS_H_

#include "third_party/blink/renderer/core/frame/navigator.h"
#include "third_party/blink/renderer/platform/heap/handle.h"
#include "third_party/blink/renderer/platform/supplementable.h"

namespace blink {

class CredentialsContainer;
class Navigator;

class NavigatorCredentials final
    : public GarbageCollected<NavigatorCredentials>,
      public Supplement<Navigator> {
  USING_GARBAGE_COLLECTED_MIXIN(NavigatorCredentials);

 public:
  static const char kSupplementName[];

  static NavigatorCredentials& From(Navigator&);
  // NavigatorCredentials.idl
  static CredentialsContainer* credentials(Navigator&);

  void Trace(blink::Visitor*) override;

 private:
  explicit NavigatorCredentials(Navigator&);
  CredentialsContainer* credentials();

  Member<CredentialsContainer> credentials_container_;
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_MODULES_CREDENTIALMANAGER_NAVIGATOR_CREDENTIALS_H_
