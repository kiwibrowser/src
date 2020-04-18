// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <utility>

#include "base/containers/flat_set.h"
#include "base/logging.h"
#include "base/memory/singleton.h"
#include "base/stl_util.h"
#include "base/task_runner_util.h"
#include "base/threading/sequenced_task_runner_handle.h"
#include "components/arc/arc_bridge_service.h"
#include "components/arc/arc_browser_context_keyed_service_factory_base.h"
#include "components/arc/arc_service_manager.h"
#include "components/arc/timer/arc_timer.h"
#include "components/arc/timer/arc_timer_bridge.h"
#include "components/arc/timer/arc_timer_traits.h"

namespace arc {

namespace {

// Singleton factory for ArcTimerBridge.
class ArcTimerBridgeFactory
    : public internal::ArcBrowserContextKeyedServiceFactoryBase<
          ArcTimerBridge,
          ArcTimerBridgeFactory> {
 public:
  // Factory name used by ArcBrowserContextKeyedServiceFactoryBase.
  static constexpr const char* kName = "ArcTimerBridgeFactory";

  static ArcTimerBridgeFactory* GetInstance() {
    return base::Singleton<ArcTimerBridgeFactory>::get();
  }

 private:
  friend base::DefaultSingletonTraits<ArcTimerBridgeFactory>;
  ArcTimerBridgeFactory() = default;
  ~ArcTimerBridgeFactory() override = default;
};

bool IsSupportedClock(int32_t clock_id) {
  return clock_id == CLOCK_BOOTTIME_ALARM || clock_id == CLOCK_REALTIME_ALARM;
}

}  // namespace

// static
BrowserContextKeyedServiceFactory* ArcTimerBridge::GetFactory() {
  return ArcTimerBridgeFactory::GetInstance();
}

// static
ArcTimerBridge* ArcTimerBridge::GetForBrowserContext(
    content::BrowserContext* context) {
  return ArcTimerBridgeFactory::GetForBrowserContext(context);
}

// static
ArcTimerBridge* ArcTimerBridge::GetForBrowserContextForTesting(
    content::BrowserContext* context) {
  return ArcTimerBridgeFactory::GetForBrowserContextForTesting(context);
}

ArcTimerBridge::ArcTimerBridge(content::BrowserContext* context,
                               ArcBridgeService* bridge_service)
    : timer_task_runner_(
          base::CreateSequencedTaskRunnerWithTraits({base::MayBlock()})),
      arc_bridge_service_(bridge_service),
      binding_(this),
      weak_ptr_factory_(this) {
  arc_bridge_service_->timer()->SetHost(this);
  arc_bridge_service_->timer()->AddObserver(this);
}

ArcTimerBridge::~ArcTimerBridge() {
  arc_bridge_service_->timer()->RemoveObserver(this);
  arc_bridge_service_->timer()->SetHost(nullptr);
}

void ArcTimerBridge::CreateTimers(
    std::vector<CreateTimerRequest> arc_timer_requests,
    CreateTimersCallback callback) {
  DVLOG(1) << "Received CreateTimers";
  // Alarm timers can't be created on the UI thread because they make syscalls
  // in the constructor. Post a task to create |ArcTimer| objects containing
  // alarm timer objects.
  base::PostTaskAndReplyWithResult(
      timer_task_runner_.get(), FROM_HERE,
      base::BindOnce(&CreateArcTimers, !arc_timers_.empty(),
                     base::SequencedTaskRunnerHandle::Get(), timer_task_runner_,
                     std::move(arc_timer_requests),
                     weak_ptr_factory_.GetWeakPtr()),
      base::BindOnce(&ArcTimerBridge::OnArcTimersCreated,
                     weak_ptr_factory_.GetWeakPtr(), std::move(callback)));
}

void ArcTimerBridge::OnConnectionClosed() {
  DVLOG(1) << "OnConnectionClosed";
  DeleteArcTimers();
}

// Deleter for |ArcTimer|s. Deletes the timer object on |task_runner|.
class ArcTimerBridge::DeleteOnSequence {
 public:
  explicit DeleteOnSequence(
      scoped_refptr<base::SequencedTaskRunner> task_runner)
      : task_runner_(std::move(task_runner)) {}
  ~DeleteOnSequence() = default;
  DeleteOnSequence(DeleteOnSequence&&) = default;
  DeleteOnSequence& operator=(DeleteOnSequence&&) = default;

  void operator()(ArcTimer* timer) const {
    task_runner_->DeleteSoon(FROM_HERE, timer);
  }

 private:
  scoped_refptr<base::SequencedTaskRunner> task_runner_;

