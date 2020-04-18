// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "extensions/browser/load_monitoring_extension_host_queue.h"

#include <stddef.h>

#include <limits>
#include <vector>

#include "base/bind.h"
#include "base/run_loop.h"
#include "extensions/browser/deferred_start_render_host.h"
#include "extensions/browser/extensions_test.h"
#include "extensions/browser/serial_extension_host_queue.h"

namespace extensions {

namespace {

class StubDeferredStartRenderHost : public DeferredStartRenderHost {
 public:
  // Returns true if this host is being observed by |observer|.
  bool IsObservedBy(DeferredStartRenderHostObserver* observer) {
    return observers_.count(observer) > 0;
  }

 private:
  // DeferredStartRenderHost:
  void AddDeferredStartRenderHostObserver(
      DeferredStartRenderHostObserver* observer) override {
    observers_.insert(observer);
  }
  void RemoveDeferredStartRenderHostObserver(
      DeferredStartRenderHostObserver* observer) override {
    observers_.erase(observer);
  }
  void CreateRenderViewNow() override {}

  std::set<DeferredStartRenderHostObserver*> observers_;
};

const size_t g_invalid_size_t = std::numeric_limits<size_t>::max();

}  // namespace

class LoadMonitoringExtensionHostQueueTest : public ExtensionsTest {
 public:
  LoadMonitoringExtensionHostQueueTest()
      : finished_(false),
        // Arbitrary choice of an invalid size_t.
        num_queued_(g_invalid_size_t),
        num_loaded_(g_invalid_size_t),
        max_awaiting_loading_(g_invalid_size_t),
        max_active_loading_(g_invalid_size_t) {}

  void SetUp() override {
    queue_.reset(new LoadMonitoringExtensionHostQueue(
        // Use a SerialExtensionHostQueue because it's simple.
        std::unique_ptr<ExtensionHostQueue>(new SerialExtensionHostQueue()),
        base::TimeDelta(),  // no delay, easier to test
        base::Bind(&LoadMonitoringExtensionHostQueueTest::Finished,
                   base::Unretained(this))));
  }

 protected:
  // Creates a new DeferredStartRenderHost. Ownership is held by this class,
  // not passed to caller.
  StubDeferredStartRenderHost* CreateHost() {
    stubs_.push_back(std::make_unique<StubDeferredStartRenderHost>());
    return stubs_.back().get();
  }

  // Our single LoadMonitoringExtensionHostQueue instance.
  LoadMonitoringExtensionHostQueue* queue() { return queue_.get(); }

  // Returns true if the queue has finished monitoring.
  bool finished() const { return finished_; }

  // These are available after the queue has finished (in which case finished()
  // will return true).
  size_t num_queued() { return num_queued_; }
  size_t num_loaded() { return num_loaded_; }
  size_t max_awaiting_loading() { return max_awaiting_loading_; }
  size_t max_active_loading() { return max_active_loading_; }

 private:
  // Callback when queue has finished monitoring.
  void Finished(size_t num_queued,
                size_t num_loaded,
                size_t max_awaiting_loading,
                size_t max_active_loading) {
    CHECK(!finished_);
    finished_ = true;
    num_queued_ = num_queued;
    num_loaded_ = num_loaded;
    max_awaiting_loading_ = max_awaiting_loading;
    max_active_loading_ = max_active_loading;
  }

  std::unique_ptr<LoadMonitoringExtensionHostQueue> queue_;
  std::vector<std::unique_ptr<StubDeferredStartRenderHost>> stubs_;

