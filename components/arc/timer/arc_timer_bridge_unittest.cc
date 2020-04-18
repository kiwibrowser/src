// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <map>
#include <memory>
#include <utility>
#include <vector>

#include "base/bind.h"
#include "base/callback_helpers.h"
#include "base/files/file_descriptor_watcher_posix.h"
#include "base/files/scoped_file.h"
#include "base/optional.h"
#include "base/posix/unix_domain_socket.h"
#include "base/run_loop.h"
#include "base/time/time.h"
#include "components/arc/arc_bridge_service.h"
#include "components/arc/arc_service_manager.h"
#include "components/arc/common/timer.mojom.h"
#include "components/arc/connection_holder.h"
#include "components/arc/test/connection_holder_util.h"
#include "components/arc/test/fake_timer_instance.h"
#include "components/arc/test/test_browser_context.h"
#include "components/arc/timer/arc_timer_bridge.h"
#include "components/arc/timer/arc_timer_traits.h"
#include "components/arc/timer/create_timer_request.h"
#include "components/keyed_service/content/browser_context_keyed_service_factory.h"
#include "content/public/test/test_browser_thread_bundle.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace arc {

namespace {

class ArcTimerTest : public testing::Test {
 public:
  ArcTimerTest()
      : thread_bundle_(content::TestBrowserThreadBundle::IO_MAINLOOP),
        timer_bridge_(
            ArcTimerBridge::GetForBrowserContextForTesting(&context_)) {
    // This results in ArcTimerBridge::OnInstanceReady being called.
    ArcServiceManager::Get()->arc_bridge_service()->timer()->SetInstance(
        &timer_instance_);
    WaitForInstanceReady(
        ArcServiceManager::Get()->arc_bridge_service()->timer());
  }

  ~ArcTimerTest() override {
    // Destroys the FakeTimerInstance. This results in
    // ArcTimerBridge::OnInstanceClosed being called.
    ArcServiceManager::Get()->arc_bridge_service()->timer()->CloseInstance(
        &timer_instance_);
    timer_bridge_->Shutdown();
  }

 protected:
  FakeTimerInstance* GetFakeTimerInstance() { return &timer_instance_; }

 private:
  content::TestBrowserThreadBundle thread_bundle_;
  ArcServiceManager arc_service_manager_;
  TestBrowserContext context_;
  FakeTimerInstance timer_instance_;

  ArcTimerBridge* const timer_bridge_;

  DISALLOW_COPY_AND_ASSIGN(ArcTimerTest);
};

// Stores timer proxies associated with each clock's timer.
class ArcTimerStore {
 public:
  ArcTimerStore() = default;

  bool AddTimer(clockid_t clock_id, base::ScopedFD read_fd) {
    ArcTimerStore::ArcTimerInfo arc_timer_info;
    arc_timer_info.read_fd = std::move(read_fd);
    auto emplace_result =
        arc_timers_.emplace(clock_id, std::move(arc_timer_info));
    if (!emplace_result.second)
      ADD_FAILURE() << "Failed to create timer entry";
    return emplace_result.second;
  }

  void ClearTimers() { return arc_timers_.clear(); }

  base::Optional<int> GetTimerReadFd(clockid_t clock_id) {
    if (!HasTimer(clock_id))
      return base::nullopt;
    return base::Optional<int>(arc_timers_[clock_id].read_fd.get());
  }

  mojom::TimerPtr* GetTimerProxy(clockid_t clock_id) {
    if (!HasTimer(clock_id))
      return nullptr;
    return &arc_timers_[clock_id].timer;
  }

  bool HasTimer(clockid_t clock_id) {
    auto it = arc_timers_.find(clock_id);
    return (it != arc_timers_.end() && it->second.timer);
  }

  bool SetTimerProxy(clockid_t clock_id, mojom::TimerPtrInfo timer) {
    // An entry for storing the timer proxy should already be present.
    auto it = arc_timers_.find(clock_id);
    if (it == arc_timers_.end())
      return false;
    it->second.timer = mojom::TimerPtr(std::move(timer));
    return true;
  }

  size_t size() const { return arc_timers_.size(); }

 private:
  struct ArcTimerInfo {
    // The mojom::Timer associated with an arc timer.
    mojom::TimerPtr timer;

    // The fd that will be signalled by the host when the timer expires.
    base::ScopedFD read_fd;
  };

  std::map<clockid_t, ArcTimerInfo> arc_timers_;

