// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_RESOURCE_COORDINATOR_COORDINATION_UNIT_PAGE_SIGNAL_GENERATOR_IMPL_H_
#define SERVICES_RESOURCE_COORDINATOR_COORDINATION_UNIT_PAGE_SIGNAL_GENERATOR_IMPL_H_

#include <map>

#include "base/gtest_prod_util.h"
#include "base/macros.h"
#include "base/timer/timer.h"
#include "mojo/public/cpp/bindings/binding_set.h"
#include "mojo/public/cpp/bindings/interface_ptr_set.h"
#include "services/resource_coordinator/observers/coordination_unit_graph_observer.h"
#include "services/resource_coordinator/public/mojom/page_signal.mojom.h"

namespace service_manager {
struct BindSourceInfo;
}  // namespace service_manager

namespace resource_coordinator {

// The PageSignalGenerator is a dedicated |CoordinationUnitGraphObserver| for
// calculating and emitting page-scoped signals. This observer observes
// PageCoordinationUnits, ProcessCoordinationUnits and FrameCoordinationUnits,
// combining information from the graph to generate page level signals.
class PageSignalGeneratorImpl : public CoordinationUnitGraphObserver,
                                public mojom::PageSignalGenerator {
 public:
  // The amount of time a page has to be idle post-loading in order for it to be
  // considered loaded and idle. This is used in UpdateLoadIdleState
  // transitions.
  static const base::TimeDelta kLoadedAndIdlingTimeout;

  // The maximum amount of time post-DidStopLoading a page can be waiting for
  // an idle state to occur before the page is simply considered loaded anyways.
  // Since PageAlmostIdle is intended as an "initial loading complete" signal,
  // it needs to eventually terminate. This is strictly greater than the
  // kLoadedAndIdlingTimeout.
  static const base::TimeDelta kWaitingForIdleTimeout;

  PageSignalGeneratorImpl();
  ~PageSignalGeneratorImpl() override;

  // mojom::PageSignalGenerator implementation.
  void AddReceiver(mojom::PageSignalReceiverPtr receiver) override;

  // CoordinationUnitGraphObserver implementation.
  bool ShouldObserve(const CoordinationUnitBase* coordination_unit) override;
  void OnCoordinationUnitCreated(const CoordinationUnitBase* cu) override;
  void OnBeforeCoordinationUnitDestroyed(
      const CoordinationUnitBase* cu) override;
  void OnFramePropertyChanged(const FrameCoordinationUnitImpl* frame_cu,
                              const mojom::PropertyType property_type,
                              int64_t value) override;
  void OnPagePropertyChanged(const PageCoordinationUnitImpl* page_cu,
                             const mojom::PropertyType property_type,
                             int64_t value) override;
  void OnProcessPropertyChanged(const ProcessCoordinationUnitImpl* process_cu,
                                const mojom::PropertyType property_type,
                                int64_t value) override;
  void OnPageEventReceived(const PageCoordinationUnitImpl* page_cu,
                           const mojom::Event event) override;

  void BindToInterface(
      resource_coordinator::mojom::PageSignalGeneratorRequest request,
      const service_manager::BindSourceInfo& source_info);

 private:
  friend class PageSignalGeneratorImplTest;
  FRIEND_TEST_ALL_PREFIXES(PageSignalGeneratorImplTest, IsLoading);
  FRIEND_TEST_ALL_PREFIXES(PageSignalGeneratorImplTest, IsIdling);
  FRIEND_TEST_ALL_PREFIXES(PageSignalGeneratorImplTest,
                           PageDataCorrectlyManaged);
  FRIEND_TEST_ALL_PREFIXES(PageSignalGeneratorImplTest,
                           PageAlmostIdleTransitionsNoTimeout);
  FRIEND_TEST_ALL_PREFIXES(PageSignalGeneratorImplTest,
                           PageAlmostIdleTransitionsWithTimeout);

  // The state transitions for the PageAlmostIdle signal. In general a page
  // transitions through these states from top to bottom.
  enum LoadIdleState {
    // The initial state. Can only transition to kLoading from here.
    kLoadingNotStarted,
    // Loading has started. Almost idle signals are ignored in this state.
    // Can transition to kLoadedNotIdling and kLoadedAndIdling from here.
    kLoading,
    // Loading has completed, but the page has not started idling. Can only
    // transition to kLoadedAndIdling from here.
    kLoadedNotIdling,
    // Loading has completed, and the page is idling. Can transition to
    // kLoadedNotIdling or kLoadedAndIdle from here.
    kLoadedAndIdling,
    // Loading has completed and the page has been idling for sufficiently long.
    // This is the final state. Once this state has been reached a signal will
    // be emitted and no further state transitions will be tracked. Committing a
    // new non-same document navigation can start the cycle over again.
    kLoadedAndIdle
  };

  // Holds state per page CU. These are created via OnCoordinationUnitCreated
  // and destroyed via OnBeforeCoordinationUnitDestroyed.
  struct PageData {
    // Initially at kLoadingNotStarted. Transitions through the states via calls
    // to UpdateLoadIdleState. Is reset to kLoadingNotStarted when a non-same
    // document navigation is committed.
    LoadIdleState load_idle_state;
    // Marks the point in time when the DidStopLoading signal was received,
    // transitioning to kLoadedAndNotIdling or kLoadedAndIdling. This is used as
    // the basis for the kWaitingForIdleTimeout.
    base::TimeTicks loading_stopped;
    // Marks the point in time when the last transition to kLoadedAndIdling
    // occurred. Used for gating the transition to kLoadedAndIdle.
    base::TimeTicks idling_started;
    // A one-shot timer used for transitioning between kLoadedAndIdling and
    // kLoadedAndIdle.
    base::OneShotTimer idling_timer;
  };

  // These are called when properties/events affecting the load-idle state are
  // observed. Frame and Process variants will eventually all redirect to the
  // appropriate Page variant, where the real work is done.
  void UpdateLoadIdleStateFrame(const FrameCoordinationUnitImpl* frame_cu);
  void UpdateLoadIdleStatePage(const PageCoordinationUnitImpl* page_cu);
  void UpdateLoadIdleStateProcess(
      const ProcessCoordinationUnitImpl* process_cu);

  // This method is called when a property affecting the lifecycle state is
  // observed.
  void UpdateLifecycleState(const PageCoordinationUnitImpl* page_cu,
                            mojom::LifecycleState state);

  // Helper function for transitioning to the final state.
  void TransitionToLoadedAndIdle(const PageCoordinationUnitImpl* page_cu);

  // Convenience accessors for state associated with a |page_cu|.
  PageData* GetPageData(const PageCoordinationUnitImpl* page_cu);
  bool IsLoading(const PageCoordinationUnitImpl* page_cu);
  bool IsIdling(const PageCoordinationUnitImpl* page_cu);

  mojo::BindingSet<mojom::PageSignalGenerator> bindings_;
  mojo::InterfacePtrSet<mojom::PageSignalReceiver> receivers_;

  // Stores per Page CU data. This set is maintained by
  // OnCoordinationUnitCreated and OnBeforeCoordinationUnitDestroyed.
  std::map<const PageCoordinationUnitImpl*, PageData> page_data_;

  DISALLOW_COPY_AND_ASSIGN(PageSignalGeneratorImpl);
};

}  // namespace resource_coordinator

#endif  // SERVICES_RESOURCE_COORDINATOR_COORDINATION_UNIT_PAGE_SIGNAL_GENERATOR_IMPL_H_
