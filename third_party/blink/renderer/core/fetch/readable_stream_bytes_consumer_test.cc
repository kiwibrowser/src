// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/core/fetch/readable_stream_bytes_consumer.h"

#include <memory>

#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/blink/renderer/bindings/core/v8/v8_binding_for_core.h"
#include "third_party/blink/renderer/bindings/core/v8/v8_gc_controller.h"
#include "third_party/blink/renderer/core/fetch/bytes_consumer_test_util.h"
#include "third_party/blink/renderer/core/streams/readable_stream_operations.h"
#include "third_party/blink/renderer/core/testing/dummy_page_holder.h"
#include "third_party/blink/renderer/platform/bindings/script_state.h"
#include "third_party/blink/renderer/platform/bindings/v8_binding_macros.h"
#include "third_party/blink/renderer/platform/heap/handle.h"
#include "third_party/blink/renderer/platform/testing/unit_test_helpers.h"
#include "v8/include/v8.h"

namespace blink {

namespace {

using testing::InSequence;
using testing::StrictMock;
using Checkpoint = StrictMock<testing::MockFunction<void(int)>>;
using Result = BytesConsumer::Result;
using PublicState = BytesConsumer::PublicState;

class MockClient : public GarbageCollectedFinalized<MockClient>,
                   public BytesConsumer::Client {
  USING_GARBAGE_COLLECTED_MIXIN(MockClient);

 public:
  static MockClient* Create() { return new StrictMock<MockClient>(); }
  MOCK_METHOD0(OnStateChange, void());
  String DebugName() const override { return "MockClient"; }

  void Trace(blink::Visitor* visitor) override {}

 protected:
  MockClient() = default;
};

class ReadableStreamBytesConsumerTest : public testing::Test {
 public:
  ReadableStreamBytesConsumerTest() : page_(DummyPageHolder::Create()) {}

  ScriptState* GetScriptState() {
    return ToScriptStateForMainWorld(page_->GetDocument().GetFrame());
  }
  v8::Isolate* GetIsolate() { return GetScriptState()->GetIsolate(); }

  v8::MaybeLocal<v8::Value> Eval(const char* s) {
    v8::Local<v8::String> source;
    v8::Local<v8::Script> script;
    v8::MicrotasksScope microtasks(GetIsolate(),
                                   v8::MicrotasksScope::kDoNotRunMicrotasks);
    if (!v8::String::NewFromUtf8(GetIsolate(), s, v8::NewStringType::kNormal)
             .ToLocal(&source)) {
      ADD_FAILURE();
      return v8::MaybeLocal<v8::Value>();
    }
    if (!v8::Script::Compile(GetScriptState()->GetContext(), source)
             .ToLocal(&script)) {
      ADD_FAILURE() << "Compilation fails";
      return v8::MaybeLocal<v8::Value>();
    }
    return script->Run(GetScriptState()->GetContext());
  }
  v8::MaybeLocal<v8::Value> EvalWithPrintingError(const char* s) {
    v8::TryCatch block(GetIsolate());
    v8::MaybeLocal<v8::Value> r = Eval(s);
    if (block.HasCaught()) {
      ADD_FAILURE() << ToCoreString(block.Exception()->ToString(GetIsolate()))
                           .Utf8()
                           .data();
      block.ReThrow();
    }
    return r;
  }

  ReadableStreamBytesConsumer* CreateConsumer(ScriptValue stream) {
    NonThrowableExceptionState es;
    ScriptValue reader =
        ReadableStreamOperations::GetReader(GetScriptState(), stream, es);
    DCHECK(!reader.IsEmpty());
    DCHECK(reader.V8Value()->IsObject());
    return new ReadableStreamBytesConsumer(GetScriptState(), reader);
  }

  void Gc() { V8GCController::CollectAllGarbageForTesting(GetIsolate()); }