  // Set after the queue has finished monitoring.
  bool finished_;
  size_t num_queued_;
  size_t num_loaded_;
  size_t max_awaiting_loading_;
  size_t max_active_loading_;
};

// Tests that if monitoring is never started, nor any hosts added, nothing is
// recorded.
TEST_F(LoadMonitoringExtensionHostQueueTest, NeverStarted) {
  base::RunLoop().RunUntilIdle();
  ASSERT_FALSE(finished());
}

// Tests that if monitoring has started but no hosts added, it's recorded as 0.
TEST_F(LoadMonitoringExtensionHostQueueTest, NoHosts) {
  queue()->StartMonitoring();

  base::RunLoop().RunUntilIdle();
  ASSERT_TRUE(finished());
  EXPECT_EQ(0u, num_queued());
  EXPECT_EQ(0u, num_loaded());
  EXPECT_EQ(0u, max_awaiting_loading());
  EXPECT_EQ(0u, max_active_loading());
}

// Tests that adding a host starts monitoring.
TEST_F(LoadMonitoringExtensionHostQueueTest, AddOneHost) {
  queue()->Add(CreateHost());

  base::RunLoop().RunUntilIdle();
  ASSERT_TRUE(finished());
  EXPECT_EQ(1u, num_queued());
  EXPECT_EQ(0u, num_loaded());
  EXPECT_EQ(1u, max_awaiting_loading());
  EXPECT_EQ(0u, max_active_loading());
}

// Tests that a host added and removed is still recorded, but not as a load
// finished.
TEST_F(LoadMonitoringExtensionHostQueueTest, AddAndRemoveOneHost) {
  DeferredStartRenderHost* host = CreateHost();
  queue()->Add(host);
  queue()->Remove(host);

  base::RunLoop().RunUntilIdle();
  ASSERT_TRUE(finished());
  EXPECT_EQ(1u, num_queued());
  EXPECT_EQ(0u, num_loaded());
  EXPECT_EQ(1u, max_awaiting_loading());
  EXPECT_EQ(0u, max_active_loading());
}

// Tests adding and starting a single host.
TEST_F(LoadMonitoringExtensionHostQueueTest, AddAndStartOneHost) {
  DeferredStartRenderHost* host = CreateHost();
  queue()->Add(host);
  queue()->OnDeferredStartRenderHostDidStartFirstLoad(host);

  base::RunLoop().RunUntilIdle();
  ASSERT_TRUE(finished());
  EXPECT_EQ(1u, num_queued());
  EXPECT_EQ(0u, num_loaded());
  EXPECT_EQ(1u, max_awaiting_loading());
  EXPECT_EQ(1u, max_active_loading());
}

// Tests adding and destroying a single host without starting it.
TEST_F(LoadMonitoringExtensionHostQueueTest, AddAndDestroyOneHost) {
  DeferredStartRenderHost* host = CreateHost();
  queue()->Add(host);
  queue()->OnDeferredStartRenderHostDestroyed(host);

  base::RunLoop().RunUntilIdle();
  ASSERT_TRUE(finished());
  EXPECT_EQ(1u, num_queued());
  EXPECT_EQ(0u, num_loaded());
  EXPECT_EQ(1u, max_awaiting_loading());
  EXPECT_EQ(0u, max_active_loading());
}

// Tests adding, starting, and stopping a single host.
TEST_F(LoadMonitoringExtensionHostQueueTest, AddAndStartAndStopOneHost) {
  DeferredStartRenderHost* host = CreateHost();
  queue()->Add(host);
  queue()->OnDeferredStartRenderHostDidStartFirstLoad(host);
  queue()->OnDeferredStartRenderHostDidStopFirstLoad(host);

  base::RunLoop().RunUntilIdle();
  ASSERT_TRUE(finished());
  EXPECT_EQ(1u, num_queued());
  EXPECT_EQ(1u, num_loaded());
  EXPECT_EQ(1u, max_awaiting_loading());
  EXPECT_EQ(1u, max_active_loading());
}

// Tests adding, starting, and stopping a single host - twice.
TEST_F(LoadMonitoringExtensionHostQueueTest, AddAndStartAndStopOneHostTwice) {
  DeferredStartRenderHost* host = CreateHost();
  queue()->Add(host);
  queue()->OnDeferredStartRenderHostDidStartFirstLoad(host);
  queue()->OnDeferredStartRenderHostDidStopFirstLoad(host);

  // Re-starting loading should also be recorded fine (e.g. navigations could
  // trigger this).
  queue()->OnDeferredStartRenderHostDidStartFirstLoad(host);
  queue()->OnDeferredStartRenderHostDidStopFirstLoad(host);

  base::RunLoop().RunUntilIdle();
  ASSERT_TRUE(finished());
  EXPECT_EQ(1u, num_queued());
  EXPECT_EQ(2u, num_loaded());  // 2 loaded this time, because we ran twice
  EXPECT_EQ(1u, max_awaiting_loading());
  EXPECT_EQ(1u, max_active_loading());
}

// Tests adding, starting, and destroying (i.e. an implicit stop) a single host.
TEST_F(LoadMonitoringExtensionHostQueueTest, AddAndStartAndDestroyOneHost) {
  DeferredStartRenderHost* host = CreateHost();
  queue()->Add(host);
  queue()->OnDeferredStartRenderHostDidStartFirstLoad(host);
  queue()->OnDeferredStartRenderHostDestroyed(host);

  base::RunLoop().RunUntilIdle();
  ASSERT_TRUE(finished());
  EXPECT_EQ(1u, num_queued());
  EXPECT_EQ(1u, num_loaded());
  EXPECT_EQ(1u, max_awaiting_loading());
  EXPECT_EQ(1u, max_active_loading());
}

// Tests adding, starting, and removing a single host.
TEST_F(LoadMonitoringExtensionHostQueueTest, AddAndStartAndRemoveOneHost) {
  DeferredStartRenderHost* host = CreateHost();
  queue()->Add(host);
  queue()->OnDeferredStartRenderHostDidStartFirstLoad(host);
  queue()->Remove(host);

  base::RunLoop().RunUntilIdle();
  ASSERT_TRUE(finished());
  EXPECT_EQ(1u, num_queued());
  EXPECT_EQ(0u, num_loaded());
  EXPECT_EQ(1u, max_awaiting_loading());
  EXPECT_EQ(1u, max_active_loading());
}

// Tests monitoring a sequence of hosts.
TEST_F(LoadMonitoringExtensionHostQueueTest, Sequence) {
  // Scenario:
  //
  // 6 hosts will be added, only 5 will start loading, with a maximum of 4 in
  // the queue and 3 loading at any time. Only 2 will finish.
  DeferredStartRenderHost* host1 = CreateHost();
  DeferredStartRenderHost* host2 = CreateHost();
  DeferredStartRenderHost* host3 = CreateHost();
  DeferredStartRenderHost* host4 = CreateHost();
  DeferredStartRenderHost* host5 = CreateHost();
  DeferredStartRenderHost* host6 = CreateHost();

  queue()->Add(host1);
  queue()->Add(host2);
  queue()->Add(host3);

  queue()->OnDeferredStartRenderHostDidStartFirstLoad(host1);
  queue()->OnDeferredStartRenderHostDidStartFirstLoad(host2);
  queue()->OnDeferredStartRenderHostDidStopFirstLoad(host1);

  queue()->Add(host4);
  queue()->Add(host5);
  queue()->Add(host6);

  queue()->OnDeferredStartRenderHostDidStartFirstLoad(host3);
  queue()->OnDeferredStartRenderHostDidStartFirstLoad(host4);
  queue()->OnDeferredStartRenderHostDidStopFirstLoad(host4);
  queue()->OnDeferredStartRenderHostDidStartFirstLoad(host5);

  base::RunLoop().RunUntilIdle();
  ASSERT_TRUE(finished());
  EXPECT_EQ(6u, num_queued());
  EXPECT_EQ(2u, num_loaded());
  EXPECT_EQ(4u, max_awaiting_loading());
  EXPECT_EQ(3u, max_active_loading());

  // Complete a realistic sequence by stopping and/or destroying all hosts.
  queue()->OnDeferredStartRenderHostDestroyed(host1);
  queue()->OnDeferredStartRenderHostDidStopFirstLoad(host2);
  queue()->OnDeferredStartRenderHostDestroyed(host2);
  queue()->OnDeferredStartRenderHostDidStopFirstLoad(host3);
  queue()->OnDeferredStartRenderHostDestroyed(host3);
  queue()->OnDeferredStartRenderHostDestroyed(host4);
  queue()->OnDeferredStartRenderHostDestroyed(host5);  // never stopped
  queue()->OnDeferredStartRenderHostDestroyed(host6);  // never started/stopped
}

// Tests that the queue is observing Hosts from adding them through to being
// removed - that the load sequence itself is irrelevant.
//
// This is an unfortunate implementation-style test, but it used to be a bug
// and difficult to catch outside of a proper test framework - one in which we
// don't have to trigger events by hand.
TEST_F(LoadMonitoringExtensionHostQueueTest, ObserverLifetime) {
  StubDeferredStartRenderHost* host1 = CreateHost();
  StubDeferredStartRenderHost* host2 = CreateHost();
  StubDeferredStartRenderHost* host3 = CreateHost();
  StubDeferredStartRenderHost* host4 = CreateHost();

  EXPECT_FALSE(host1->IsObservedBy(queue()));
  EXPECT_FALSE(host2->IsObservedBy(queue()));
  EXPECT_FALSE(host3->IsObservedBy(queue()));
  EXPECT_FALSE(host4->IsObservedBy(queue()));

  queue()->Add(host1);
  queue()->Add(host2);
  queue()->Add(host3);
  queue()->Add(host4);

  EXPECT_TRUE(host1->IsObservedBy(queue()));
  EXPECT_TRUE(host2->IsObservedBy(queue()));
  EXPECT_TRUE(host3->IsObservedBy(queue()));
  EXPECT_TRUE(host4->IsObservedBy(queue()));

  queue()->OnDeferredStartRenderHostDidStartFirstLoad(host1);
  queue()->OnDeferredStartRenderHostDidStartFirstLoad(host2);
  queue()->OnDeferredStartRenderHostDidStartFirstLoad(host3);
  // host4 will test that we Remove before Starting - so don't start.

  EXPECT_TRUE(host1->IsObservedBy(queue()));
  EXPECT_TRUE(host2->IsObservedBy(queue()));
  EXPECT_TRUE(host3->IsObservedBy(queue()));
  EXPECT_TRUE(host4->IsObservedBy(queue()));

  queue()->OnDeferredStartRenderHostDidStopFirstLoad(host1);
  queue()->OnDeferredStartRenderHostDestroyed(host2);

  EXPECT_TRUE(host1->IsObservedBy(queue()));
  EXPECT_TRUE(host2->IsObservedBy(queue()));

  queue()->Remove(host1);
  queue()->Remove(host2);
  queue()->Remove(host3);
  queue()->Remove(host4);

  EXPECT_FALSE(host1->IsObservedBy(queue()));
  EXPECT_FALSE(host2->IsObservedBy(queue()));
  EXPECT_FALSE(host3->IsObservedBy(queue()));
  EXPECT_FALSE(host4->IsObservedBy(queue()));
}

}  // namespace extensions
