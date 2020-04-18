// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_UI_WS_EVENT_DISPATCHER_IMPL_H_
#define SERVICES_UI_WS_EVENT_DISPATCHER_IMPL_H_

#include <stdint.h>

#include <string>
#include <vector>

#include "base/containers/flat_map.h"
#include "base/containers/queue.h"
#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "base/timer/timer.h"
#include "services/ui/ws/event_dispatcher.h"
#include "services/ui/ws/event_processor_delegate.h"

namespace ui {

class Event;

namespace mojom {
enum class EventResult;
}

namespace ws {

class EventDispatcherDelegate;
class EventProcessor;
class AsyncEventDispatcher;
class AsyncEventDispatcherLookup;

struct EventLocation;

// EventDispatcherImpl is the entry point for event related processing done by
// the Window Service. Events received from the platform are forwarded to
// ProcessEvent(). ProcessEvent() may queue the event for later processing (if
// waiting for an event to be dispatched, or waiting on EventProcessor to
// complete processing). EventDispatcherImpl ultimately calls to EventProcessor
// for processing. EventDispatcherImpl uses AsyncEventDispatcher for dispatch
// to clients.
class EventDispatcherImpl : public EventDispatcher {
 public:
  // |accelerator_dispatcher| must outlive this class.
  EventDispatcherImpl(AsyncEventDispatcherLookup* async_event_dispatcher_lookup,
                      AsyncEventDispatcher* accelerator_dispatcher,
                      EventDispatcherDelegate* delegate);
  ~EventDispatcherImpl() override;

  void Init(EventProcessor* processor);

  // Processes an event. If IsProcessingEvent() is true, this queues up the
  // event for later processing. This doesn't take ownership of |event|, but it
  // may modify it.
  void ProcessEvent(ui::Event* event, const EventLocation& event_location);

  // Returns true if actively processing an event. This includes waiting for an
  // AsyncEventDispatcher to respond to an event.
  bool IsProcessingEvent() const;

  // Returns the event this EventDispatcherImpl is waiting on a response for, or
  // null if not waiting on an AsyncEventDispatcher.
  const ui::Event* GetInFlightEvent() const;

  // Notifies |closure| once done processing currently queued events. This
  // notifies |closure| immediately if IsProcessingEvent() returns false.
  void ScheduleCallbackWhenDoneProcessingEvents(base::OnceClosure closure);

  // Called when an AsyncEventDispatcher is destroyed.
  // TODO(sky): AsyncEventDispatcher should support observers.
  void OnWillDestroyAsyncEventDispatcher(AsyncEventDispatcher* target);

 private:
  friend class EventDispatcherImplTestApi;
  class ProcessedEventTarget;
  struct EventTask;

  enum class EventDispatchPhase {
    // Not actively dispatching.
    NONE,

    // A PRE_TARGET accelerator has been encountered and we're awaiting the ack.
    PRE_TARGET_ACCELERATOR,

    // Dispatching to the target, awaiting the ack.
    TARGET,
  };

  // Tracks state associated with an event being dispatched to an
  // AsyncEventDispatcher.
  struct InFlightEventDispatchDetails {
    InFlightEventDispatchDetails(EventDispatcherImpl* dispatcher,
                                 AsyncEventDispatcher* async_event_dispatcher,
                                 int64_t display_id,
                                 const Event& event,
                                 EventDispatchPhase phase);
    ~InFlightEventDispatchDetails();

    // Timer used to know when the AsyncEventDispatcher has taken too long.
    base::OneShotTimer timer;
    AsyncEventDispatcher* async_event_dispatcher;
    int64_t display_id;
    std::unique_ptr<Event> event;
    EventDispatchPhase phase;
    base::WeakPtr<Accelerator> post_target_accelerator;

    // Used for callbacks associated with the processing (such as |timer|). This
    // is used rather than a WeakPtrFactory on EventDispatcherImpl itself so
    // that it's scoped to the life of waiting for the AsyncEventDispatcher to
    // respond.
    base::WeakPtrFactory<EventDispatcherImpl> weak_factory_;
  };

  // Creates an InFlightEventDispatchDetails and schedules a timer that calls
  // OnDispatchInputEventTimeout() when done. This is used prior to asking
  // an AsyncEventDispatcher to dispatch an event or accelerator.
  void ScheduleInputEventTimeout(AsyncEventDispatcher* async_event_dispatcher,
                                 int64_t display_id,
                                 const Event& event,
                                 EventDispatchPhase phase);

  // Processes all pending events until there are no more, or this class is
  // waiting on on a result from either EventProcessor of AsyncEventDispatcher.
  void ProcessEventTasks();

  // Actual implementation of DispatchInputEventToWindow(). Schedules a timeout
  // and calls DispatchEvent() on the appropriate AsyncEventDispatcher.
  void DispatchInputEventToWindowImpl(ServerWindow* target,
                                      ClientSpecificId client_id,
                                      const EventLocation& event_location,
                                      const ui::Event& event,
                                      base::WeakPtr<Accelerator> accelerator);

  // The AsyncEventDispatcher has not completed processing in an appropriate
  // amount of time.
  void OnDispatchInputEventTimeout();

  // The AsyncEventDispatcher has completed processing the current event.
  void OnDispatchInputEventDone(mojom::EventResult result);

  // Called when |accelerator_dispatcher_| has completed processing the
  // accelerator.
  void OnAcceleratorDone(
      mojom::EventResult result,
      const base::flat_map<std::string, std::vector<uint8_t>>& properties);

  // Schedules an event to be processed later.
  void QueueEvent(const Event& event,
                  std::unique_ptr<ProcessedEventTarget> processed_event_target,
                  const EventLocation& event_location);

  // EventDispatcher:
  void DispatchInputEventToWindow(ServerWindow* target,
                                  ClientSpecificId client_id,
                                  const EventLocation& event_location,
                                  const Event& event,
                                  Accelerator* accelerator) override;
  void OnAccelerator(uint32_t accelerator_id,
                     int64_t display_id,
                     const ui::Event& event,
                     AcceleratorPhase phase) override;

  EventProcessor* event_processor_ = nullptr;

  // Used to map a ClientId to an AsyncEventDispatcher.
  AsyncEventDispatcherLookup* const async_event_dispatcher_lookup_;

  // Processes accelerators. This AsyncEventDispatcher corresponds to the
  // AsyncEventDispatcher accelerators originate from, which is typically the
  // WindowManager.
  AsyncEventDispatcher* const accelerator_dispatcher_;

  EventDispatcherDelegate* delegate_;

  // Used for any event related tasks that need to be processed. Tasks are added
  // to the queue anytime work comes in while waiting for an
  // AsyncEventDispatcher to respond, or waiting for async hit-testing
  // processing to complete.
  base::queue<std::unique_ptr<EventTask>> event_tasks_;

  // If non-null we're actively waiting for a response from an
  // AsyncEventDispatcher.
  std::unique_ptr<InFlightEventDispatchDetails>
      in_flight_event_dispatch_details_;

  DISALLOW_COPY_AND_ASSIGN(EventDispatcherImpl);
};

}  // namespace ws
}  // namespace ui

#endif  // SERVICES_UI_WS_EVENT_DISPATCHER_IMPL_H_
