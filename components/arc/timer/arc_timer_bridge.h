// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_ARC_TIMER_ARC_TIMER_BRIDGE_H_
#define COMPONENTS_ARC_TIMER_ARC_TIMER_BRIDGE_H_

#include <memory>
#include <vector>

#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "base/optional.h"
#include "base/sequenced_task_runner.h"
#include "components/arc/common/timer.mojom.h"
#include "components/arc/connection_observer.h"
#include "components/arc/timer/arc_timer.h"
#include "components/arc/timer/create_timer_request.h"
#include "components/keyed_service/core/keyed_service.h"
#include "mojo/public/cpp/bindings/binding.h"

class BrowserContextKeyedServiceFactory;

namespace content {
class BrowserContext;
}  // namespace content

namespace arc {

class ArcBridgeService;

// Sets wake up timers / alarms based on calls from the instance.
class ArcTimerBridge : public KeyedService,
                       public ConnectionObserver<mojom::TimerInstance>,
                       public mojom::TimerHost {
 public:
  // Returns the factory instance for this class.
  static BrowserContextKeyedServiceFactory* GetFactory();

  // Returns singleton instance for the given BrowserContext,
  // or nullptr if the browser |context| is not allowed to use ARC.
  static ArcTimerBridge* GetForBrowserContext(content::BrowserContext* context);

  static ArcTimerBridge* GetForBrowserContextForTesting(
      content::BrowserContext* context);

  ArcTimerBridge(content::BrowserContext* context,
                 ArcBridgeService* bridge_service);
  ~ArcTimerBridge() override;

  // ConnectionObserver<mojom::TimerInstance>::Observer overrides.
  void OnConnectionClosed() override;

  // mojom::TimerHost overrides.
  void CreateTimers(std::vector<CreateTimerRequest> arc_timer_requests,
                    CreateTimersCallback callback) override;

 private:
  class DeleteOnSequence;
  struct TimersAndProxies;

  // Deletes |timer| from |arc_timers_|.
  void OnConnectionError(ArcTimer* timer);

  // Callback invoked when an |ArcTimer| object has a connection error on it's
  // mojo binding. It schedules |OnConnectionErrror| on |task_runner| which
  // finally deletes |timer|. Since, |ArcTimerBridge| lives on the main thread
  // and this function runs on |timer_task_runner_| this function needs to be
  // static to be thread safe.
  static void OnConnectionErrorOnTimerThread(
      scoped_refptr<base::SequencedTaskRunner> task_runner,
      base::WeakPtr<ArcTimerBridge> weak_self,
      ArcTimer* timer);

  // Creates timers with the given arguments. Returns base::nullopt on failure.
  // On success, returns a non-empty vector of |TimersAndProxies| objects. Runs
  // on |timer_task_runner| and should only access thread-safe members of the
  // parent class. For this reason it is also a static member.
  static base::Optional<TimersAndProxies> CreateArcTimers(
      bool timers_already_created,
      scoped_refptr<base::SequencedTaskRunner> original_task_runner,
      scoped_refptr<base::SequencedTaskRunner> timer_task_runner,
      std::vector<arc::CreateTimerRequest> arc_timer_requests,
      base::WeakPtr<ArcTimerBridge> weak_self);

  // Callback for |CreateArcTimers|. Runs |callback| before exiting. Runs on the
  // main thread.
  void OnArcTimersCreated(CreateTimersCallback callback,
                          base::Optional<TimersAndProxies> timers_and_proxies);

  // Deletes all timers.
  void DeleteArcTimers();

  // Task runner on which all |ArcTimer| related operations are scheduled.
  scoped_refptr<base::SequencedTaskRunner> timer_task_runner_;

  ArcBridgeService* const arc_bridge_service_;  // Owned by ArcServiceManager.

  // Store of |ArcTimer| objects.
  std::vector<std::unique_ptr<ArcTimer, DeleteOnSequence>> arc_timers_;

  mojo::Binding<mojom::TimerHost> binding_;

  base::WeakPtrFactory<ArcTimerBridge> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(ArcTimerBridge);
};

}  // namespace arc

#endif  // COMPONENTS_ARC_TIMER_ARC_TIMER_BRIDGE_H_
