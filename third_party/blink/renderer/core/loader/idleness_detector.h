// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_CORE_LOADER_IDLENESS_DETECTOR_H_
#define THIRD_PARTY_BLINK_RENDERER_CORE_LOADER_IDLENESS_DETECTOR_H_

#include "base/macros.h"
#include "third_party/blink/renderer/core/core_export.h"
#include "third_party/blink/renderer/platform/heap/handle.h"
#include "third_party/blink/renderer/platform/scheduler/base/task_time_observer.h"
#include "third_party/blink/renderer/platform/timer.h"

namespace blink {

class LocalFrame;
class ResourceFetcher;

// IdlenessDetector observes network request count everytime a load is
// finshed after DOMContentLoadedEventEnd is fired, and emit network almost idle
// signal when there are no more than 2 network connection active in 0.5 second,
// and emit network idle signal when there is 0 network connection active in 0.5
// second.
class CORE_EXPORT IdlenessDetector
    : public GarbageCollectedFinalized<IdlenessDetector>,
      public base::sequence_manager::TaskTimeObserver {
 public:
  explicit IdlenessDetector(LocalFrame*);

  void Shutdown();
  void WillCommitLoad();
  void DomContentLoadedEventFired();
  // TODO(lpy) Don't need to pass in fetcher once the command line of disabling
  // PlzNavigate is removed.
  void OnWillSendRequest(ResourceFetcher*);
  void OnDidLoadResource();

  TimeTicks GetNetworkAlmostIdleTime();
  TimeTicks GetNetworkIdleTime();

  void Trace(blink::Visitor*);

 private:
  friend class IdlenessDetectorTest;

  // The page is quiet if there are no more than 2 active network requests for
  // this duration of time.
  static constexpr TimeDelta kNetworkQuietWindow =
      TimeDelta::FromMilliseconds(500);
  static constexpr TimeDelta kNetworkQuietWatchdog = TimeDelta::FromSeconds(2);
  static constexpr int kNetworkQuietMaximumConnections = 2;

  // TaskTimeObserver implementation.
  void WillProcessTask(double start_time) override;
  void DidProcessTask(double start_time, double end_time) override;

  void Stop();
  void NetworkQuietTimerFired(TimerBase*);

  Member<LocalFrame> local_frame_;
  bool task_observer_added_;

  bool in_network_0_quiet_period_ = true;
  bool in_network_2_quiet_period_ = true;

  // Store the accumulated time of network quiet.
  TimeTicks network_0_quiet_;
  TimeTicks network_2_quiet_;
  // Record the actual start time of network quiet.
  TimeTicks network_0_quiet_start_time_;
  TimeTicks network_2_quiet_start_time_;
  TaskRunnerTimer<IdlenessDetector> network_quiet_timer_;

  DISALLOW_COPY_AND_ASSIGN(IdlenessDetector);
};

}  // namespace blink

#endif
