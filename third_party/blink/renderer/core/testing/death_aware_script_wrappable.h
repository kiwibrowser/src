// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_CORE_TESTING_DEATH_AWARE_SCRIPT_WRAPPABLE_H_
#define THIRD_PARTY_BLINK_RENDERER_CORE_TESTING_DEATH_AWARE_SCRIPT_WRAPPABLE_H_

#include <signal.h>
#include "third_party/blink/renderer/platform/bindings/script_wrappable.h"
#include "third_party/blink/renderer/platform/bindings/trace_wrapper_member.h"
#include "third_party/blink/renderer/platform/heap/heap.h"
#include "third_party/blink/renderer/platform/wtf/text/wtf_string.h"

namespace blink {

class DeathAwareScriptWrappable : public ScriptWrappable {
  DEFINE_WRAPPERTYPEINFO();
  static DeathAwareScriptWrappable* instance_;
  static bool has_died_;

 public:
  typedef TraceWrapperMember<DeathAwareScriptWrappable> Wrapper;

  ~DeathAwareScriptWrappable() override {
    if (this == instance_) {
      has_died_ = true;
    }
  }

  static DeathAwareScriptWrappable* Create() {
    return new DeathAwareScriptWrappable();
  }

  static bool HasDied() { return has_died_; }
  static void ObserveDeathsOf(DeathAwareScriptWrappable* instance) {
    has_died_ = false;
    instance_ = instance;
  }

  void Trace(blink::Visitor* visitor) override {
    visitor->Trace(wrapped_dependency_);
    visitor->Trace(wrapped_vector_dependency_);
    visitor->Trace(wrapped_hash_map_dependency_);
    ScriptWrappable::Trace(visitor);
  }

  void TraceWrappers(ScriptWrappableVisitor* visitor) const override {
    visitor->TraceWrappers(wrapped_dependency_);
    for (auto dep : wrapped_vector_dependency_) {
      visitor->TraceWrappers(dep);
    }
    for (auto pair : wrapped_hash_map_dependency_) {
      visitor->TraceWrappers(pair.key);
      visitor->TraceWrappers(pair.value);
    }
    ScriptWrappable::TraceWrappers(visitor);
  }

  void SetWrappedDependency(DeathAwareScriptWrappable* dependency) {
    wrapped_dependency_ = dependency;
  }

  void AddWrappedVectorDependency(DeathAwareScriptWrappable* dependency) {
    wrapped_vector_dependency_.push_back(dependency);
  }

  void AddWrappedHashMapDependency(DeathAwareScriptWrappable* key,
                                   DeathAwareScriptWrappable* value) {
    wrapped_hash_map_dependency_.insert(key, value);
  }

 private:
  DeathAwareScriptWrappable() = default;

  Wrapper wrapped_dependency_;
  HeapVector<Wrapper> wrapped_vector_dependency_;
  HeapHashMap<Wrapper, Wrapper> wrapped_hash_map_dependency_;
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_CORE_TESTING_DEATH_AWARE_SCRIPT_WRAPPABLE_H_
