/*
 * Copyright (C) 2012 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE AND ITS CONTRIBUTORS "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "third_party/blink/renderer/modules/indexeddb/idb_request.h"

#include <memory>

#include "base/memory/scoped_refptr.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/blink/public/platform/modules/indexeddb/web_idb_callbacks.h"
#include "third_party/blink/public/platform/web_url_loader_mock_factory.h"
#include "third_party/blink/public/platform/web_url_response.h"
#include "third_party/blink/renderer/bindings/core/v8/v8_binding_for_core.h"
#include "third_party/blink/renderer/bindings/core/v8/v8_binding_for_testing.h"
#include "third_party/blink/renderer/core/dom/dom_exception.h"
#include "third_party/blink/renderer/core/dom/exception_code.h"
#include "third_party/blink/renderer/core/execution_context/execution_context.h"
#include "third_party/blink/renderer/core/testing/null_execution_context.h"
#include "third_party/blink/renderer/modules/indexeddb/idb_database.h"
#include "third_party/blink/renderer/modules/indexeddb/idb_database_callbacks.h"
#include "third_party/blink/renderer/modules/indexeddb/idb_key.h"
#include "third_party/blink/renderer/modules/indexeddb/idb_key_path.h"
#include "third_party/blink/renderer/modules/indexeddb/idb_metadata.h"
#include "third_party/blink/renderer/modules/indexeddb/idb_object_store.h"
#include "third_party/blink/renderer/modules/indexeddb/idb_open_db_request.h"
#include "third_party/blink/renderer/modules/indexeddb/idb_test_helper.h"
#include "third_party/blink/renderer/modules/indexeddb/idb_transaction.h"
#include "third_party/blink/renderer/modules/indexeddb/idb_value.h"
#include "third_party/blink/renderer/modules/indexeddb/idb_value_wrapping.h"
#include "third_party/blink/renderer/modules/indexeddb/mock_web_idb_database.h"
#include "third_party/blink/renderer/platform/bindings/script_state.h"
#include "third_party/blink/renderer/platform/shared_buffer.h"
#include "third_party/blink/renderer/platform/testing/testing_platform_support.h"
#include "third_party/blink/renderer/platform/wtf/dtoa/utils.h"
#include "third_party/blink/renderer/platform/wtf/vector.h"
#include "v8/include/v8.h"

namespace blink {
namespace {

class IDBRequestTest : public testing::Test {
 protected:
  void SetUp() override {
    url_loader_mock_factory_ = platform_->GetURLLoaderMockFactory();
    WebURLResponse response;
    response.SetURL(KURL("blob:"));
    url_loader_mock_factory_->RegisterURLProtocol(WebString("blob"), response,
                                                  "");
  }

  void TearDown() override {
    url_loader_mock_factory_->UnregisterAllURLsAndClearMemoryCache();
  }

  void BuildTransaction(V8TestingScope& scope,
                        std::unique_ptr<MockWebIDBDatabase> backend) {
    db_ =
        IDBDatabase::Create(scope.GetExecutionContext(), std::move(backend),
                            IDBDatabaseCallbacks::Create(), scope.GetIsolate());

    HashSet<String> transaction_scope = {"store"};
    transaction_ = IDBTransaction::CreateNonVersionChange(
        scope.GetScriptState(), kTransactionId, transaction_scope,
        kWebIDBTransactionModeReadOnly, db_.Get());

    IDBKeyPath store_key_path("primaryKey");
    scoped_refptr<IDBObjectStoreMetadata> store_metadata = base::AdoptRef(
        new IDBObjectStoreMetadata("store", kStoreId, store_key_path, true, 1));
    store_ = IDBObjectStore::Create(store_metadata, transaction_);
  }

  WebURLLoaderMockFactory* url_loader_mock_factory_;
  Persistent<IDBDatabase> db_;
  Persistent<IDBTransaction> transaction_;
  Persistent<IDBObjectStore> store_;
  ScopedTestingPlatformSupport<TestingPlatformSupport> platform_;

  static constexpr int64_t kTransactionId = 1234;
  static constexpr int64_t kStoreId = 5678;
};

void EnsureIDBCallbacksDontThrow(IDBRequest* request,
                                 ExceptionState& exception_state) {
  ASSERT_TRUE(request->transaction());
  V8TestingScope scope;

  request->HandleResponse(
      DOMException::Create(kAbortError, "Description goes here."));
  request->HandleResponse(nullptr, IDBKey::CreateInvalid(),
                          IDBKey::CreateInvalid(),
                          CreateNullIDBValueForTesting(scope.GetIsolate()));
  request->HandleResponse(IDBKey::CreateInvalid());
  request->HandleResponse(CreateNullIDBValueForTesting(scope.GetIsolate()));
  request->HandleResponse(static_cast<int64_t>(0));
  request->HandleResponse();
  request->HandleResponse(IDBKey::CreateInvalid(), IDBKey::CreateInvalid(),
                          CreateNullIDBValueForTesting(scope.GetIsolate()));
  request->EnqueueResponse(Vector<String>());

  EXPECT_TRUE(!exception_state.HadException());
}

TEST_F(IDBRequestTest, EventsAfterEarlyDeathStop) {
  V8TestingScope scope;
  std::unique_ptr<MockWebIDBDatabase> backend = MockWebIDBDatabase::Create();
  EXPECT_CALL(*backend, Close()).Times(1);
  BuildTransaction(scope, std::move(backend));

  ASSERT_TRUE(!scope.GetExceptionState().HadException());
  ASSERT_TRUE(transaction_);

  IDBRequest* request =
      IDBRequest::Create(scope.GetScriptState(), store_.Get(),
                         transaction_.Get(), IDBRequest::AsyncTraceState());

  EXPECT_EQ(request->readyState(), "pending");
  ASSERT_TRUE(!scope.GetExceptionState().HadException());
  ASSERT_TRUE(request->transaction());
  scope.GetExecutionContext()->NotifyContextDestroyed();

  EnsureIDBCallbacksDontThrow(request, scope.GetExceptionState());
}

TEST_F(IDBRequestTest, EventsAfterDoneStop) {
  V8TestingScope scope;
  std::unique_ptr<MockWebIDBDatabase> backend = MockWebIDBDatabase::Create();
  EXPECT_CALL(*backend, Close()).Times(1);
  BuildTransaction(scope, std::move(backend));

  ASSERT_TRUE(!scope.GetExceptionState().HadException());
  ASSERT_TRUE(transaction_);

  IDBRequest* request =
      IDBRequest::Create(scope.GetScriptState(), store_.Get(),
                         transaction_.Get(), IDBRequest::AsyncTraceState());
  ASSERT_TRUE(!scope.GetExceptionState().HadException());
  ASSERT_TRUE(request->transaction());
  request->HandleResponse(CreateIDBValueForTesting(scope.GetIsolate(), false));
  scope.GetExecutionContext()->NotifyContextDestroyed();

  EnsureIDBCallbacksDontThrow(request, scope.GetExceptionState());
}

TEST_F(IDBRequestTest, EventsAfterEarlyDeathStopWithQueuedResult) {
  V8TestingScope scope;
  std::unique_ptr<MockWebIDBDatabase> backend = MockWebIDBDatabase::Create();
  EXPECT_CALL(*backend, Close()).Times(1);
  BuildTransaction(scope, std::move(backend));

  ASSERT_TRUE(!scope.GetExceptionState().HadException());
  ASSERT_TRUE(transaction_);

  IDBRequest* request =
      IDBRequest::Create(scope.GetScriptState(), store_.Get(),
                         transaction_.Get(), IDBRequest::AsyncTraceState());
  EXPECT_EQ(request->readyState(), "pending");
  ASSERT_TRUE(!scope.GetExceptionState().HadException());
  ASSERT_TRUE(request->transaction());
  request->HandleResponse(CreateIDBValueForTesting(scope.GetIsolate(), true));
  scope.GetExecutionContext()->NotifyContextDestroyed();

  EnsureIDBCallbacksDontThrow(request, scope.GetExceptionState());
  url_loader_mock_factory_->ServeAsynchronousRequests();
  EnsureIDBCallbacksDontThrow(request, scope.GetExceptionState());
}

TEST_F(IDBRequestTest, EventsAfterEarlyDeathStopWithTwoQueuedResults) {
  V8TestingScope scope;
  std::unique_ptr<MockWebIDBDatabase> backend = MockWebIDBDatabase::Create();
  EXPECT_CALL(*backend, Close()).Times(1);
  BuildTransaction(scope, std::move(backend));

  ASSERT_TRUE(!scope.GetExceptionState().HadException());
  ASSERT_TRUE(transaction_);

  IDBRequest* request1 =
      IDBRequest::Create(scope.GetScriptState(), store_.Get(),
                         transaction_.Get(), IDBRequest::AsyncTraceState());
  IDBRequest* request2 =
      IDBRequest::Create(scope.GetScriptState(), store_.Get(),
                         transaction_.Get(), IDBRequest::AsyncTraceState());
  EXPECT_EQ(request1->readyState(), "pending");
  EXPECT_EQ(request2->readyState(), "pending");
  ASSERT_TRUE(!scope.GetExceptionState().HadException());
  ASSERT_TRUE(request1->transaction());
  ASSERT_TRUE(request2->transaction());
  request1->HandleResponse(CreateIDBValueForTesting(scope.GetIsolate(), true));
  request2->HandleResponse(CreateIDBValueForTesting(scope.GetIsolate(), true));
  scope.GetExecutionContext()->NotifyContextDestroyed();

  EnsureIDBCallbacksDontThrow(request1, scope.GetExceptionState());
  EnsureIDBCallbacksDontThrow(request2, scope.GetExceptionState());
  url_loader_mock_factory_->ServeAsynchronousRequests();
  EnsureIDBCallbacksDontThrow(request1, scope.GetExceptionState());
  EnsureIDBCallbacksDontThrow(request2, scope.GetExceptionState());
}

TEST_F(IDBRequestTest, AbortErrorAfterAbort) {
  V8TestingScope scope;
  IDBTransaction* transaction = nullptr;
  IDBRequest* request =
      IDBRequest::Create(scope.GetScriptState(), store_.Get(), transaction,
                         IDBRequest::AsyncTraceState());
  EXPECT_EQ(request->readyState(), "pending");

  // Simulate the IDBTransaction having received OnAbort from back end and
  // aborting the request:
  request->Abort();

  // Now simulate the back end having fired an abort error at the request to
  // clear up any intermediaries.  Ensure an assertion is not raised.
  request->HandleResponse(
      DOMException::Create(kAbortError, "Description goes here."));

  // Stop the request lest it be GCed and its destructor
  // finds the object in a pending state (and asserts.)
  scope.GetExecutionContext()->NotifyContextDestroyed();
}

TEST_F(IDBRequestTest, ConnectionsAfterStopping) {
  V8TestingScope scope;
  const int64_t kTransactionId = 1234;
  const int64_t kVersion = 1;
  const int64_t kOldVersion = 0;
  const WebIDBMetadata metadata;
  Persistent<IDBDatabaseCallbacks> callbacks = IDBDatabaseCallbacks::Create();

  {
    std::unique_ptr<MockWebIDBDatabase> backend = MockWebIDBDatabase::Create();
    EXPECT_CALL(*backend, Close()).Times(1);
    IDBOpenDBRequest* request = IDBOpenDBRequest::Create(
        scope.GetScriptState(), callbacks, kTransactionId, kVersion,
        IDBRequest::AsyncTraceState());
    EXPECT_EQ(request->readyState(), "pending");
    std::unique_ptr<WebIDBCallbacks> callbacks = request->CreateWebCallbacks();

    scope.GetExecutionContext()->NotifyContextDestroyed();
    callbacks->OnUpgradeNeeded(kOldVersion, backend.release(), metadata,
                               kWebIDBDataLossNone, String());
  }

  {
    std::unique_ptr<MockWebIDBDatabase> backend = MockWebIDBDatabase::Create();
    EXPECT_CALL(*backend, Close()).Times(1);
    IDBOpenDBRequest* request = IDBOpenDBRequest::Create(
        scope.GetScriptState(), callbacks, kTransactionId, kVersion,
        IDBRequest::AsyncTraceState());
    EXPECT_EQ(request->readyState(), "pending");
    std::unique_ptr<WebIDBCallbacks> callbacks = request->CreateWebCallbacks();

    scope.GetExecutionContext()->NotifyContextDestroyed();
    callbacks->OnSuccess(backend.release(), metadata);
  }
}

// Expose private state for testing.
class AsyncTraceStateForTesting : public IDBRequest::AsyncTraceState {
 public:
  AsyncTraceStateForTesting() : IDBRequest::AsyncTraceState() {}
  AsyncTraceStateForTesting(AsyncTraceStateForTesting&& other)
      : IDBRequest::AsyncTraceState(std::move(other)) {}
  AsyncTraceStateForTesting& operator=(AsyncTraceStateForTesting&& rhs) {
    AsyncTraceState::operator=(std::move(rhs));
    return *this;
  }

  const char* trace_event_name() const {
    return IDBRequest::AsyncTraceState::trace_event_name();
  }
  size_t id() const { return IDBRequest::AsyncTraceState::id(); }

  size_t PopulateForNewEvent(const char* trace_event_name) {
    return IDBRequest::AsyncTraceState::PopulateForNewEvent(trace_event_name);
  }
};

TEST(IDBRequestAsyncTraceStateTest, EmptyConstructor) {
  AsyncTraceStateForTesting state;

  EXPECT_EQ(nullptr, state.trace_event_name());
  EXPECT_TRUE(state.IsEmpty());
}

TEST(IDBRequestAsyncTraceStateTest, PopulateForNewEvent) {
  AsyncTraceStateForTesting state1, state2, state3;

  const char* name1 = "event1";
  size_t id1 = state1.PopulateForNewEvent(name1);
  const char* name2 = "event2";
  size_t id2 = state2.PopulateForNewEvent(name2);
  const char* name3 = "event3";
  size_t id3 = state3.PopulateForNewEvent(name3);

  EXPECT_EQ(name1, state1.trace_event_name());
  EXPECT_EQ(name2, state2.trace_event_name());
  EXPECT_EQ(name3, state3.trace_event_name());
  EXPECT_EQ(id1, state1.id());
  EXPECT_EQ(id2, state2.id());
  EXPECT_EQ(id3, state3.id());

  EXPECT_NE(id1, id2);
  EXPECT_NE(id1, id3);
  EXPECT_NE(id2, id3);

  EXPECT_TRUE(!state1.IsEmpty());
  EXPECT_TRUE(!state2.IsEmpty());
  EXPECT_TRUE(!state3.IsEmpty());
}

TEST(IDBRequestAsyncTraceStateTest, MoveConstructor) {
  AsyncTraceStateForTesting source_state;
  const char* event_name = "event_name";
  size_t id = source_state.PopulateForNewEvent(event_name);

  AsyncTraceStateForTesting state(std::move(source_state));
  EXPECT_EQ(event_name, state.trace_event_name());
  EXPECT_EQ(id, state.id());
  EXPECT_TRUE(source_state.IsEmpty());
}

TEST(IDBRequestAsyncTraceStateTest, MoveAssignment) {
  AsyncTraceStateForTesting source_state;
  const char* event_name = "event_name";
  size_t id = source_state.PopulateForNewEvent(event_name);

  AsyncTraceStateForTesting state;

  EXPECT_TRUE(state.IsEmpty());
  state = std::move(source_state);
  EXPECT_EQ(event_name, state.trace_event_name());
  EXPECT_EQ(id, state.id());
  EXPECT_TRUE(source_state.IsEmpty());
}

}  // namespace
}  // namespace blink
