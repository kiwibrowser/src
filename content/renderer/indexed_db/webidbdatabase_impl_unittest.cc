// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <stddef.h>
#include <stdint.h>

#include <memory>

#include "base/macros.h"
#include "base/threading/thread_task_runner_handle.h"
#include "content/child/thread_safe_sender.h"
#include "content/common/indexed_db/indexed_db_key.h"
#include "content/public/test/test_browser_thread_bundle.h"
#include "content/renderer/indexed_db/mock_webidbcallbacks.h"
#include "content/renderer/indexed_db/webidbdatabase_impl.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/blink/public/platform/scheduler/test/renderer_scheduler_test_support.h"
#include "third_party/blink/public/platform/web_blob_info.h"
#include "third_party/blink/public/platform/web_data.h"
#include "third_party/blink/public/web/web_heap.h"

using blink::WebBlobInfo;
using blink::WebData;
using blink::WebIDBCursor;
using blink::WebIDBKey;
using blink::WebVector;
using testing::_;
using testing::Invoke;
using testing::StrictMock;
using testing::WithArgs;

namespace content {

class WebIDBDatabaseImplTest : public testing::Test {
 public:
  WebIDBDatabaseImplTest() {}

  void TearDown() override { blink::WebHeap::CollectAllGarbageForTesting(); }

 private:
  TestBrowserThreadBundle thread_bundle_;

  DISALLOW_COPY_AND_ASSIGN(WebIDBDatabaseImplTest);
};

TEST_F(WebIDBDatabaseImplTest, ValueSizeTest) {
  // For testing use a much smaller maximum size to prevent allocating >100 MB
  // of memory, which crashes on memory-constrained systems.
  const size_t kMaxValueSizeForTesting = 10 * 1024 * 1024;  // 10 MB

  const std::vector<char> data(kMaxValueSizeForTesting + 1);
  const WebData value(&data.front(), data.size());
  const WebVector<WebBlobInfo> web_blob_info;

  const int64_t transaction_id = 1;
  const int64_t object_store_id = 2;

  StrictMock<MockWebIDBCallbacks> callbacks;
  EXPECT_CALL(callbacks, OnError(_)).Times(1);

  WebIDBDatabaseImpl database_impl(
      nullptr, blink::scheduler::GetSingleThreadTaskRunnerForTesting(),
      blink::scheduler::GetSingleThreadTaskRunnerForTesting());
  database_impl.max_put_value_size_ = kMaxValueSizeForTesting;
  const WebIDBKey idb_key = WebIDBKey::CreateNumber(0);
  database_impl.Put(transaction_id, object_store_id, value, web_blob_info,
                    idb_key.View(), blink::kWebIDBPutModeAddOrUpdate,
                    &callbacks, WebVector<long long>(),
                    WebVector<WebVector<WebIDBKey>>());
}

TEST_F(WebIDBDatabaseImplTest, KeyAndValueSizeTest) {
  // For testing use a much smaller maximum size to prevent allocating >100 MB
  // of memory, which crashes on memory-constrained systems.
  const size_t kMaxValueSizeForTesting = 10 * 1024 * 1024;  // 10 MB
  const size_t kKeySize = 1024 * 1024;

  const std::vector<char> data(kMaxValueSizeForTesting - kKeySize);
  const WebData value(&data.front(), data.size());
  const WebVector<WebBlobInfo> web_blob_info;
  const WebIDBKey key = WebIDBKey::CreateString(blink::WebString::FromUTF16(
      base::string16(kKeySize / sizeof(base::string16::value_type), 'x')));

  const int64_t transaction_id = 1;
  const int64_t object_store_id = 2;

  StrictMock<MockWebIDBCallbacks> callbacks;
  EXPECT_CALL(callbacks, OnError(_)).Times(1);

  WebIDBDatabaseImpl database_impl(
      nullptr, blink::scheduler::GetSingleThreadTaskRunnerForTesting(),
      blink::scheduler::GetSingleThreadTaskRunnerForTesting());
  database_impl.max_put_value_size_ = kMaxValueSizeForTesting;
  database_impl.Put(transaction_id, object_store_id, value, web_blob_info,
                    key.View(), blink::kWebIDBPutModeAddOrUpdate, &callbacks,
                    WebVector<long long>(), WebVector<WebVector<WebIDBKey>>());
}

}  // namespace content
