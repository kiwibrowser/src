/*
 * Copyright (C) 2013 Google Inc. All Rights Reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE COMPUTER, INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE COMPUTER, INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/blink/renderer/platform/heap/handle.h"
#include "third_party/blink/renderer/platform/lifecycle_notifier.h"
#include "third_party/blink/renderer/platform/lifecycle_observer.h"

namespace blink {

class TestingObserver;

class DummyContext final
    : public GarbageCollectedFinalized<DummyContext>,
      public LifecycleNotifier<DummyContext, TestingObserver> {
  USING_GARBAGE_COLLECTED_MIXIN(DummyContext);

 public:
  static DummyContext* Create() { return new DummyContext; }

  void Trace(blink::Visitor* visitor) override {
    LifecycleNotifier<DummyContext, TestingObserver>::Trace(visitor);
  }
};

class TestingObserver final
    : public GarbageCollected<TestingObserver>,
      public LifecycleObserver<DummyContext, TestingObserver> {
  USING_GARBAGE_COLLECTED_MIXIN(TestingObserver);

 public:
  static TestingObserver* Create(DummyContext* context) {
    return new TestingObserver(context);
  }

  void ContextDestroyed(DummyContext* destroyed_context) {
    if (observer_to_remove_on_destruct_) {
      destroyed_context->RemoveObserver(observer_to_remove_on_destruct_);
      observer_to_remove_on_destruct_.Clear();
    }
    context_destroyed_called_ = true;
  }

  void Trace(blink::Visitor* visitor) override {
    visitor->Trace(observer_to_remove_on_destruct_);
    LifecycleObserver::Trace(visitor);
  }

  void Unobserve() { SetContext(nullptr); }

  void SetObserverToRemoveAndDestroy(
      TestingObserver* observer_to_remove_on_destruct) {
    DCHECK(!observer_to_remove_on_destruct_);
    observer_to_remove_on_destruct_ = observer_to_remove_on_destruct;
  }

  TestingObserver* InnerObserver() const {
    return observer_to_remove_on_destruct_;
  }
  bool ContextDestroyedCalled() const { return context_destroyed_called_; }

 private:
  explicit TestingObserver(DummyContext* context)
      : LifecycleObserver(context), context_destroyed_called_(false) {}

  Member<TestingObserver> observer_to_remove_on_destruct_;
  bool context_destroyed_called_;
};

TEST(LifecycleContextTest, shouldObserveContextDestroyed) {
  DummyContext* context = DummyContext::Create();
  Persistent<TestingObserver> observer = TestingObserver::Create(context);

  EXPECT_EQ(observer->LifecycleContext(), context);
  EXPECT_FALSE(observer->ContextDestroyedCalled());
  context->NotifyContextDestroyed();
  context = nullptr;
  ThreadState::Current()->CollectAllGarbage();
  EXPECT_EQ(observer->LifecycleContext(), static_cast<DummyContext*>(nullptr));
  EXPECT_TRUE(observer->ContextDestroyedCalled());
}

TEST(LifecycleContextTest, shouldNotObserveContextDestroyedIfUnobserve) {
  DummyContext* context = DummyContext::Create();
  Persistent<TestingObserver> observer = TestingObserver::Create(context);
  observer->Unobserve();
  context->NotifyContextDestroyed();
  context = nullptr;
  ThreadState::Current()->CollectAllGarbage();
  EXPECT_EQ(observer->LifecycleContext(), static_cast<DummyContext*>(nullptr));
  EXPECT_FALSE(observer->ContextDestroyedCalled());
}

TEST(LifecycleContextTest, observerRemovedDuringNotifyDestroyed) {
  DummyContext* context = DummyContext::Create();
  Persistent<TestingObserver> observer = TestingObserver::Create(context);
  TestingObserver* inner_observer = TestingObserver::Create(context);
  // Attach the observer to the other. When 'observer' is notified
  // of destruction, it will remove & destroy 'innerObserver'.
  observer->SetObserverToRemoveAndDestroy(inner_observer);

  EXPECT_EQ(observer->LifecycleContext(), context);
  EXPECT_EQ(observer->InnerObserver()->LifecycleContext(), context);
  EXPECT_FALSE(observer->ContextDestroyedCalled());
  EXPECT_FALSE(observer->InnerObserver()->ContextDestroyedCalled());

  context->NotifyContextDestroyed();
  EXPECT_EQ(observer->InnerObserver(), nullptr);
  context = nullptr;
  ThreadState::Current()->CollectAllGarbage();
  EXPECT_EQ(observer->LifecycleContext(), static_cast<DummyContext*>(nullptr));
  EXPECT_TRUE(observer->ContextDestroyedCalled());
}

}  // namespace blink
