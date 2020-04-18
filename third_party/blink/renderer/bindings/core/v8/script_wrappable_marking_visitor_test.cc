// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/platform/bindings/script_wrappable_marking_visitor.h"

#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/blink/renderer/bindings/core/v8/to_v8_for_core.h"
#include "third_party/blink/renderer/bindings/core/v8/v8_binding_for_testing.h"
#include "third_party/blink/renderer/bindings/core/v8/v8_gc_controller.h"
#include "third_party/blink/renderer/core/testing/death_aware_script_wrappable.h"
#include "third_party/blink/renderer/platform/bindings/trace_wrapper_v8_reference.h"
#include "third_party/blink/renderer/platform/bindings/v8_dom_wrapper.h"
#include "third_party/blink/renderer/platform/bindings/v8_per_isolate_data.h"

namespace blink {

static void PreciselyCollectGarbage() {
  ThreadState::Current()->CollectAllGarbage();
}

static void RunV8Scavenger(v8::Isolate* isolate) {
  V8GCController::CollectGarbage(isolate, true);
}

static void RunV8FullGc(v8::Isolate* isolate) {
  V8GCController::CollectGarbage(isolate, false);
}

TEST(ScriptWrappableMarkingVisitorTest,
     ScriptWrappableMarkingVisitorTracesWrappers) {
  V8TestingScope scope;
  ScriptWrappableMarkingVisitor* visitor =
      V8PerIsolateData::From(scope.GetIsolate())
          ->GetScriptWrappableMarkingVisitor();
  DeathAwareScriptWrappable* target = DeathAwareScriptWrappable::Create();
  DeathAwareScriptWrappable* dependency = DeathAwareScriptWrappable::Create();
  target->SetWrappedDependency(dependency);

  // The graph needs to be set up before starting tracing as otherwise the
  // conservative write barrier would trigger.
  visitor->TracePrologue();

  HeapObjectHeader* target_header = HeapObjectHeader::FromPayload(target);
  HeapObjectHeader* dependency_header =
      HeapObjectHeader::FromPayload(dependency);

  EXPECT_TRUE(visitor->MarkingDeque()->IsEmpty());
  EXPECT_FALSE(target_header->IsWrapperHeaderMarked());
  EXPECT_FALSE(dependency_header->IsWrapperHeaderMarked());

  std::pair<void*, void*> pair = std::make_pair(
      const_cast<WrapperTypeInfo*>(target->GetWrapperTypeInfo()), target);
  visitor->RegisterV8Reference(pair);
  EXPECT_EQ(visitor->MarkingDeque()->size(), 1ul);

  visitor->AdvanceTracing(
      0, v8::EmbedderHeapTracer::AdvanceTracingActions(
             v8::EmbedderHeapTracer::ForceCompletionAction::FORCE_COMPLETION));
  EXPECT_EQ(visitor->MarkingDeque()->size(), 0ul);
  EXPECT_TRUE(target_header->IsWrapperHeaderMarked());
  EXPECT_TRUE(dependency_header->IsWrapperHeaderMarked());

  visitor->AbortTracing();
}

TEST(ScriptWrappableMarkingVisitorTest,
     OilpanCollectObjectsNotReachableFromV8) {
  V8TestingScope scope;
  v8::Isolate* isolate = scope.GetIsolate();

  {
    v8::HandleScope handle_scope(isolate);
    DeathAwareScriptWrappable* object = DeathAwareScriptWrappable::Create();
    DeathAwareScriptWrappable::ObserveDeathsOf(object);

    // Creates new V8 wrapper and associates it with global scope
    ToV8(object, scope.GetContext()->Global(), isolate);
  }

  RunV8Scavenger(isolate);
  RunV8FullGc(isolate);
  PreciselyCollectGarbage();

  EXPECT_TRUE(DeathAwareScriptWrappable::HasDied());
}

TEST(ScriptWrappableMarkingVisitorTest,
     OilpanDoesntCollectObjectsReachableFromV8) {
  V8TestingScope scope;
  v8::Isolate* isolate = scope.GetIsolate();
  v8::HandleScope handle_scope(isolate);
  DeathAwareScriptWrappable* object = DeathAwareScriptWrappable::Create();
  DeathAwareScriptWrappable::ObserveDeathsOf(object);

  // Creates new V8 wrapper and associates it with global scope
  ToV8(object, scope.GetContext()->Global(), isolate);

  RunV8Scavenger(isolate);
  RunV8FullGc(isolate);
  PreciselyCollectGarbage();

  EXPECT_FALSE(DeathAwareScriptWrappable::HasDied());
}

TEST(ScriptWrappableMarkingVisitorTest, V8ReportsLiveObjectsDuringScavenger) {
  V8TestingScope scope;
  v8::Isolate* isolate = scope.GetIsolate();
  v8::HandleScope handle_scope(isolate);
  DeathAwareScriptWrappable* object = DeathAwareScriptWrappable::Create();
  DeathAwareScriptWrappable::ObserveDeathsOf(object);

  v8::Local<v8::Value> wrapper =
      ToV8(object, scope.GetContext()->Global(), isolate);
  EXPECT_TRUE(wrapper->IsObject());
  v8::Local<v8::Object> wrapper_object = wrapper->ToObject();
  // V8 collects wrappers with unmodified maps (as they can be recreated
  // without loosing any data if needed). We need to create some property on
  // wrapper so V8 will not see it as unmodified.
  EXPECT_TRUE(wrapper_object->CreateDataProperty(scope.GetContext(), 1, wrapper)
                  .IsJust());

  RunV8Scavenger(isolate);
  PreciselyCollectGarbage();

  EXPECT_FALSE(DeathAwareScriptWrappable::HasDied());
}

TEST(ScriptWrappableMarkingVisitorTest, V8ReportsLiveObjectsDuringFullGc) {
  V8TestingScope scope;
  v8::Isolate* isolate = scope.GetIsolate();
  v8::HandleScope handle_scope(isolate);
  DeathAwareScriptWrappable* object = DeathAwareScriptWrappable::Create();
  DeathAwareScriptWrappable::ObserveDeathsOf(object);

  ToV8(object, scope.GetContext()->Global(), isolate);

  RunV8Scavenger(isolate);
  RunV8FullGc(isolate);
  PreciselyCollectGarbage();

  EXPECT_FALSE(DeathAwareScriptWrappable::HasDied());
}

TEST(ScriptWrappableMarkingVisitorTest, OilpanClearsHeadersWhenObjectDied) {
  V8TestingScope scope;

  DeathAwareScriptWrappable* object = DeathAwareScriptWrappable::Create();
  ScriptWrappableMarkingVisitor* visitor =
      V8PerIsolateData::From(scope.GetIsolate())
          ->GetScriptWrappableMarkingVisitor();
  visitor->TracePrologue();
  auto* header = HeapObjectHeader::FromPayload(object);
  visitor->headers_to_unmark_.push_back(header);

  PreciselyCollectGarbage();

  EXPECT_FALSE(visitor->headers_to_unmark_.Contains(header));
  visitor->AbortTracing();
}

TEST(ScriptWrappableMarkingVisitorTest,
     OilpanClearsMarkingDequeWhenObjectDied) {
  V8TestingScope scope;

  DeathAwareScriptWrappable* object = DeathAwareScriptWrappable::Create();
  ScriptWrappableMarkingVisitor* visitor =
      V8PerIsolateData::From(scope.GetIsolate())
          ->GetScriptWrappableMarkingVisitor();
  visitor->TracePrologue();

  visitor->TraceWrappersWithManualWriteBarrier(object);

  EXPECT_EQ(visitor->MarkingDeque()->front().RawObjectPointer(), object);

  PreciselyCollectGarbage();

  EXPECT_EQ(visitor->MarkingDeque()->front().RawObjectPointer(), nullptr);

  visitor->AbortTracing();
}

TEST(ScriptWrappableMarkingVisitorTest,
     MarkedObjectDoesNothingOnWriteBarrierHitWhenDependencyIsMarkedToo) {
  V8TestingScope scope;

  ScriptWrappableMarkingVisitor* visitor =
      V8PerIsolateData::From(scope.GetIsolate())
          ->GetScriptWrappableMarkingVisitor();
  visitor->TracePrologue();

  DeathAwareScriptWrappable* target = DeathAwareScriptWrappable::Create();
  DeathAwareScriptWrappable* dependencies[] = {
      DeathAwareScriptWrappable::Create(), DeathAwareScriptWrappable::Create(),
      DeathAwareScriptWrappable::Create(), DeathAwareScriptWrappable::Create()};

  HeapObjectHeader::FromPayload(target)->MarkWrapperHeader();
  for (int i = 0; i < 4; i++) {
    HeapObjectHeader::FromPayload(dependencies[i])->MarkWrapperHeader();
  }

  EXPECT_TRUE(visitor->MarkingDeque()->IsEmpty());

  target->SetWrappedDependency(dependencies[0]);
  target->AddWrappedVectorDependency(dependencies[1]);
  target->AddWrappedHashMapDependency(dependencies[2], dependencies[3]);

  EXPECT_TRUE(visitor->MarkingDeque()->IsEmpty());
  visitor->AbortTracing();
}

TEST(ScriptWrappableMarkingVisitorTest,
     MarkedObjectMarksDependencyOnWriteBarrierHitWhenNotMarked) {
  V8TestingScope scope;

  ScriptWrappableMarkingVisitor* visitor =
      V8PerIsolateData::From(scope.GetIsolate())
          ->GetScriptWrappableMarkingVisitor();
  visitor->TracePrologue();

  DeathAwareScriptWrappable* target = DeathAwareScriptWrappable::Create();
  DeathAwareScriptWrappable* dependencies[] = {
      DeathAwareScriptWrappable::Create(), DeathAwareScriptWrappable::Create(),
      DeathAwareScriptWrappable::Create(), DeathAwareScriptWrappable::Create()};

  HeapObjectHeader::FromPayload(target)->MarkWrapperHeader();

  EXPECT_TRUE(visitor->MarkingDeque()->IsEmpty());

  target->SetWrappedDependency(dependencies[0]);
  target->AddWrappedVectorDependency(dependencies[1]);
  target->AddWrappedHashMapDependency(dependencies[2], dependencies[3]);

  for (int i = 0; i < 4; i++) {
    EXPECT_TRUE(visitor->MarkingDequeContains(dependencies[i]));
  }

  visitor->AbortTracing();
}

namespace {

class HandleContainer
    : public blink::GarbageCollectedFinalized<HandleContainer>,
      public blink::TraceWrapperBase {
 public:
  static HandleContainer* Create() { return new HandleContainer(); }
  virtual ~HandleContainer() = default;

  void Trace(blink::Visitor* visitor) {}
  void TraceWrappers(ScriptWrappableVisitor* visitor) const override {
    visitor->TraceWrappers(handle_.Cast<v8::Value>());
  }
  const char* NameInHeapSnapshot() const override { return "HandleContainer"; }

  void SetValue(v8::Isolate* isolate, v8::Local<v8::String> string) {
    handle_.Set(isolate, string);
  }

 private:
  HandleContainer() = default;

  TraceWrapperV8Reference<v8::String> handle_;
};

class InterceptingScriptWrappableMarkingVisitor
    : public blink::ScriptWrappableMarkingVisitor {
 public:
  InterceptingScriptWrappableMarkingVisitor(v8::Isolate* isolate)
      : ScriptWrappableMarkingVisitor(isolate),
        marked_wrappers_(new size_t(0)) {}
  ~InterceptingScriptWrappableMarkingVisitor() override {
    delete marked_wrappers_;
  }

  void Visit(const TraceWrapperV8Reference<v8::Value>&) override {
    *marked_wrappers_ += 1;
    // Do not actually mark this visitor, as this would call into v8, which
    // would require executing an actual GC.
  }

  size_t NumberOfMarkedWrappers() const { return *marked_wrappers_; }

  void Start() { TracePrologue(); }

  void end() {
    // Gracefully terminate tracing.
    AdvanceTracing(
        0,
        v8::EmbedderHeapTracer::AdvanceTracingActions(
            v8::EmbedderHeapTracer::ForceCompletionAction::FORCE_COMPLETION));
    AbortTracing();
  }

 private:
  size_t* marked_wrappers_;  // Indirection required because of const override.
};

class InterceptingScriptWrappableMarkingVisitorScope
    : public V8PerIsolateData::TemporaryScriptWrappableVisitorScope {
  WTF_MAKE_NONCOPYABLE(InterceptingScriptWrappableMarkingVisitorScope);
  STACK_ALLOCATED();

 public:
  InterceptingScriptWrappableMarkingVisitorScope(v8::Isolate* isolate)
      : V8PerIsolateData::TemporaryScriptWrappableVisitorScope(
            isolate,
            std::unique_ptr<InterceptingScriptWrappableMarkingVisitor>(
                new InterceptingScriptWrappableMarkingVisitor(isolate))) {
    Visitor()->Start();
  }

  virtual ~InterceptingScriptWrappableMarkingVisitorScope() {
    Visitor()->end();
  }

  InterceptingScriptWrappableMarkingVisitor* Visitor() {
    return reinterpret_cast<InterceptingScriptWrappableMarkingVisitor*>(
        CurrentVisitor());
  }
};

}  // namespace

TEST(ScriptWrappableMarkingVisitorTest, WriteBarrierOnUnmarkedContainer) {
  V8TestingScope scope;
  InterceptingScriptWrappableMarkingVisitorScope visitor_scope(
      scope.GetIsolate());
  auto* raw_visitor = visitor_scope.Visitor();

  v8::Local<v8::String> str =
      v8::String::NewFromUtf8(scope.GetIsolate(), "teststring",
                              v8::NewStringType::kNormal, sizeof("teststring"))
          .ToLocalChecked();
  HandleContainer* container = HandleContainer::Create();
  CHECK_EQ(0u, raw_visitor->NumberOfMarkedWrappers());
  container->SetValue(scope.GetIsolate(), str);
  // The write barrier is conservative and does not check the mark bits of the
  // source container.
  CHECK_EQ(1u, raw_visitor->NumberOfMarkedWrappers());
}

TEST(ScriptWrappableMarkingVisitorTest, WriteBarrierTriggersOnMarkedContainer) {
  V8TestingScope scope;
  InterceptingScriptWrappableMarkingVisitorScope visitor_scope(
      scope.GetIsolate());
  auto* raw_visitor = visitor_scope.Visitor();

  v8::Local<v8::String> str =
      v8::String::NewFromUtf8(scope.GetIsolate(), "teststring",
                              v8::NewStringType::kNormal, sizeof("teststring"))
          .ToLocalChecked();
  HandleContainer* container = HandleContainer::Create();
  HeapObjectHeader::FromPayload(container)->MarkWrapperHeader();
  CHECK_EQ(0u, raw_visitor->NumberOfMarkedWrappers());
  container->SetValue(scope.GetIsolate(), str);
  CHECK_EQ(1u, raw_visitor->NumberOfMarkedWrappers());
}

TEST(ScriptWrappableMarkingVisitorTest, VtableAtObjectStart) {
  // This test makes sure that the subobject v8::EmbedderHeapTracer is placed
  // at the start of a ScriptWrappableMarkingVisitor object. We do this to
  // mitigate potential problems that could be caused by LTO when passing
  // v8::EmbedderHeapTracer across the API boundary.
  V8TestingScope scope;
  std::unique_ptr<blink::ScriptWrappableMarkingVisitor> visitor(
      new ScriptWrappableMarkingVisitor(scope.GetIsolate()));
  CHECK_EQ(
      static_cast<void*>(visitor.get()),
      static_cast<void*>(dynamic_cast<v8::EmbedderHeapTracer*>(visitor.get())));
}

TEST(ScriptWrappableMarkingVisitor, WriteBarrierForScriptWrappable) {
  // Regression test for crbug.com/702490.
  V8TestingScope scope;
  InterceptingScriptWrappableMarkingVisitorScope visitor_scope(
      scope.GetIsolate());
  auto* raw_visitor = visitor_scope.Visitor();

  // Mark the ScriptWrappable.
  DeathAwareScriptWrappable* target = DeathAwareScriptWrappable::Create();
  HeapObjectHeader::FromPayload(target)->MarkWrapperHeader();

  // Create a 'wrapper' object.
  v8::Local<v8::Object> wrapper = V8DOMWrapper::CreateWrapper(
      scope.GetIsolate(), scope.GetContext()->Global(),
      target->GetWrapperTypeInfo());

  // Upon setting the wrapper we should have executed the write barrier.
  CHECK_EQ(0u, raw_visitor->NumberOfMarkedWrappers());
  v8::Local<v8::Object> final_wrapper =
      V8DOMWrapper::AssociateObjectWithWrapper(
          scope.GetIsolate(), target, target->GetWrapperTypeInfo(), wrapper);
  CHECK(!final_wrapper.IsEmpty());
  CHECK_EQ(1u, raw_visitor->NumberOfMarkedWrappers());
}

TEST(ScriptWrappableMarkingVisitorTest, WriteBarrierOnHeapVectorSwap1) {
  V8TestingScope scope;
  ScriptWrappableMarkingVisitor* visitor =
      V8PerIsolateData::From(scope.GetIsolate())
          ->GetScriptWrappableMarkingVisitor();

  HeapVector<DeathAwareScriptWrappable::Wrapper> vector1;
  DeathAwareScriptWrappable* entry1 = DeathAwareScriptWrappable::Create();
  vector1.push_back(entry1);
  HeapVector<DeathAwareScriptWrappable::Wrapper> vector2;
  DeathAwareScriptWrappable* entry2 = DeathAwareScriptWrappable::Create();
  vector2.push_back(entry2);

  visitor->TracePrologue();

  EXPECT_TRUE(visitor->MarkingDeque()->IsEmpty());
  swap(vector1, vector2);

  EXPECT_TRUE(visitor->MarkingDequeContains(entry1));
  EXPECT_TRUE(visitor->MarkingDequeContains(entry2));

  visitor->AbortTracing();
}

TEST(ScriptWrappableMarkingVisitorTest, WriteBarrierOnHeapVectorSwap2) {
  V8TestingScope scope;
  ScriptWrappableMarkingVisitor* visitor =
      V8PerIsolateData::From(scope.GetIsolate())
          ->GetScriptWrappableMarkingVisitor();

  HeapVector<DeathAwareScriptWrappable::Wrapper> vector1;
  DeathAwareScriptWrappable* entry1 = DeathAwareScriptWrappable::Create();
  vector1.push_back(entry1);
  HeapVector<Member<DeathAwareScriptWrappable>> vector2;
  DeathAwareScriptWrappable* entry2 = DeathAwareScriptWrappable::Create();
  vector2.push_back(entry2);

  visitor->TracePrologue();

  EXPECT_TRUE(visitor->MarkingDeque()->IsEmpty());
  swap(vector1, vector2);

  // Only entry2 is held alive by TraceWrapperMember, so we only expect this
  // barrier to fire.
  EXPECT_TRUE(visitor->MarkingDequeContains(entry2));

  visitor->AbortTracing();
}

namespace {

class Mixin : public GarbageCollectedMixin {
 public:
  explicit Mixin(DeathAwareScriptWrappable* wrapper_in_mixin)
      : wrapper_in_mixin_(wrapper_in_mixin) {}

