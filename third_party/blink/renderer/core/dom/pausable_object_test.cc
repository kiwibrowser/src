/*
 * Copyright (c) 2014, Google Inc. All rights reserved.
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

#include "third_party/blink/renderer/core/dom/pausable_object.h"

#include <memory>
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/blink/renderer/core/dom/document.h"
#include "third_party/blink/renderer/core/testing/dummy_page_holder.h"

namespace blink {

class MockPausableObject final
    : public GarbageCollectedFinalized<MockPausableObject>,
      public PausableObject {
  USING_GARBAGE_COLLECTED_MIXIN(MockPausableObject);

 public:
  explicit MockPausableObject(ExecutionContext* context)
      : PausableObject(context) {}

  void Trace(blink::Visitor* visitor) override {
    PausableObject::Trace(visitor);
  }

  MOCK_METHOD0(Pause, void());
  MOCK_METHOD0(Unpause, void());
  MOCK_METHOD1(ContextDestroyed, void(ExecutionContext*));
};

class PausableObjectTest : public testing::Test {
 protected:
  PausableObjectTest();

  Document& SrcDocument() const { return src_page_holder_->GetDocument(); }
  Document& DestDocument() const { return dest_page_holder_->GetDocument(); }
  MockPausableObject& PausableObject() { return *pausable_object_; }

 private:
  std::unique_ptr<DummyPageHolder> src_page_holder_;
  std::unique_ptr<DummyPageHolder> dest_page_holder_;
  Persistent<MockPausableObject> pausable_object_;
};

PausableObjectTest::PausableObjectTest()
    : src_page_holder_(DummyPageHolder::Create(IntSize(800, 600))),
      dest_page_holder_(DummyPageHolder::Create(IntSize(800, 600))),
      pausable_object_(
          new MockPausableObject(&src_page_holder_->GetDocument())) {
  pausable_object_->PauseIfNeeded();
}

TEST_F(PausableObjectTest, NewContextObserved) {
  unsigned initial_src_count = SrcDocument().PausableObjectCount();
  unsigned initial_dest_count = DestDocument().PausableObjectCount();

  EXPECT_CALL(PausableObject(), Unpause());
  PausableObject().DidMoveToNewExecutionContext(&DestDocument());

  EXPECT_EQ(initial_src_count - 1, SrcDocument().PausableObjectCount());
  EXPECT_EQ(initial_dest_count + 1, DestDocument().PausableObjectCount());
}

TEST_F(PausableObjectTest, MoveToActiveDocument) {
  EXPECT_CALL(PausableObject(), Unpause());
  PausableObject().DidMoveToNewExecutionContext(&DestDocument());
}

TEST_F(PausableObjectTest, MoveToSuspendedDocument) {
  DestDocument().PauseScheduledTasks();

  EXPECT_CALL(PausableObject(), Pause());
  PausableObject().DidMoveToNewExecutionContext(&DestDocument());
}

TEST_F(PausableObjectTest, MoveToStoppedDocument) {
  DestDocument().Shutdown();

  EXPECT_CALL(PausableObject(), ContextDestroyed(&DestDocument()));
  PausableObject().DidMoveToNewExecutionContext(&DestDocument());
}

}  // namespace blink
