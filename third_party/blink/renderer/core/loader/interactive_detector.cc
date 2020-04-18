// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/core/loader/interactive_detector.h"

#include "third_party/blink/public/platform/web_input_event.h"
#include "third_party/blink/renderer/core/dom/document.h"
#include "third_party/blink/renderer/core/frame/local_frame.h"
#include "third_party/blink/renderer/core/loader/document_loader.h"
#include "third_party/blink/renderer/platform/histogram.h"
#include "third_party/blink/renderer/platform/instrumentation/tracing/trace_event.h"
#include "third_party/blink/renderer/platform/loader/fetch/resource_fetcher.h"
#include "third_party/blink/renderer/platform/wtf/time.h"

namespace blink {

// Required length of main thread and network quiet window for determining
// Time to Interactive.
constexpr auto kTimeToInteractiveWindow = TimeDelta::FromSeconds(5);
// Network is considered "quiet" if there are no more than 2 active network
// requests for this duration of time.
constexpr int kNetworkQuietMaximumConnections = 2;

// static
const char InteractiveDetector::kSupplementName[] = "InteractiveDetector";

InteractiveDetector* InteractiveDetector::From(Document& document) {
  InteractiveDetector* detector =
      Supplement<Document>::From<InteractiveDetector>(document);
  if (!detector) {
    detector = new InteractiveDetector(document,
                                       new NetworkActivityChecker(&document));
    Supplement<Document>::ProvideTo(document, detector);
  }
  return detector;
}

const char* InteractiveDetector::SupplementName() {
  return "InteractiveDetector";
}

InteractiveDetector::InteractiveDetector(
    Document& document,
    NetworkActivityChecker* network_activity_checker)
    : Supplement<Document>(document),
      network_activity_checker_(network_activity_checker),
      time_to_interactive_timer_(
          document.GetTaskRunner(TaskType::kInternalDefault),
          this,
          &InteractiveDetector::TimeToInteractiveTimerFired) {}

InteractiveDetector::~InteractiveDetector() {
  LongTaskDetector::Instance().UnregisterObserver(this);
}

void InteractiveDetector::SetNavigationStartTime(
    TimeTicks navigation_start_time) {
  // Should not set nav start twice.
  DCHECK(page_event_times_.nav_start.is_null());

  // Don't record TTI for OOPIFs (yet).
  // TODO(crbug.com/808086): enable this case.
  if (!GetSupplementable()->IsInMainFrame())
    return;

  LongTaskDetector::Instance().RegisterObserver(this);
  page_event_times_.nav_start = navigation_start_time;
  TimeTicks initial_timer_fire_time =
      navigation_start_time + kTimeToInteractiveWindow;

  active_main_thread_quiet_window_start_ = navigation_start_time;
  active_network_quiet_window_start_ = navigation_start_time;
  StartOrPostponeCITimer(initial_timer_fire_time);
}

int InteractiveDetector::NetworkActivityChecker::GetActiveConnections() {
  DCHECK(document_);
  ResourceFetcher* fetcher = document_->Fetcher();
  return fetcher->BlockingRequestCount() + fetcher->NonblockingRequestCount();
}

int InteractiveDetector::ActiveConnections() {
  return network_activity_checker_->GetActiveConnections();
}

void InteractiveDetector::StartOrPostponeCITimer(TimeTicks timer_fire_time) {
  // This function should never be called after Time To Interactive is
  // reached.
  DCHECK(interactive_time_.is_null());

  // We give 1ms extra padding to the timer fire time to prevent floating point
  // arithmetic pitfalls when comparing window sizes.
  timer_fire_time += TimeDelta::FromMilliseconds(1);

  // Return if there is an active timer scheduled to fire later than
  // |timer_fire_time|.
  if (timer_fire_time < time_to_interactive_timer_fire_time_)
    return;

  TimeDelta delay = timer_fire_time - CurrentTimeTicks();
  time_to_interactive_timer_fire_time_ = timer_fire_time;

  if (delay <= TimeDelta()) {
    // This argument of this function is never used and only there to fulfill
    // the API contract. nullptr should work fine.
    TimeToInteractiveTimerFired(nullptr);
  } else {
    time_to_interactive_timer_.StartOneShot(delay, FROM_HERE);
  }
}

TimeTicks InteractiveDetector::GetInteractiveTime() const {
  // TODO(crbug.com/808685) Simplify FMP and TTI input invalidation.
  return page_event_times_.first_meaningful_paint_invalidated
             ? TimeTicks()
             : interactive_time_;
}

TimeTicks InteractiveDetector::GetInteractiveDetectionTime() const {
  // TODO(crbug.com/808685) Simplify FMP and TTI input invalidation.
  return page_event_times_.first_meaningful_paint_invalidated
             ? TimeTicks()
             : interactive_detection_time_;
}

TimeTicks InteractiveDetector::GetFirstInvalidatingInputTime() const {
  return page_event_times_.first_invalidating_input;
}

TimeDelta InteractiveDetector::GetFirstInputDelay() const {
  return page_event_times_.first_input_delay;
}

TimeTicks InteractiveDetector::GetFirstInputTimestamp() const {
  return page_event_times_.first_input_timestamp;
}

// This is called early enough in the pipeline that we don't need to worry about
// javascript dispatching untrusted input events.
void InteractiveDetector::HandleForFirstInputDelay(const WebInputEvent& event) {
  if (!page_event_times_.first_input_delay.is_zero())
    return;

  DCHECK(event.GetType() != WebInputEvent::kTouchStart);

  // We can't report a pointerDown until the pointerUp, in case it turns into a
  // scroll.
  if (event.GetType() == WebInputEvent::kPointerDown) {
    pending_pointerdown_delay_ = CurrentTimeTicks() - event.TimeStamp();
    pending_pointerdown_timestamp_ = event.TimeStamp();
    return;
  }

  bool event_is_meaningful =
      event.GetType() == WebInputEvent::kMouseDown ||
      event.GetType() == WebInputEvent::kKeyDown ||
      event.GetType() == WebInputEvent::kRawKeyDown ||
      // We need to explicitly include tap, as if there are no listeners, we
      // won't receive the pointer events.
      event.GetType() == WebInputEvent::kGestureTap ||
      event.GetType() == WebInputEvent::kPointerUp;

  if (!event_is_meaningful)
    return;

  TimeDelta delay;
  TimeTicks event_timestamp;
  if (event.GetType() == WebInputEvent::kPointerUp) {
    // It is possible that this pointer up doesn't match with the pointer down
    // whose delay is stored in pending_pointerdown_delay_. In this case, the
    // user gesture started by this event contained some non-scroll input, so we
    // consider it reasonable to use the delay of the initial event.
    delay = pending_pointerdown_delay_;
    event_timestamp = pending_pointerdown_timestamp_;
  } else {
    delay = CurrentTimeTicks() - event.TimeStamp();
    event_timestamp = event.TimeStamp();
  }

  pending_pointerdown_delay_ = base::TimeDelta();
  pending_pointerdown_timestamp_ = base::TimeTicks();

  page_event_times_.first_input_delay = delay;
  page_event_times_.first_input_timestamp = event_timestamp;

  if (GetSupplementable()->Loader())
    GetSupplementable()->Loader()->DidChangePerformanceTiming();
}

void InteractiveDetector::BeginNetworkQuietPeriod(TimeTicks current_time) {
  // Value of 0.0 indicates there is no currently actively network quiet window.
  DCHECK(active_network_quiet_window_start_.is_null());
  active_network_quiet_window_start_ = current_time;

  StartOrPostponeCITimer(current_time + kTimeToInteractiveWindow);
}

void InteractiveDetector::EndNetworkQuietPeriod(TimeTicks current_time) {
  DCHECK(!active_network_quiet_window_start_.is_null());

  if (current_time - active_network_quiet_window_start_ >=
      kTimeToInteractiveWindow) {
    network_quiet_windows_.emplace_back(active_network_quiet_window_start_,
                                        current_time);
  }
  active_network_quiet_window_start_ = TimeTicks();
}

// The optional opt_current_time, if provided, saves us a call to
// CurrentTimeTicksInSeconds.
void InteractiveDetector::UpdateNetworkQuietState(
    double request_count,
    base::Optional<TimeTicks> opt_current_time) {
  if (request_count <= kNetworkQuietMaximumConnections &&
      active_network_quiet_window_start_.is_null()) {
    // Not using `value_or(CurrentTimeTicksInSeconds())` here because
    // arguments to functions are eagerly evaluated, which always call
    // CurrentTimeTicksInSeconds.
    TimeTicks current_time =
        opt_current_time ? opt_current_time.value() : CurrentTimeTicks();
    BeginNetworkQuietPeriod(current_time);
  } else if (request_count > kNetworkQuietMaximumConnections &&
             !active_network_quiet_window_start_.is_null()) {
    TimeTicks current_time =
        opt_current_time ? opt_current_time.value() : CurrentTimeTicks();
    EndNetworkQuietPeriod(current_time);
  }
}

void InteractiveDetector::OnResourceLoadBegin(
    base::Optional<TimeTicks> load_begin_time) {
  if (!GetSupplementable())
    return;
  if (!interactive_time_.is_null())
    return;
  // The request that is about to begin is not counted in ActiveConnections(),
  // so we add one to it.
  UpdateNetworkQuietState(ActiveConnections() + 1, load_begin_time);
}

// The optional load_finish_time, if provided, saves us a call to
// CurrentTimeTicksInSeconds.
void InteractiveDetector::OnResourceLoadEnd(
    base::Optional<TimeTicks> load_finish_time) {
  if (!GetSupplementable())
    return;
  if (!interactive_time_.is_null())
    return;
  UpdateNetworkQuietState(ActiveConnections(), load_finish_time);
}

void InteractiveDetector::OnLongTaskDetected(TimeTicks start_time,
                                             TimeTicks end_time) {
  // We should not be receiving long task notifications after Time to
  // Interactive has already been reached.
  DCHECK(interactive_time_.is_null());
  TimeDelta quiet_window_length =
      start_time - active_main_thread_quiet_window_start_;
  if (quiet_window_length >= kTimeToInteractiveWindow) {
    main_thread_quiet_windows_.emplace_back(
        active_main_thread_quiet_window_start_, start_time);
  }
  active_main_thread_quiet_window_start_ = end_time;
  StartOrPostponeCITimer(end_time + kTimeToInteractiveWindow);
}

void InteractiveDetector::OnFirstMeaningfulPaintDetected(
    TimeTicks fmp_time,
    FirstMeaningfulPaintDetector::HadUserInput user_input_before_fmp) {
  DCHECK(page_event_times_.first_meaningful_paint
             .is_null());  // Should not set FMP twice.
  page_event_times_.first_meaningful_paint = fmp_time;
  page_event_times_.first_meaningful_paint_invalidated =
      user_input_before_fmp == FirstMeaningfulPaintDetector::kHadUserInput;
  if (CurrentTimeTicks() - fmp_time >= kTimeToInteractiveWindow) {
    // We may have reached TTCI already. Check right away.
    CheckTimeToInteractiveReached();
  } else {
    StartOrPostponeCITimer(page_event_times_.first_meaningful_paint +
                           kTimeToInteractiveWindow);
  }
}

void InteractiveDetector::OnDomContentLoadedEnd(TimeTicks dcl_end_time) {
  // InteractiveDetector should only receive the first DCL event.
  DCHECK(page_event_times_.dom_content_loaded_end.is_null());
  page_event_times_.dom_content_loaded_end = dcl_end_time;
  CheckTimeToInteractiveReached();
}

void InteractiveDetector::OnInvalidatingInputEvent(
    TimeTicks invalidation_time) {
  if (!page_event_times_.first_invalidating_input.is_null())
    return;

  // In some edge cases (e.g. inaccurate input timestamp provided through remote
  // debugging protocol) we might receive an input timestamp that is earlier
  // than navigation start. Since invalidating input timestamp before navigation
  // start in non-sensical, we clamp it at navigation start.
  page_event_times_.first_invalidating_input =
      std::max(invalidation_time, page_event_times_.nav_start);

  if (GetSupplementable()->Loader())
    GetSupplementable()->Loader()->DidChangePerformanceTiming();
}

void InteractiveDetector::OnFirstInputDelay(TimeDelta delay) {
  if (!page_event_times_.first_input_delay.is_zero())
    return;

  page_event_times_.first_input_delay = delay;
  if (GetSupplementable()->Loader())
    GetSupplementable()->Loader()->DidChangePerformanceTiming();
}

void InteractiveDetector::TimeToInteractiveTimerFired(TimerBase*) {
  if (!GetSupplementable() || !interactive_time_.is_null())
    return;

  // Value of 0.0 indicates there is currently no active timer.
  time_to_interactive_timer_fire_time_ = TimeTicks();
  CheckTimeToInteractiveReached();
}

void InteractiveDetector::AddCurrentlyActiveQuietIntervals(
    TimeTicks current_time) {
  // Network is currently quiet.
  if (!active_network_quiet_window_start_.is_null()) {
    if (current_time - active_network_quiet_window_start_ >=
        kTimeToInteractiveWindow) {
      network_quiet_windows_.emplace_back(active_network_quiet_window_start_,
                                          current_time);
    }
  }

  // Since this code executes on the main thread, we know that no task is
  // currently running on the main thread. We can therefore skip checking.
  // main_thread_quiet_window_being != 0.0.
  if (current_time - active_main_thread_quiet_window_start_ >=
      kTimeToInteractiveWindow) {
    main_thread_quiet_windows_.emplace_back(
        active_main_thread_quiet_window_start_, current_time);
  }
}

void InteractiveDetector::RemoveCurrentlyActiveQuietIntervals() {
  if (!network_quiet_windows_.empty() &&
      network_quiet_windows_.back().Low() ==
          active_network_quiet_window_start_) {
    network_quiet_windows_.pop_back();
  }

  if (!main_thread_quiet_windows_.empty() &&
      main_thread_quiet_windows_.back().Low() ==
          active_main_thread_quiet_window_start_) {
    main_thread_quiet_windows_.pop_back();
  }
}

TimeTicks InteractiveDetector::FindInteractiveCandidate(TimeTicks lower_bound) {
  // Main thread iterator.
  auto it_mt = main_thread_quiet_windows_.begin();
  // Network iterator.
  auto it_net = network_quiet_windows_.begin();

  while (it_mt < main_thread_quiet_windows_.end() &&
         it_net < network_quiet_windows_.end()) {
    if (it_mt->High() <= lower_bound) {
      it_mt++;
      continue;
    }
    if (it_net->High() <= lower_bound) {
      it_net++;
      continue;
    }

    // First handling the no overlap cases.
    // [ main thread interval ]
    //                                     [ network interval ]
    if (it_mt->High() <= it_net->Low()) {
      it_mt++;
      continue;
    }
    //                                     [ main thread interval ]
    // [   network interval   ]
    if (it_net->High() <= it_mt->Low()) {
      it_net++;
      continue;
    }

    // At this point we know we have a non-empty overlap after lower_bound.
    TimeTicks overlap_start =
        std::max({it_mt->Low(), it_net->Low(), lower_bound});
    TimeTicks overlap_end = std::min(it_mt->High(), it_net->High());
    TimeDelta overlap_duration = overlap_end - overlap_start;
    if (overlap_duration >= kTimeToInteractiveWindow) {
      return std::max(lower_bound, it_mt->Low());
    }

    // The interval with earlier end time will not produce any more overlap, so
    // we move on from it.
    if (it_mt->High() <= it_net->High()) {
      it_mt++;
    } else {
      it_net++;
    }
  }

  // Time To Interactive candidate not found.
  return TimeTicks();
}

void InteractiveDetector::CheckTimeToInteractiveReached() {
  // Already detected Time to Interactive.
  if (!interactive_time_.is_null())
    return;

  // FMP and DCL have not been detected yet.
  if (page_event_times_.first_meaningful_paint.is_null() ||
      page_event_times_.dom_content_loaded_end.is_null())
    return;

  const TimeTicks current_time = CurrentTimeTicks();
  if (current_time - page_event_times_.first_meaningful_paint <
      kTimeToInteractiveWindow) {
    // Too close to FMP to determine Time to Interactive.
    return;
  }

  AddCurrentlyActiveQuietIntervals(current_time);
  const TimeTicks interactive_candidate =
      FindInteractiveCandidate(page_event_times_.first_meaningful_paint);
  RemoveCurrentlyActiveQuietIntervals();

  // No Interactive Candidate found.
  if (interactive_candidate.is_null())
    return;

  interactive_time_ = std::max(
      {interactive_candidate, page_event_times_.dom_content_loaded_end});
  interactive_detection_time_ = CurrentTimeTicks();
  OnTimeToInteractiveDetected();
}

void InteractiveDetector::OnTimeToInteractiveDetected() {
  LongTaskDetector::Instance().UnregisterObserver(this);
  main_thread_quiet_windows_.clear();
  network_quiet_windows_.clear();

  bool had_user_input_before_interactive =
      !page_event_times_.first_invalidating_input.is_null() &&
      page_event_times_.first_invalidating_input < interactive_time_;

  // We log the trace event even if there is user input, but annotate the event
  // with whether that happened.
  TRACE_EVENT_MARK_WITH_TIMESTAMP2(
      "loading,rail", "InteractiveTime", interactive_time_, "frame",
      ToTraceValue(GetSupplementable()->GetFrame()),
      "had_user_input_before_interactive", had_user_input_before_interactive);

  // We only send TTI to Performance Timing Observers if FMP was not invalidated
  // by input.
  // TODO(crbug.com/808685) Simplify FMP and TTI input invalidation.
  if (!page_event_times_.first_meaningful_paint_invalidated) {
    if (GetSupplementable()->Loader())
      GetSupplementable()->Loader()->DidChangePerformanceTiming();
  }
}

void InteractiveDetector::Trace(Visitor* visitor) {
  Supplement<Document>::Trace(visitor);
}

}  // namespace blink
