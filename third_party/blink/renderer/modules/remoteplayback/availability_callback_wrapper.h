// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_MODULES_REMOTEPLAYBACK_AVAILABILITY_CALLBACK_WRAPPER_H_
#define THIRD_PARTY_BLINK_RENDERER_MODULES_REMOTEPLAYBACK_AVAILABILITY_CALLBACK_WRAPPER_H_

#include <memory>

#include "base/callback.h"
#include "third_party/blink/renderer/platform/bindings/script_wrappable.h"
#include "third_party/blink/renderer/platform/bindings/trace_wrapper_member.h"
#include "third_party/blink/renderer/platform/heap/garbage_collected.h"
#include "third_party/blink/renderer/platform/wtf/compiler.h"

namespace blink {

class RemotePlayback;
class V8RemotePlaybackAvailabilityCallback;

// Wraps either a base::OnceClosure or RemotePlaybackAvailabilityCallback object
// to be kept in the RemotePlayback's |availability_callbacks_| map.
class AvailabilityCallbackWrapper final
    : public GarbageCollectedFinalized<AvailabilityCallbackWrapper>,
      public TraceWrapperBase {
  WTF_MAKE_NONCOPYABLE(AvailabilityCallbackWrapper);

 public:
  explicit AvailabilityCallbackWrapper(V8RemotePlaybackAvailabilityCallback*);
  explicit AvailabilityCallbackWrapper(base::RepeatingClosure);
  ~AvailabilityCallbackWrapper() = default;

  void Run(RemotePlayback*, bool new_availability);

  virtual void Trace(blink::Visitor*);
  void TraceWrappers(ScriptWrappableVisitor*) const override;
  const char* NameInHeapSnapshot() const override {
    return "AvailabilityCallbackWrapper";
  }

 private:
  // Only one of these callbacks must be set.
  TraceWrapperMember<V8RemotePlaybackAvailabilityCallback> bindings_cb_;
  base::RepeatingClosure internal_cb_;
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_MODULES_REMOTEPLAYBACK_AVAILABILITY_CALLBACK_WRAPPER_H_
