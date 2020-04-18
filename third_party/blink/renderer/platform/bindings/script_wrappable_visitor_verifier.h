// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_PLATFORM_BINDINGS_SCRIPT_WRAPPABLE_VISITOR_VERIFIER_H_
#define THIRD_PARTY_BLINK_RENDERER_PLATFORM_BINDINGS_SCRIPT_WRAPPABLE_VISITOR_VERIFIER_H_

#include "base/logging.h"
#include "third_party/blink/renderer/platform/bindings/script_wrappable_visitor.h"
#include "third_party/blink/renderer/platform/heap/gc_info.h"

namespace blink {

// This visitor should be applied on wrapper members of each marked object
// after marking is complete. The Visit method checks that the given wrapper
// is also marked.
class ScriptWrappableVisitorVerifier final : public ScriptWrappableVisitor {
 public:
  void Visit(const TraceWrapperV8Reference<v8::Value>&) final {}

  void Visit(void* object, TraceWrapperDescriptor descriptor) final {
    HeapObjectHeader* header =
        HeapObjectHeader::FromPayload(descriptor.base_object_payload);
    const char* name = GCInfoTable::Get()
                           .GCInfoFromIndex(header->GcInfoIndex())
                           ->name_(descriptor.base_object_payload);
    // If this FATAL is hit, it means that a white (not discovered by
    // TraceWrappers) object was assigned as a member to a black object (already
    // processed by TraceWrappers). The black object will not be processed
    // anymore so white object will remain undetected and therefore its wrapper
    // and all wrappers reachable from it would be collected.
    //
    // This means there is a write barrier missing somewhere. Check the
    // backtrace to see which types are causing this and review all the places
    // where white object is set to a black object.
    LOG_IF(FATAL, !header->IsWrapperHeaderMarked())
        << "Write barrier missed for " << name;
  }

  void Visit(DOMWrapperMap<ScriptWrappable>*,
             const ScriptWrappable* key) final {}

 protected:
  using Visitor::Visit;
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_PLATFORM_BINDINGS_SCRIPT_WRAPPABLE_VISITOR_VERIFIER_H_
