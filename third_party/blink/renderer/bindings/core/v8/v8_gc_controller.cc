/*
 * Copyright (C) 2009 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *     * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "third_party/blink/renderer/bindings/core/v8/v8_gc_controller.h"

#include <algorithm>
#include <unordered_map>
#include <unordered_set>
#include "third_party/blink/public/platform/blame_context.h"
#include "third_party/blink/public/platform/platform.h"
#include "third_party/blink/renderer/bindings/core/v8/active_script_wrappable.h"
#include "third_party/blink/renderer/bindings/core/v8/script_source_code.h"
#include "third_party/blink/renderer/bindings/core/v8/v8_abstract_event_listener.h"
#include "third_party/blink/renderer/bindings/core/v8/v8_binding_for_core.h"
#include "third_party/blink/renderer/bindings/core/v8/v8_node.h"
#include "third_party/blink/renderer/bindings/core/v8/v8_script_runner.h"
#include "third_party/blink/renderer/core/dom/attr.h"
#include "third_party/blink/renderer/core/dom/element.h"
#include "third_party/blink/renderer/core/dom/node.h"
#include "third_party/blink/renderer/core/html/imports/html_imports_controller.h"
#include "third_party/blink/renderer/core/inspector/inspector_trace_events.h"
#include "third_party/blink/renderer/platform/bindings/script_wrappable_marking_visitor.h"
#include "third_party/blink/renderer/platform/bindings/wrapper_type_info.h"
#include "third_party/blink/renderer/platform/histogram.h"
#include "third_party/blink/renderer/platform/instrumentation/tracing/trace_event.h"
#include "third_party/blink/renderer/platform/wtf/allocator/partitions.h"

namespace blink {

Node* V8GCController::OpaqueRootForGC(v8::Isolate*, Node* node) {
  DCHECK(node);
  if (node->isConnected())
    return &node->GetDocument().MasterDocument();

  if (node->IsAttributeNode()) {
    Node* owner_element = ToAttr(node)->ownerElement();
    if (!owner_element)
      return node;
    node = owner_element;
  }

  while (Node* parent = node->ParentOrShadowHostOrTemplateHostNode())
    node = parent;

  return node;
}

class MinorGCUnmodifiedWrapperVisitor : public v8::PersistentHandleVisitor {
 public:
  explicit MinorGCUnmodifiedWrapperVisitor(v8::Isolate* isolate)
      : isolate_(isolate) {}

  void VisitPersistentHandle(v8::Persistent<v8::Value>* value,
                             uint16_t class_id) override {
    if (class_id != WrapperTypeInfo::kNodeClassId &&
        class_id != WrapperTypeInfo::kObjectClassId) {
      return;
    }

    // MinorGC does not collect objects because it may be expensive to
    // update references during minorGC
    if (class_id == WrapperTypeInfo::kObjectClassId) {
      v8::Persistent<v8::Object>::Cast(*value).MarkActive();
      return;
    }

    v8::Local<v8::Object> wrapper = v8::Local<v8::Object>::New(
        isolate_, v8::Persistent<v8::Object>::Cast(*value));
    DCHECK(V8DOMWrapper::HasInternalFieldsSet(wrapper));
    if (ToWrapperTypeInfo(wrapper)->IsActiveScriptWrappable() &&
        ToScriptWrappable(wrapper)->HasPendingActivity()) {
      v8::Persistent<v8::Object>::Cast(*value).MarkActive();
      return;
    }

    if (class_id == WrapperTypeInfo::kNodeClassId) {
      DCHECK(V8Node::hasInstance(wrapper, isolate_));
      Node* node = V8Node::ToImpl(wrapper);
      if (node->HasEventListeners()) {
        v8::Persistent<v8::Object>::Cast(*value).MarkActive();
        return;
      }
      // FIXME: Remove the special handling for SVG elements.
      // We currently can't collect SVG Elements from minor gc, as we have
      // strong references from SVG property tear-offs keeping context SVG
      // element alive.
      if (node->IsSVGElement()) {
        v8::Persistent<v8::Object>::Cast(*value).MarkActive();
        return;
      }
    }
  }

 private:
  v8::Isolate* isolate_;
};

static unsigned long long UsedHeapSize(v8::Isolate* isolate) {
  v8::HeapStatistics heap_statistics;
  isolate->GetHeapStatistics(&heap_statistics);
  return heap_statistics.used_heap_size();
}

namespace {

void VisitWeakHandlesForMinorGC(v8::Isolate* isolate) {
  MinorGCUnmodifiedWrapperVisitor visitor(isolate);
  isolate->VisitWeakHandles(&visitor);
}

}  // namespace

void V8GCController::GcPrologue(v8::Isolate* isolate,
                                v8::GCType type,
                                v8::GCCallbackFlags flags) {
  RUNTIME_CALL_TIMER_SCOPE(isolate, RuntimeCallStats::CounterId::kGcPrologue);
  ScriptForbiddenScope::Enter();

  // Attribute garbage collection to the all frames instead of a specific
  // frame.
  if (BlameContext* blame_context =
          Platform::Current()->GetTopLevelBlameContext())
    blame_context->Enter();

  // TODO(haraken): A GC callback is not allowed to re-enter V8. This means
  // that it's unsafe to run Oilpan's GC in the GC callback because it may
  // run finalizers that call into V8. To avoid the risk, we should post
  // a task to schedule the Oilpan's GC.
  // (In practice, there is no finalizer that calls into V8 and thus is safe.)

  v8::HandleScope scope(isolate);
  switch (type) {
    case v8::kGCTypeScavenge:
      TRACE_EVENT_BEGIN1("devtools.timeline,v8", "MinorGC",
                         "usedHeapSizeBefore", UsedHeapSize(isolate));
      VisitWeakHandlesForMinorGC(isolate);
      break;
    case v8::kGCTypeMarkSweepCompact:
      if (ThreadState::Current())
        ThreadState::Current()->WillStartV8GC(BlinkGC::kV8MajorGC);

      TRACE_EVENT_BEGIN2("devtools.timeline,v8", "MajorGC",
                         "usedHeapSizeBefore", UsedHeapSize(isolate), "type",
                         "atomic pause");
      break;
    case v8::kGCTypeIncrementalMarking:
      if (ThreadState::Current())
        ThreadState::Current()->WillStartV8GC(BlinkGC::kV8MajorGC);

      TRACE_EVENT_BEGIN2("devtools.timeline,v8", "MajorGC",
                         "usedHeapSizeBefore", UsedHeapSize(isolate), "type",
                         "incremental marking");
      break;
    case v8::kGCTypeProcessWeakCallbacks:
      TRACE_EVENT_BEGIN2("devtools.timeline,v8", "MajorGC",
                         "usedHeapSizeBefore", UsedHeapSize(isolate), "type",
                         "weak processing");
      break;
    default:
      NOTREACHED();
  }
}

namespace {

void UpdateCollectedPhantomHandles(v8::Isolate* isolate) {
  ThreadHeapStats& heap_stats = ThreadState::Current()->Heap().HeapStats();
  size_t count = isolate->NumberOfPhantomHandleResetsSinceLastCall();
  heap_stats.DecreaseWrapperCount(count);
  heap_stats.IncreaseCollectedWrapperCount(count);
}

}  // namespace

void V8GCController::GcEpilogue(v8::Isolate* isolate,
                                v8::GCType type,
                                v8::GCCallbackFlags flags) {
  RUNTIME_CALL_TIMER_SCOPE(isolate, RuntimeCallStats::CounterId::kGcEpilogue);
  UpdateCollectedPhantomHandles(isolate);
  switch (type) {
    case v8::kGCTypeScavenge:
      TRACE_EVENT_END1("devtools.timeline,v8", "MinorGC", "usedHeapSizeAfter",
                       UsedHeapSize(isolate));
      break;
    case v8::kGCTypeMarkSweepCompact:
      TRACE_EVENT_END1("devtools.timeline,v8", "MajorGC", "usedHeapSizeAfter",
                       UsedHeapSize(isolate));
      if (ThreadState::Current())
        ThreadState::Current()->ScheduleV8FollowupGCIfNeeded(
            BlinkGC::kV8MajorGC);
      break;
    case v8::kGCTypeIncrementalMarking:
      TRACE_EVENT_END1("devtools.timeline,v8", "MajorGC", "usedHeapSizeAfter",
                       UsedHeapSize(isolate));
      break;
    case v8::kGCTypeProcessWeakCallbacks:
      TRACE_EVENT_END1("devtools.timeline,v8", "MajorGC", "usedHeapSizeAfter",
                       UsedHeapSize(isolate));
      break;
    default:
      NOTREACHED();
  }

  ScriptForbiddenScope::Exit();

  if (BlameContext* blame_context =
          Platform::Current()->GetTopLevelBlameContext())
    blame_context->Leave();

  ThreadState* current_thread_state = ThreadState::Current();
  if (current_thread_state && !current_thread_state->IsGCForbidden()) {
    // v8::kGCCallbackFlagForced forces a Blink heap garbage collection
    // when a garbage collection was forced from V8. This is either used
    // for tests that force GCs from JavaScript to verify that objects die
    // when expected.
    if (flags & v8::kGCCallbackFlagForced) {
      // This single GC is not enough for two reasons:
      //   (1) The GC is not precise because the GC scans on-stack pointers
      //       conservatively.
      //   (2) One GC is not enough to break a chain of persistent handles. It's
      //       possible that some heap allocated objects own objects that
      //       contain persistent handles pointing to other heap allocated
      //       objects. To break the chain, we need multiple GCs.
      //
      // Regarding (1), we force a precise GC at the end of the current event
      // loop. So if you want to collect all garbage, you need to wait until the
      // next event loop.  Regarding (2), it would be OK in practice to trigger
      // only one GC per gcEpilogue, because GCController.collectAll() forces
      // multiple V8's GC.
      current_thread_state->CollectGarbage(
          BlinkGC::kHeapPointersOnStack, BlinkGC::kAtomicMarking,
          BlinkGC::kEagerSweeping, BlinkGC::kForcedGC);

      // Forces a precise GC at the end of the current event loop.
      current_thread_state->SetGCState(ThreadState::kFullGCScheduled);
    }

    // v8::kGCCallbackFlagCollectAllAvailableGarbage is used when V8 handles
    // low memory notifications.
    if ((flags & v8::kGCCallbackFlagCollectAllAvailableGarbage) ||
        (flags & v8::kGCCallbackFlagCollectAllExternalMemory)) {
      // This single GC is not enough. See the above comment.
      current_thread_state->CollectGarbage(
          BlinkGC::kHeapPointersOnStack, BlinkGC::kAtomicMarking,
          BlinkGC::kEagerSweeping, BlinkGC::kForcedGC);

      // The conservative GC might have left floating garbage. Schedule
      // precise GC to ensure that we collect all available garbage.
      current_thread_state->SchedulePreciseGC();
    }

    // Schedules a precise GC for the next idle time period.
    if (flags & v8::kGCCallbackScheduleIdleGarbageCollection) {
      current_thread_state->ScheduleIdleGC();
    }
  }

  TRACE_EVENT_INSTANT1(TRACE_DISABLED_BY_DEFAULT("devtools.timeline"),
                       "UpdateCounters", TRACE_EVENT_SCOPE_THREAD, "data",
                       InspectorUpdateCountersEvent::Data());
}

void V8GCController::CollectGarbage(v8::Isolate* isolate, bool only_minor_gc) {
  v8::HandleScope handle_scope(isolate);
  scoped_refptr<ScriptState> script_state = ScriptState::Create(
      v8::Context::New(isolate),
      DOMWrapperWorld::Create(isolate,
                              DOMWrapperWorld::WorldType::kGarbageCollector));
  ScriptState::Scope scope(script_state.get());
  StringBuilder builder;
  builder.Append("if (gc) gc(");
  builder.Append(only_minor_gc ? "true" : "false");
  builder.Append(")");
  V8ScriptRunner::CompileAndRunInternalScript(
      isolate, script_state.get(),
      ScriptSourceCode(builder.ToString(), ScriptSourceLocationType::kInternal,
                       nullptr, KURL(), TextPosition()));
  script_state->DisposePerContextData();
}

void V8GCController::CollectAllGarbageForTesting(v8::Isolate* isolate) {
  for (unsigned i = 0; i < 5; i++)
    isolate->RequestGarbageCollectionForTesting(
        v8::Isolate::kFullGarbageCollection);
}

namespace {

// Traces all DOM persistent handles using the provided visitor.
class DOMWrapperTracer final : public v8::PersistentHandleVisitor {
 public:
  explicit DOMWrapperTracer(Visitor* visitor) : visitor_(visitor) {
    DCHECK(visitor_);
  }

  void VisitPersistentHandle(v8::Persistent<v8::Value>* value,
                             uint16_t class_id) final {
    if (class_id != WrapperTypeInfo::kNodeClassId &&
        class_id != WrapperTypeInfo::kObjectClassId)
      return;

    visitor_->Trace(
        ToScriptWrappable(v8::Persistent<v8::Object>::Cast(*value)));
  }

 private:
  Visitor* const visitor_;
};

// Purges all DOM persistent handles.
class DOMWrapperPurger final : public v8::PersistentHandleVisitor {
 public:
  explicit DOMWrapperPurger(v8::Isolate* isolate)
      : isolate_(isolate), scope_(isolate) {}

  void VisitPersistentHandle(v8::Persistent<v8::Value>* value,
                             uint16_t class_id) final {
    if (class_id != WrapperTypeInfo::kNodeClassId &&
        class_id != WrapperTypeInfo::kObjectClassId)
      return;

    // Clear out wrapper type information, essentially disconnecting the Blink
    // wrappable from the V8 wrapper. This way, V8 cannot find the C++ object
    // anymore.
    int indices[] = {kV8DOMWrapperObjectIndex, kV8DOMWrapperTypeIndex};
    void* values[] = {nullptr, nullptr};
    v8::Local<v8::Object> wrapper = v8::Local<v8::Object>::New(
        isolate_, v8::Persistent<v8::Object>::Cast(*value));
    wrapper->SetAlignedPointerInInternalFields(arraysize(indices), indices,
                                               values);
    value->Reset();
  }

 private:
  v8::Isolate* isolate_;
  v8::HandleScope scope_;
};

}  // namespace

void V8GCController::TraceDOMWrappers(v8::Isolate* isolate, Visitor* visitor) {
  DOMWrapperTracer tracer(visitor);
  isolate->VisitHandlesWithClassIds(&tracer);
}

void V8GCController::ClearDOMWrappers(v8::Isolate* isolate) {
  DOMWrapperPurger purger(isolate);
  isolate->VisitHandlesWithClassIds(&purger);
}

}  // namespace blink