  DISALLOW_COPY_AND_ASSIGN(DeleteOnSequence);
};

void ArcTimerBridge::OnConnectionError(ArcTimer* timer) {
  DVLOG(1) << "OnConnectionError";
  // At this point the mojo binding for |timer| has an error. Find and delete
  // the timer. Since the lifetime of |ArcTimer| objects is managed by this
  // class it is safe to compare |timer|.
  base::EraseIf(
      arc_timers_,
      [timer](const std::unique_ptr<ArcTimer, DeleteOnSequence>& element) {
        return element.get() == timer;
      });
}

// static.
void ArcTimerBridge::OnConnectionErrorOnTimerThread(
    scoped_refptr<base::SequencedTaskRunner> task_runner,
    base::WeakPtr<ArcTimerBridge> weak_self,
    ArcTimer* timer) {
  // Post task on the main thread since the task will access class members.
  task_runner->PostTask(
      FROM_HERE,
      base::BindOnce(&ArcTimerBridge::OnConnectionError, weak_self, timer));
}

struct ArcTimerBridge::TimersAndProxies {
  TimersAndProxies() = default;
  ~TimersAndProxies() = default;
  TimersAndProxies(TimersAndProxies&&) = default;
  TimersAndProxies& operator=(TimersAndProxies&&) = default;
  std::vector<int32_t> clocks;
  std::vector<std::unique_ptr<ArcTimer, DeleteOnSequence>> timers;
  std::vector<mojom::TimerPtrInfo> proxies;
};

// static.
base::Optional<ArcTimerBridge::TimersAndProxies>
ArcTimerBridge::CreateArcTimers(
    bool timers_already_created,
    scoped_refptr<base::SequencedTaskRunner> original_task_runner,
    scoped_refptr<base::SequencedTaskRunner> timer_task_runner,
    std::vector<arc::CreateTimerRequest> arc_timer_requests,
    base::WeakPtr<ArcTimerBridge> weak_self) {
  if (timers_already_created) {
    LOG(ERROR) << "Double creation not supported";
    return base::nullopt;
  }

  // Iterate over the list of {clock_id, expiration_fd} and create an |ArcTimer|
  // and |mojom::TimerPtrInfo| entry for each clock.
  base::flat_set<int32_t> seen_clocks;
  ArcTimerBridge::TimersAndProxies result;
  for (auto& request : arc_timer_requests) {
    // Read each entry one by one. Each entry will have an |ArcTimer| entry
    // associated with it.
    int32_t clock_id = request.clock_id;

    if (!IsSupportedClock(clock_id)) {
      LOG(ERROR) << "Unsupported clock=" << clock_id;
      return base::nullopt;
    }

    if (!seen_clocks.insert(clock_id).second) {
      LOG(ERROR) << "Duplicate clocks not supported";
      return base::nullopt;
    }

    base::ScopedFD expiration_fd = std::move(request.expiration_fd);
    if (!expiration_fd.is_valid()) {
      LOG(ERROR) << "Bad file descriptor for clock=" << clock_id;
      return base::nullopt;
    }

    mojom::TimerPtrInfo timer_proxy_info;
    // TODO(b/69759087): Make |ArcTimer| take |clock_id| to create timers of
    // different clock types.
    // The instance opens clocks of type CLOCK_BOOTTIME_ALARM and
    // CLOCK_REALTIME_ALARM. However, it uses only CLOCK_BOOTTIME_ALARM to set
    // wake up alarms. At this point, it's okay to pretend the host supports
    // CLOCK_REALTIME_ALARM instead of returning an error.
    //
    // Mojo guarantees to call all callbacks on the task runner that the
    // mojo::Binding i.e. |ArcTimer| was created on.
    result.clocks.push_back(clock_id);
    result.timers.push_back(std::unique_ptr<ArcTimer, DeleteOnSequence>(
        new ArcTimer(
            std::move(expiration_fd), mojo::MakeRequest(&timer_proxy_info),
            base::BindOnce(&ArcTimerBridge::OnConnectionErrorOnTimerThread,
                           original_task_runner, weak_self)),
        DeleteOnSequence(timer_task_runner)));
    result.proxies.push_back(std::move(timer_proxy_info));
  }
  return result;
}

void ArcTimerBridge::OnArcTimersCreated(
    CreateTimersCallback callback,
    base::Optional<TimersAndProxies> timers_and_proxies) {
  if (timers_and_proxies == base::nullopt) {
    std::move(callback).Run(base::nullopt);
    return;
  }
  DCHECK_EQ(timers_and_proxies->clocks.size(),
            timers_and_proxies->timers.size());
  DCHECK_EQ(timers_and_proxies->clocks.size(),
            timers_and_proxies->proxies.size());
  arc_timers_ = std::move(timers_and_proxies->timers);

  // Respond to instance with timer proxies.
  std::vector<mojom::CreateTimerResponsePtr> result;
  for (size_t i = 0; i < timers_and_proxies->clocks.size(); i++) {
    mojom::CreateTimerResponsePtr response = mojom::CreateTimerResponse::New();
    response->clock_id =
        mojo::EnumTraits<arc::mojom::ClockId, int32_t>::ToMojom(
            timers_and_proxies->clocks[i]);
    response->timer = std::move(timers_and_proxies->proxies[i]);
    result.push_back(std::move(response));
  }
  std::move(callback).Run(std::move(result));
}

void ArcTimerBridge::DeleteArcTimers() {
  // The timer objects are deleted on |timer_task_runner_|.
  arc_timers_.clear();
}

}  // namespace arc
