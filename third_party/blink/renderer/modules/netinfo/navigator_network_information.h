// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_MODULES_NETINFO_NAVIGATOR_NETWORK_INFORMATION_H_
#define THIRD_PARTY_BLINK_RENDERER_MODULES_NETINFO_NAVIGATOR_NETWORK_INFORMATION_H_

#include "third_party/blink/renderer/core/dom/context_lifecycle_observer.h"
#include "third_party/blink/renderer/core/frame/navigator.h"
#include "third_party/blink/renderer/platform/supplementable.h"

namespace blink {

class Navigator;
class NetworkInformation;

class NavigatorNetworkInformation final
    : public GarbageCollected<NavigatorNetworkInformation>,
      public Supplement<Navigator>,
      public ContextClient {
  USING_GARBAGE_COLLECTED_MIXIN(NavigatorNetworkInformation);

 public:
  static const char kSupplementName[];

  static NavigatorNetworkInformation& From(Navigator&);
  static NavigatorNetworkInformation* ToNavigatorNetworkInformation(Navigator&);
  static NetworkInformation* connection(Navigator&);

  void Trace(blink::Visitor*) override;

 private:
  explicit NavigatorNetworkInformation(Navigator&);
  NetworkInformation* connection();

  Member<NetworkInformation> connection_;
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_MODULES_NETINFO_NAVIGATOR_NETWORK_INFORMATION_H_
