// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/renderer/service_worker/thread_safe_script_container.h"

#include "base/bind.h"
#include "base/synchronization/waitable_event.h"
#include "base/threading/thread.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace content {

using ScriptStatus = ThreadSafeScriptContainer::ScriptStatus;

class ThreadSafeScriptContainerTest : public testing::Test {
 public:
  ThreadSafeScriptContainerTest()
      : writer_thread_("writer_thread"),
        reader_thread_("reader_thread"),
        writer_waiter_(base::WaitableEvent::ResetPolicy::AUTOMATIC,
                       base::WaitableEvent::InitialState::NOT_SIGNALED),
        reader_waiter_(base::WaitableEvent::ResetPolicy::AUTOMATIC,
                       base::WaitableEvent::InitialState::NOT_SIGNALED),
        container_(base::MakeRefCounted<ThreadSafeScriptContainer>()) {}

 protected:
  void SetUp() override {
    ASSERT_TRUE(writer_thread_.Start());
    ASSERT_TRUE(reader_thread_.Start());
    writer_task_runner_ = writer_thread_.task_runner();
    reader_task_runner_ = reader_thread_.task_runner();
  }

  void TearDown() override {
    writer_thread_.Stop();
    reader_thread_.Stop();
  }

  base::WaitableEvent* AddOnWriterThread(
      const GURL& url,
      ThreadSafeScriptContainer::Data** out_data) {
    writer_task_runner_->PostTask(
        FROM_HERE,
        base::BindOnce(
            [](scoped_refptr<ThreadSafeScriptContainer> container,
               const GURL& url, ThreadSafeScriptContainer::Data** out_data,
               base::WaitableEvent* waiter) {
              auto data = ThreadSafeScriptContainer::Data::Create(
                  blink::WebString::FromUTF8("utf-8") /* encoding */,
                  blink::WebVector<blink::WebVector<char>>() /* script_text */,
                  blink::WebVector<blink::WebVector<char>>() /* meta_data */);
              *out_data = data.get();
              container->AddOnIOThread(url, std::move(data));
              waiter->Signal();
            },
            container_, url, out_data, &writer_waiter_));
    return &writer_waiter_;
  }

  base::WaitableEvent* OnAllDataAddedOnWriterThread() {
    writer_task_runner_->PostTask(
        FROM_HERE, base::BindOnce(
                       [](scoped_refptr<ThreadSafeScriptContainer> container,
                          base::WaitableEvent* waiter) {
                         container->OnAllDataAddedOnIOThread();
                         waiter->Signal();
                       },
                       container_, &writer_waiter_));
    return &writer_waiter_;
  }

  base::WaitableEvent* GetStatusOnReaderThread(const GURL& url,
                                               ScriptStatus* out_status) {
    reader_task_runner_->PostTask(
        FROM_HERE, base::BindOnce(
                       [](scoped_refptr<ThreadSafeScriptContainer> container,
                          const GURL& url, ScriptStatus* out_status,
                          base::WaitableEvent* waiter) {
                         *out_status = container->GetStatusOnWorkerThread(url);
                         waiter->Signal();
                       },
                       container_, url, out_status, &reader_waiter_));
    return &reader_waiter_;
  }

  base::WaitableEvent* WaitOnReaderThread(const GURL& url, bool* out_exists) {
    reader_task_runner_->PostTask(
        FROM_HERE,
        base::BindOnce(
            [](scoped_refptr<ThreadSafeScriptContainer> container,
               const GURL& url, bool* out_exists, base::WaitableEvent* waiter) {
              *out_exists = container->WaitOnWorkerThread(url);
              waiter->Signal();
            },
            container_, url, out_exists, &reader_waiter_));
    return &reader_waiter_;
  }

  base::WaitableEvent* TakeOnReaderThread(
      const GURL& url,
      ThreadSafeScriptContainer::Data** out_data) {
    reader_task_runner_->PostTask(
        FROM_HERE,
        base::BindOnce(
            [](scoped_refptr<ThreadSafeScriptContainer> container,
               const GURL& url, ThreadSafeScriptContainer::Data** out_data,
               base::WaitableEvent* waiter) {
              auto data = container->TakeOnWorkerThread(url);
              *out_data = data.get();
              waiter->Signal();
            },
            container_, url, out_data, &reader_waiter_));
    return &reader_waiter_;
  }

