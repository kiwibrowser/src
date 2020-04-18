// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_MODULES_NETINFO_WORKER_NAVIGATOR_NETWORK_INFORMATION_H_
#define THIRD_PARTY_BLINK_RENDERER_MODULES_NETINFO_WORKER_NAVIGATOR_NETWORK_INFORMATION_H_

#include "third_party/blink/renderer/core/workers/worker_navigator.h"
#include "third_party/blink/renderer/platform/supplementable.h"

namespace blink {

class ExecutionContext;
class NetworkInformation;
class ScriptState;
class WorkerNavigator;

class WorkerNavigatorNetworkInformation final
    : public GarbageCollected<WorkerNavigatorNetworkInformation>,
      public Supplement<WorkerNavigator> {
  USING_GARBAGE_COLLECTED_MIXIN(WorkerNavigatorNetworkInformation);

 public:
  static const char kSupplementName[];

  static WorkerNavigatorNetworkInformation& From(WorkerNavigator&,
                                                 ExecutionContext*);
  static WorkerNavigatorNetworkInformation* ToWorkerNavigatorNetworkInformation(
      WorkerNavigator&,
      ExecutionContext*);

  static NetworkInformation* connection(ScriptState*, WorkerNavigator&);

  void Trace(blink::Visitor*) override;

 private:
  WorkerNavigatorNetworkInformation(WorkerNavigator&, ExecutionContext*);
  NetworkInformation* connection(ExecutionContext*);

  Member<NetworkInformation> connection_;
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_MODULES_NETINFO_WORKER_NAVIGATOR_NETWORK_INFORMATION_H_
