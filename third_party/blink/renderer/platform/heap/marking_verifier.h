// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_PLATFORM_HEAP_MARKING_VERIFIER_H_
#define THIRD_PARTY_BLINK_RENDERER_PLATFORM_HEAP_MARKING_VERIFIER_H_

#include "third_party/blink/renderer/platform/heap/garbage_collected.h"
#include "third_party/blink/renderer/platform/heap/heap_page.h"
#include "third_party/blink/renderer/platform/heap/visitor.h"

namespace blink {

// Marking verifier that checks that a child is marked if its parent is marked.
class MarkingVerifier final : public Visitor {
 public:
  explicit MarkingVerifier(ThreadState* state) : Visitor(state) {}
  ~MarkingVerifier() override {}

  void VerifyObject(HeapObjectHeader* header) {
    // Verify only non-free marked objects.
    if (header->IsFree() || !header->IsMarked())
      return;

    const GCInfo* info =
        GCInfoTable::Get().GCInfoFromIndex(header->GcInfoIndex());
    const bool can_verify =
        !info->HasVTable() || blink::VTableInitialized(header->Payload());
    if (can_verify) {
      CHECK(header->IsValid());
      info->trace_(this, header->Payload());
    }
  }

  void Visit(void* object, TraceDescriptor desc) final {
    CHECK(object);
    VerifyChild(desc.base_object_payload);
  }

  void VisitWeak(void* object,
                 void** object_slot,
                 TraceDescriptor desc,
                 WeakCallback callback) final {
    CHECK(object);
    VerifyChild(desc.base_object_payload);
  }

  // Unused overrides.
  void VisitBackingStoreStrongly(void*, void**, TraceDescriptor) final {}
  void VisitBackingStoreWeakly(void*,
                               void**,
                               TraceDescriptor,
                               WeakCallback,
                               void*) final {}
  void VisitBackingStoreOnly(void*, void**) final {}
  void RegisterBackingStoreCallback(void*, MovingObjectCallback, void*) final {}
  void RegisterWeakCallback(void*, WeakCallback) final {}
  void Visit(const TraceWrapperV8Reference<v8::Value>&) final {}
  void Visit(DOMWrapperMap<ScriptWrappable>*,
             const ScriptWrappable* key) final {}
  void Visit(void*, TraceWrapperDescriptor) final {}

 private:
  void VerifyChild(void* base_object_payload) {
    CHECK(base_object_payload);
    HeapObjectHeader* child_header =
        HeapObjectHeader::FromPayload(base_object_payload);
    // This CHECKs ensure that any children reachable from marked parents are
    // also marked. If you hit these CHECKs then marking is in an inconsistent
    // state meaning that there are unmarked objects reachable from marked
    // ones.
    CHECK(child_header);
    CHECK(child_header->IsMarked());
  }
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_PLATFORM_HEAP_MARKING_VERIFIER_H_
