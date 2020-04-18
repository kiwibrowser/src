// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/child/memory/child_memory_coordinator_impl.h"

#if defined(OS_ANDROID)
#include "content/child/memory/child_memory_coordinator_impl_android.h"
#endif  // defined(OS_ANDROID)

#include <memory>

#include "base/memory/memory_coordinator_client_registry.h"
#include "base/message_loop/message_loop.h"
#include "base/run_loop.h"
#include "base/threading/thread.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace content {

class MockMemoryCoordinatorHandle : public mojom::MemoryCoordinatorHandle {
 public:
  MockMemoryCoordinatorHandle() : binding_(this) {}

  void AddChild(mojom::ChildMemoryCoordinatorPtr child) override {
    child_ = std::move(child);
  }

  mojom::MemoryCoordinatorHandlePtr Bind() {
    DCHECK(!binding_.is_bound());
    mojom::MemoryCoordinatorHandlePtr handle;
    binding_.Bind(mojo::MakeRequest(&handle));
    return handle;
  }

  mojom::ChildMemoryCoordinatorPtr& child() { return child_; }

 private:
  mojo::Binding<mojom::MemoryCoordinatorHandle> binding_;
  mojom::ChildMemoryCoordinatorPtr child_ = nullptr;
};

class ChildMemoryCoordinatorImplTest : public testing::Test,
                                       public ChildMemoryCoordinatorDelegate {
 public:
  ChildMemoryCoordinatorImplTest()
      : message_loop_(new base::MessageLoop) {
    auto parent = coordinator_handle_.Bind();
    coordinator_impl_ = CreateChildMemoryCoordinator(std::move(parent), this);
    // Needs to run loop to initalize mojo pointers including |child_| in
    // MockMemoryCoordinatorHandle.
    base::RunLoop loop;
    loop.RunUntilIdle();
  }

  void RegisterClient(base::MemoryCoordinatorClient* client) {
    base::MemoryCoordinatorClientRegistry::GetInstance()->Register(client);
  }

  void UnregisterClient(base::MemoryCoordinatorClient* client) {
    base::MemoryCoordinatorClientRegistry::GetInstance()->Unregister(client);
  }

  mojom::ChildMemoryCoordinatorPtr& coordinator() {
    return coordinator_handle_.child();
  }

  ChildMemoryCoordinatorImpl& coordinator_impl() {
    return *coordinator_impl_.get();
  }

  void ChangeState(mojom::MemoryState state) {
    base::RunLoop loop;
    coordinator()->OnStateChange(state);
    loop.RunUntilIdle();
  }

  // ChildMemoryCoordinatorDelegate implementation:
  void OnTrimMemoryImmediately() override {
    on_trim_memory_called_ = true;
  }

 protected:
  bool on_trim_memory_called_ = false;

 private:
  std::unique_ptr<base::MessageLoop> message_loop_;
  MockMemoryCoordinatorHandle coordinator_handle_;
  std::unique_ptr<ChildMemoryCoordinatorImpl> coordinator_impl_;

  DISALLOW_COPY_AND_ASSIGN(ChildMemoryCoordinatorImplTest);
};

namespace {

class MockMemoryCoordinatorClient final : public base::MemoryCoordinatorClient {
public:
  void OnMemoryStateChange(base::MemoryState state) override {
    last_state_ = state;
  }

  base::MemoryState last_state() const { return last_state_; }

 private:
  base::MemoryState last_state_ = base::MemoryState::UNKNOWN;
};

class MemoryCoordinatorTestThread : public base::Thread,
                                    public base::MemoryCoordinatorClient {
 public:
  MemoryCoordinatorTestThread(
      const std::string& name)
      : Thread(name) {}
  ~MemoryCoordinatorTestThread() override { Stop(); }

  void Init() override {
    base::MemoryCoordinatorClientRegistry::GetInstance()->Register(this);
  }

  void OnMemoryStateChange(base::MemoryState state) override {
    EXPECT_EQ(message_loop(), base::MessageLoop::current());
    last_state_ = state;
  }

  void CheckLastState(base::MemoryState state) {
    task_runner()->PostTask(
        FROM_HERE,
        base::BindOnce(&MemoryCoordinatorTestThread::CheckLastStateInternal,
                       base::Unretained(this), state));
  }

 private:
  void CheckLastStateInternal(base::MemoryState state) {
    base::RunLoop loop;
    loop.RunUntilIdle();
    EXPECT_EQ(state, last_state_);
  }

  base::MemoryState last_state_ = base::MemoryState::UNKNOWN;
};

TEST_F(ChildMemoryCoordinatorImplTest, SingleClient) {
  auto* memory_coordinator_proxy = base::MemoryCoordinatorProxy::GetInstance();
  MockMemoryCoordinatorClient client;
  RegisterClient(&client);

  ChangeState(mojom::MemoryState::THROTTLED);
  EXPECT_EQ(base::MemoryState::THROTTLED, client.last_state());
  EXPECT_EQ(base::MemoryState::THROTTLED,
            memory_coordinator_proxy->GetCurrentMemoryState());

  ChangeState(mojom::MemoryState::NORMAL);
  EXPECT_EQ(base::MemoryState::NORMAL, client.last_state());
  EXPECT_EQ(base::MemoryState::NORMAL,
            memory_coordinator_proxy->GetCurrentMemoryState());

  UnregisterClient(&client);
  ChangeState(mojom::MemoryState::THROTTLED);
  EXPECT_TRUE(base::MemoryState::THROTTLED != client.last_state());
  EXPECT_EQ(base::MemoryState::THROTTLED,
            memory_coordinator_proxy->GetCurrentMemoryState());
}

TEST_F(ChildMemoryCoordinatorImplTest, MultipleClients) {
  MemoryCoordinatorTestThread t1("thread 1");
  MemoryCoordinatorTestThread t2("thread 2");

  t1.StartAndWaitForTesting();
  t2.StartAndWaitForTesting();

  ChangeState(mojom::MemoryState::THROTTLED);
  t1.CheckLastState(base::MemoryState::THROTTLED);
  t2.CheckLastState(base::MemoryState::THROTTLED);

  ChangeState(mojom::MemoryState::NORMAL);
  t1.CheckLastState(base::MemoryState::NORMAL);
  t2.CheckLastState(base::MemoryState::NORMAL);

  t1.Stop();
  t2.Stop();
}

#if defined(OS_ANDROID)
TEST_F(ChildMemoryCoordinatorImplTest, OnTrimMemoryImmediately) {
  // TRIM_MEMORY_COMPLETE defined in ComponentCallbacks2.
  static const int kTrimMemoryComplete = 80;

  ChildMemoryCoordinatorImplAndroid& coordinator_android =
      static_cast<ChildMemoryCoordinatorImplAndroid&>(coordinator_impl());
  coordinator_android.OnTrimMemory(kTrimMemoryComplete);
  EXPECT_EQ(true, on_trim_memory_called_);
}
#endif  // defined(OS_ANDROID)

}  // namespace

}  // namespace content