  DISALLOW_COPY_AND_ASSIGN(ArcTimerStore);
};

// Stores timer proxies returned by |mojom::TimerHost::CreateTimers|. Iff timers
// for all clocks are created successfully, then |arc_timer_store| will have
// proxies corresponding to each clock.
void CreateTimersCallback(
    ArcTimerStore* arc_timer_store,
    base::OnceClosure quit_callback,
    base::Optional<std::vector<mojom::CreateTimerResponsePtr>> result) {
  base::ScopedClosureRunner quit_runner(std::move(quit_callback));

  if (result == base::nullopt) {
    ADD_FAILURE() << "Null timer objects array returned";
    arc_timer_store->ClearTimers();
    return;
  }

  const size_t responses_size = result.value().size();
  if (responses_size != arc_timer_store->size()) {
    ADD_FAILURE() << "Incorrect number of timer objects returned: "
                  << responses_size;
    arc_timer_store->ClearTimers();
    return;
  }

  // Store the timer objects. These will be retrieved to set timers.
  for (auto& response : result.value()) {
    int32_t clock_id;
    if (!mojo::EnumTraits<arc::mojom::ClockId, int32_t>::FromMojom(
            response->clock_id, &clock_id)) {
      ADD_FAILURE() << "Failed to convert mojo clock id: "
                    << response->clock_id;
      arc_timer_store->ClearTimers();
      return;
    }
    EXPECT_TRUE(
        arc_timer_store->SetTimerProxy(clock_id, std::move(response->timer)));
  }
}

// Returns true iff timer creation of each clock type succeeded.
bool CreateTimers(const std::vector<clockid_t>& clocks,
                  ArcTimerStore* arc_timer_store,
                  FakeTimerInstance* timer_instance) {
  // Create requests to create a timer for each clock.
  std::vector<CreateTimerRequest> arc_timer_requests;
  for (const clockid_t& clock : clocks) {
    CreateTimerRequest request;
    request.clock_id = clock;
    // Create a socket pair for each clock. One socket will be part of the mojo
    // argument and will be used by the host to indicate when the timer expires.
    // The other socket will be used to detect the expiration of the timer by
    // epolling and reading.
    base::ScopedFD read_fd;
    base::ScopedFD write_fd;
    if (!base::CreateSocketPair(&read_fd, &write_fd)) {
      ADD_FAILURE() << "Failed to create socket pair for ARC timers";
      return false;
    }
    request.expiration_fd = std::move(write_fd);
    // Create an entry for each clock in the store. The timer object will be
    // populated in the callback to the call to create timers.
    if (!arc_timer_store->AddTimer(clock, std::move(read_fd))) {
      ADD_FAILURE() << "Failed to create timer entry";
      arc_timer_store->ClearTimers();
      return false;
    }
    arc_timer_requests.emplace_back(std::move(request));
  }
  // Call the host to create timers.
  base::RunLoop loop;
  timer_instance->CallCreateTimers(
      std::move(arc_timer_requests),
      base::BindOnce(&CreateTimersCallback, arc_timer_store,
                     loop.QuitClosure()));
  loop.Run();
  // Check if each clock's timer is created successfully.
  for (const clockid_t clock : clocks) {
    if (!arc_timer_store->HasTimer(clock)) {
      ADD_FAILURE() << "Failed to create timer for clock:" << clock;
      return false;
    }
  }
  return true;
}

// Returns true iff the read descriptor of a timer is signalled. If the
// signalling is incorrect returns false. Blocks otherwise.
bool WaitForExpiration(clockid_t clock_id, ArcTimerStore* arc_timer_store) {
  if (!arc_timer_store->HasTimer(clock_id)) {
    ADD_FAILURE() << "Timer of type: " << clock_id << " not present";
    return false;
  }

  // Wait for the host to indicate expiration by watching the read end of the
  // socket pair.
  base::Optional<int> timer_read_fd_opt =
      arc_timer_store->GetTimerReadFd(clock_id);
  EXPECT_NE(timer_read_fd_opt, base::nullopt);
  int timer_read_fd = timer_read_fd_opt.value();
  base::RunLoop loop;
  std::unique_ptr<base::FileDescriptorWatcher::Controller>
      watch_readable_controller = base::FileDescriptorWatcher::WatchReadable(
          timer_read_fd, loop.QuitClosure());
  loop.Run();

  // The timer expects 8 bytes to be written from the host upon expiration.
  uint64_t timer_data;
  std::vector<base::ScopedFD> fds;
  ssize_t bytes_read = base::UnixDomainSocket::RecvMsg(
      timer_read_fd, &timer_data, sizeof(timer_data), &fds);
  if (bytes_read < static_cast<ssize_t>(sizeof(timer_data))) {
    ADD_FAILURE() << "Incorrect timer wake up bytes_read: " << bytes_read;
    return false;
  }
  LOG(INFO) << "Actual expiration time: " << base::Time::Now();
  return true;
}

TEST_F(ArcTimerTest, StartTimer) {
  std::vector<clockid_t> clocks = {CLOCK_REALTIME_ALARM, CLOCK_BOOTTIME_ALARM};
  ArcTimerStore arc_timer_store;
  // Create timers before starting it.
  EXPECT_TRUE(CreateTimers(clocks, &arc_timer_store, GetFakeTimerInstance()));
  mojom::TimerPtr* timer = arc_timer_store.GetTimerProxy(CLOCK_BOOTTIME_ALARM);
  ASSERT_TRUE(timer);
  ASSERT_TRUE(*timer);
  // Start timer and check if timer expired.
  base::TimeDelta delay = base::TimeDelta::FromMilliseconds(20);
  base::RunLoop loop;
  LOG(INFO) << "Start time: " << base::Time::Now()
            << " Expiration time: " << base::Time::Now() + delay;
  (*timer)->Start(base::TimeTicks::Now() + delay,
                  base::BindOnce(
                      [](base::RunLoop* loop, mojom::ArcTimerResult result) {
                        if (result != mojom::ArcTimerResult::SUCCESS)
                          ADD_FAILURE() << "Start timer failed";
                        loop->Quit();
                      },
                      &loop));
  loop.Run();
  EXPECT_TRUE(WaitForExpiration(CLOCK_BOOTTIME_ALARM, &arc_timer_store));
}

}  // namespace

}  // namespace arc