 private:
  std::unique_ptr<DummyPageHolder> page_;
};

TEST_F(ReadableStreamBytesConsumerTest, Create) {
  ScriptState::Scope scope(GetScriptState());
  ScriptValue stream(GetScriptState(),
                     EvalWithPrintingError("new ReadableStream"));
  ASSERT_FALSE(stream.IsEmpty());
  Persistent<BytesConsumer> consumer = CreateConsumer(stream);

  EXPECT_EQ(PublicState::kReadableOrWaiting, consumer->GetPublicState());
}

TEST_F(ReadableStreamBytesConsumerTest, EmptyStream) {
  ScriptState::Scope scope(GetScriptState());
  ScriptValue stream(
      GetScriptState(),
      EvalWithPrintingError("new ReadableStream({start: c => c.close()})"));
  ASSERT_FALSE(stream.IsEmpty());
  Persistent<BytesConsumer> consumer = CreateConsumer(stream);
  Persistent<MockClient> client = MockClient::Create();
  consumer->SetClient(client);

  Checkpoint checkpoint;
  InSequence s;
  EXPECT_CALL(checkpoint, Call(1));
  EXPECT_CALL(checkpoint, Call(2));
  EXPECT_CALL(checkpoint, Call(3));
  EXPECT_CALL(*client, OnStateChange());
  EXPECT_CALL(checkpoint, Call(4));

  const char* buffer = nullptr;
  size_t available = 0;
  checkpoint.Call(1);
  test::RunPendingTasks();
  checkpoint.Call(2);
  EXPECT_EQ(PublicState::kReadableOrWaiting, consumer->GetPublicState());
  EXPECT_EQ(Result::kShouldWait, consumer->BeginRead(&buffer, &available));
  checkpoint.Call(3);
  test::RunPendingTasks();
  checkpoint.Call(4);
  EXPECT_EQ(PublicState::kClosed, consumer->GetPublicState());
  EXPECT_EQ(Result::kDone, consumer->BeginRead(&buffer, &available));
}

TEST_F(ReadableStreamBytesConsumerTest, ErroredStream) {
  ScriptState::Scope scope(GetScriptState());
  ScriptValue stream(
      GetScriptState(),
      EvalWithPrintingError("new ReadableStream({start: c => c.error()})"));
  ASSERT_FALSE(stream.IsEmpty());
  Persistent<BytesConsumer> consumer = CreateConsumer(stream);
  Persistent<MockClient> client = MockClient::Create();
  consumer->SetClient(client);
  Checkpoint checkpoint;

  InSequence s;
  EXPECT_CALL(checkpoint, Call(1));
  EXPECT_CALL(checkpoint, Call(2));
  EXPECT_CALL(checkpoint, Call(3));
  EXPECT_CALL(*client, OnStateChange());
  EXPECT_CALL(checkpoint, Call(4));

  const char* buffer = nullptr;
  size_t available = 0;
  checkpoint.Call(1);
  test::RunPendingTasks();
  checkpoint.Call(2);
  EXPECT_EQ(PublicState::kReadableOrWaiting, consumer->GetPublicState());
  EXPECT_EQ(Result::kShouldWait, consumer->BeginRead(&buffer, &available));
  checkpoint.Call(3);
  test::RunPendingTasks();
  checkpoint.Call(4);
  EXPECT_EQ(PublicState::kErrored, consumer->GetPublicState());
  EXPECT_EQ(Result::kError, consumer->BeginRead(&buffer, &available));
}

TEST_F(ReadableStreamBytesConsumerTest, TwoPhaseRead) {
  ScriptState::Scope scope(GetScriptState());
  ScriptValue stream(
      GetScriptState(),
      EvalWithPrintingError(
          "var controller;"
          "var stream = new ReadableStream({start: c => controller = c});"
          "controller.enqueue(new Uint8Array());"
          "controller.enqueue(new Uint8Array([0x43, 0x44, 0x45, 0x46]));"
          "controller.enqueue(new Uint8Array([0x47, 0x48, 0x49, 0x4a]));"
          "controller.close();"
          "stream"));
  ASSERT_FALSE(stream.IsEmpty());
  Persistent<BytesConsumer> consumer = CreateConsumer(stream);
  Persistent<MockClient> client = MockClient::Create();
  consumer->SetClient(client);
  Checkpoint checkpoint;

  InSequence s;
  EXPECT_CALL(checkpoint, Call(1));
  EXPECT_CALL(checkpoint, Call(2));
  EXPECT_CALL(checkpoint, Call(3));
  EXPECT_CALL(*client, OnStateChange());
  EXPECT_CALL(checkpoint, Call(4));
  EXPECT_CALL(checkpoint, Call(5));
  EXPECT_CALL(*client, OnStateChange());
  EXPECT_CALL(checkpoint, Call(6));
  EXPECT_CALL(checkpoint, Call(7));
  EXPECT_CALL(*client, OnStateChange());
  EXPECT_CALL(checkpoint, Call(8));
  EXPECT_CALL(checkpoint, Call(9));
  EXPECT_CALL(*client, OnStateChange());
  EXPECT_CALL(checkpoint, Call(10));

  const char* buffer = nullptr;
  size_t available = 0;
  checkpoint.Call(1);
  test::RunPendingTasks();
  checkpoint.Call(2);
  EXPECT_EQ(Result::kShouldWait, consumer->BeginRead(&buffer, &available));
  checkpoint.Call(3);
  test::RunPendingTasks();
  checkpoint.Call(4);
  EXPECT_EQ(Result::kOk, consumer->BeginRead(&buffer, &available));
  ASSERT_EQ(0u, available);
  EXPECT_EQ(Result::kOk, consumer->EndRead(0));
  EXPECT_EQ(Result::kShouldWait, consumer->BeginRead(&buffer, &available));
  checkpoint.Call(5);
  test::RunPendingTasks();
  checkpoint.Call(6);
  EXPECT_EQ(PublicState::kReadableOrWaiting, consumer->GetPublicState());
  EXPECT_EQ(Result::kOk, consumer->BeginRead(&buffer, &available));
  ASSERT_EQ(4u, available);
  EXPECT_EQ(0x43, buffer[0]);
  EXPECT_EQ(0x44, buffer[1]);
  EXPECT_EQ(0x45, buffer[2]);
  EXPECT_EQ(0x46, buffer[3]);
  EXPECT_EQ(Result::kOk, consumer->EndRead(0));
  EXPECT_EQ(Result::kOk, consumer->BeginRead(&buffer, &available));
  ASSERT_EQ(4u, available);
  EXPECT_EQ(0x43, buffer[0]);
  EXPECT_EQ(0x44, buffer[1]);
  EXPECT_EQ(0x45, buffer[2]);
  EXPECT_EQ(0x46, buffer[3]);
  EXPECT_EQ(Result::kOk, consumer->EndRead(1));
  EXPECT_EQ(Result::kOk, consumer->BeginRead(&buffer, &available));
  ASSERT_EQ(3u, available);
  EXPECT_EQ(0x44, buffer[0]);
  EXPECT_EQ(0x45, buffer[1]);
  EXPECT_EQ(0x46, buffer[2]);
  EXPECT_EQ(Result::kOk, consumer->EndRead(3));
  EXPECT_EQ(Result::kShouldWait, consumer->BeginRead(&buffer, &available));
  checkpoint.Call(7);
  test::RunPendingTasks();
  checkpoint.Call(8);
  EXPECT_EQ(PublicState::kReadableOrWaiting, consumer->GetPublicState());
  EXPECT_EQ(Result::kOk, consumer->BeginRead(&buffer, &available));
  ASSERT_EQ(4u, available);
  EXPECT_EQ(0x47, buffer[0]);
  EXPECT_EQ(0x48, buffer[1]);
  EXPECT_EQ(0x49, buffer[2]);
  EXPECT_EQ(0x4a, buffer[3]);
  EXPECT_EQ(Result::kOk, consumer->EndRead(4));
  EXPECT_EQ(Result::kShouldWait, consumer->BeginRead(&buffer, &available));
  checkpoint.Call(9);
  test::RunPendingTasks();
  checkpoint.Call(10);
  EXPECT_EQ(PublicState::kClosed, consumer->GetPublicState());
  EXPECT_EQ(Result::kDone, consumer->BeginRead(&buffer, &available));
}

TEST_F(ReadableStreamBytesConsumerTest, EnqueueUndefined) {
  ScriptState::Scope scope(GetScriptState());
  ScriptValue stream(
      GetScriptState(),
      EvalWithPrintingError(
          "var controller;"
          "var stream = new ReadableStream({start: c => controller = c});"
          "controller.enqueue(undefined);"
          "controller.close();"
          "stream"));
  ASSERT_FALSE(stream.IsEmpty());
  Persistent<BytesConsumer> consumer = CreateConsumer(stream);
  Persistent<MockClient> client = MockClient::Create();
  consumer->SetClient(client);
  Checkpoint checkpoint;

  InSequence s;
  EXPECT_CALL(checkpoint, Call(1));
  EXPECT_CALL(checkpoint, Call(2));
  EXPECT_CALL(checkpoint, Call(3));
  EXPECT_CALL(*client, OnStateChange());
  EXPECT_CALL(checkpoint, Call(4));

  const char* buffer = nullptr;
  size_t available = 0;
  checkpoint.Call(1);
  test::RunPendingTasks();
  checkpoint.Call(2);
  EXPECT_EQ(PublicState::kReadableOrWaiting, consumer->GetPublicState());
  EXPECT_EQ(Result::kShouldWait, consumer->BeginRead(&buffer, &available));
  checkpoint.Call(3);
  test::RunPendingTasks();
  checkpoint.Call(4);
  EXPECT_EQ(PublicState::kErrored, consumer->GetPublicState());
  EXPECT_EQ(Result::kError, consumer->BeginRead(&buffer, &available));
}

TEST_F(ReadableStreamBytesConsumerTest, EnqueueNull) {
  ScriptState::Scope scope(GetScriptState());
  ScriptValue stream(
      GetScriptState(),
      EvalWithPrintingError(
          "var controller;"
          "var stream = new ReadableStream({start: c => controller = c});"
          "controller.enqueue(null);"
          "controller.close();"
          "stream"));

  ASSERT_FALSE(stream.IsEmpty());
  Persistent<BytesConsumer> consumer = CreateConsumer(stream);
  Persistent<MockClient> client = MockClient::Create();
  consumer->SetClient(client);
  Checkpoint checkpoint;

  InSequence s;
  EXPECT_CALL(checkpoint, Call(1));
  EXPECT_CALL(checkpoint, Call(2));
  EXPECT_CALL(checkpoint, Call(3));
  EXPECT_CALL(*client, OnStateChange());
  EXPECT_CALL(checkpoint, Call(4));

  const char* buffer = nullptr;
  size_t available = 0;
  checkpoint.Call(1);
  test::RunPendingTasks();
  checkpoint.Call(2);
  EXPECT_EQ(PublicState::kReadableOrWaiting, consumer->GetPublicState());
  EXPECT_EQ(Result::kShouldWait, consumer->BeginRead(&buffer, &available));
  checkpoint.Call(3);
  test::RunPendingTasks();
  checkpoint.Call(4);
  EXPECT_EQ(PublicState::kErrored, consumer->GetPublicState());
  EXPECT_EQ(Result::kError, consumer->BeginRead(&buffer, &available));
}

TEST_F(ReadableStreamBytesConsumerTest, EnqueueString) {
  ScriptState::Scope scope(GetScriptState());
  ScriptValue stream(
      GetScriptState(),
      EvalWithPrintingError(
          "var controller;"
          "var stream = new ReadableStream({start: c => controller = c});"
          "controller.enqueue('hello');"
          "controller.close();"
          "stream"));
  ASSERT_FALSE(stream.IsEmpty());
  Persistent<BytesConsumer> consumer = CreateConsumer(stream);
  Persistent<MockClient> client = MockClient::Create();
  consumer->SetClient(client);
  Checkpoint checkpoint;

  InSequence s;
  EXPECT_CALL(checkpoint, Call(1));
  EXPECT_CALL(checkpoint, Call(2));
  EXPECT_CALL(checkpoint, Call(3));
  EXPECT_CALL(*client, OnStateChange());
  EXPECT_CALL(checkpoint, Call(4));

  const char* buffer = nullptr;
  size_t available = 0;
  checkpoint.Call(1);
  test::RunPendingTasks();
  checkpoint.Call(2);
  EXPECT_EQ(PublicState::kReadableOrWaiting, consumer->GetPublicState());
  EXPECT_EQ(Result::kShouldWait, consumer->BeginRead(&buffer, &available));
  checkpoint.Call(3);
  test::RunPendingTasks();
  checkpoint.Call(4);
  EXPECT_EQ(PublicState::kErrored, consumer->GetPublicState());
  EXPECT_EQ(Result::kError, consumer->BeginRead(&buffer, &available));
}

}  // namespace

}  // namespace blink