  void TraceWrappers(ScriptWrappableVisitor* visitor) const {
    visitor->TraceWrappers(wrapper_in_mixin_);
  }

 protected:
  DeathAwareScriptWrappable::Wrapper wrapper_in_mixin_;
};

class ClassWithField {
 protected:
  int field_;
};

class Base : public blink::GarbageCollected<Base>,
             public blink::TraceWrapperBase,
             public ClassWithField,
             public Mixin {
  USING_GARBAGE_COLLECTED_MIXIN(Base);

 public:
  static Base* Create(DeathAwareScriptWrappable* wrapper_in_base,
                      DeathAwareScriptWrappable* wrapper_in_mixin) {
    return new Base(wrapper_in_base, wrapper_in_mixin);
  }

  void TraceWrappers(ScriptWrappableVisitor* visitor) const override {
    visitor->TraceWrappers(wrapper_in_base_);
    Mixin::TraceWrappers(visitor);
  }

  const char* NameInHeapSnapshot() const override { return "HandleContainer"; }

 protected:
  Base(DeathAwareScriptWrappable* wrapper_in_base,
       DeathAwareScriptWrappable* wrapper_in_mixin)
      : Mixin(wrapper_in_mixin), wrapper_in_base_(wrapper_in_base) {
    // Use field_;
    field_ = 0;
  }

  DeathAwareScriptWrappable::Wrapper wrapper_in_base_;
};

}  // namespace

TEST(ScriptWrappableMarkingVisitorTest, MixinTracing) {
  V8TestingScope scope;
  ScriptWrappableMarkingVisitor* visitor =
      V8PerIsolateData::From(scope.GetIsolate())
          ->GetScriptWrappableMarkingVisitor();

  DeathAwareScriptWrappable* base_wrapper = DeathAwareScriptWrappable::Create();
  DeathAwareScriptWrappable* mixin_wrapper =
      DeathAwareScriptWrappable::Create();
  Base* base = Base::Create(base_wrapper, mixin_wrapper);
  Mixin* mixin = static_cast<Mixin*>(base);
  HeapObjectHeader* base_header = HeapObjectHeader::FromPayload(base);
  EXPECT_FALSE(base_header->IsWrapperHeaderMarked());

  // Make sure that mixin does not point to the object header.
  EXPECT_NE(static_cast<void*>(base), static_cast<void*>(mixin));

  visitor->TracePrologue();

  EXPECT_TRUE(visitor->MarkingDeque()->IsEmpty());

  // TraceWrapperMember itself is not required to live in an Oilpan object.
  TraceWrapperMember<Mixin> mixin_handle = mixin;
  EXPECT_TRUE(base_header->IsWrapperHeaderMarked());
  EXPECT_FALSE(visitor->MarkingDeque()->IsEmpty());
  EXPECT_TRUE(visitor->MarkingDequeContains(base));

  visitor->AdvanceTracing(
      0, v8::EmbedderHeapTracer::AdvanceTracingActions(
             v8::EmbedderHeapTracer::ForceCompletionAction::FORCE_COMPLETION));
  EXPECT_EQ(visitor->MarkingDeque()->size(), 0ul);
  EXPECT_TRUE(base_header->IsWrapperHeaderMarked());
  EXPECT_TRUE(
      HeapObjectHeader::FromPayload(base_wrapper)->IsWrapperHeaderMarked());
  EXPECT_TRUE(
      HeapObjectHeader::FromPayload(mixin_wrapper)->IsWrapperHeaderMarked());

  mixin_handle = nullptr;
  visitor->AbortTracing();
}

}  // namespace blink