 private:
  base::Thread writer_thread_;
  base::Thread reader_thread_;

  scoped_refptr<base::SingleThreadTaskRunner> writer_task_runner_;
  scoped_refptr<base::SingleThreadTaskRunner> reader_task_runner_;

  base::WaitableEvent writer_waiter_;
  base::WaitableEvent reader_waiter_;

  scoped_refptr<ThreadSafeScriptContainer> container_;
};

TEST_F(ThreadSafeScriptContainerTest, WaitExistingKey) {
  const GURL kKey("https://example.com/key");
  {
    ScriptStatus result = ScriptStatus::kReceived;
    GetStatusOnReaderThread(kKey, &result)->Wait();
    EXPECT_EQ(ScriptStatus::kPending, result);
  }

  ThreadSafeScriptContainer::Data* added_data;
  {
    bool result = false;
    base::WaitableEvent* pending_wait = WaitOnReaderThread(kKey, &result);
    // This should not be signaled until data is added.
    EXPECT_FALSE(pending_wait->IsSignaled());
    base::WaitableEvent* pending_write = AddOnWriterThread(kKey, &added_data);
    pending_wait->Wait();
    pending_write->Wait();
    EXPECT_TRUE(result);
  }

  {
    ScriptStatus result = ScriptStatus::kFailed;
    GetStatusOnReaderThread(kKey, &result)->Wait();
    EXPECT_EQ(ScriptStatus::kReceived, result);
  }

  {
    ThreadSafeScriptContainer::Data* taken_data;
    TakeOnReaderThread(kKey, &taken_data)->Wait();
    EXPECT_EQ(added_data, taken_data);
  }

  {
    ScriptStatus result = ScriptStatus::kFailed;
    GetStatusOnReaderThread(kKey, &result)->Wait();
    // The record of |kKey| should be exist though it's already taken.
    EXPECT_EQ(ScriptStatus::kTaken, result);
  }

  {
    bool result = false;
    WaitOnReaderThread(kKey, &result)->Wait();
    // Waiting for |kKey| should succeed.
    EXPECT_TRUE(result);

    ThreadSafeScriptContainer::Data* taken_data;
    TakeOnReaderThread(kKey, &taken_data)->Wait();
    // |taken_data| should be nullptr because it's already taken.
    EXPECT_EQ(nullptr, taken_data);
  }

  // Finish adding data.
  OnAllDataAddedOnWriterThread()->Wait();

  {
    bool result = false;
    WaitOnReaderThread(kKey, &result)->Wait();
    // The record has been already added, so Wait shouldn't fail.
    EXPECT_TRUE(result);

    ThreadSafeScriptContainer::Data* taken_data;
    TakeOnReaderThread(kKey, &taken_data)->Wait();
    // |taken_data| should be nullptr because it's already taken.
    EXPECT_EQ(nullptr, taken_data);
  }
}

TEST_F(ThreadSafeScriptContainerTest, WaitNonExistingKey) {
  const GURL kKey("https://example.com/key");
  {
    ScriptStatus result = ScriptStatus::kReceived;
    GetStatusOnReaderThread(kKey, &result)->Wait();
    EXPECT_EQ(ScriptStatus::kPending, result);
  }

  {
    bool result = true;
    base::WaitableEvent* pending_wait = WaitOnReaderThread(kKey, &result);
    // This should not be signaled until OnAllDataAdded is called.
    EXPECT_FALSE(pending_wait->IsSignaled());
    base::WaitableEvent* pending_on_all_data_added =
        OnAllDataAddedOnWriterThread();
    pending_wait->Wait();
    pending_on_all_data_added->Wait();
    // Aborted wait should return false.
    EXPECT_FALSE(result);
  }

  {
    bool result = true;
    WaitOnReaderThread(kKey, &result)->Wait();
    // Wait fails immediately because OnAllDataAdded is called.
    EXPECT_FALSE(result);
  }
}

}  // namespace content
