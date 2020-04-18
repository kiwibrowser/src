// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/ui/ws/event_dispatcher_impl.h"

#include "base/debug/debugger.h"
#include "services/ui/public/interfaces/window_tree_constants.mojom.h"
#include "services/ui/ws/accelerator.h"
#include "services/ui/ws/async_event_dispatcher.h"
#include "services/ui/ws/async_event_dispatcher_lookup.h"
#include "services/ui/ws/event_dispatcher_delegate.h"
#include "services/ui/ws/event_location.h"
#include "services/ui/ws/event_processor.h"
#include "services/ui/ws/server_window.h"
#include "services/ui/ws/server_window_tracker.h"
#include "ui/events/event.h"

namespace ui {
namespace ws {
namespace {

bool CanEventsBeCoalesced(const ui::Event& one, const ui::Event& two) {
  if (one.type() != two.type() || one.flags() != two.flags())
    return false;

  // TODO(sad): wheel events can also be merged.
  if (one.type() != ui::ET_POINTER_MOVED)
    return false;

  return one.AsPointerEvent()->pointer_details().id ==
         two.AsPointerEvent()->pointer_details().id;
}

std::unique_ptr<ui::Event> CoalesceEvents(std::unique_ptr<ui::Event> first,
                                          std::unique_ptr<ui::Event> second) {
  DCHECK(first->type() == ui::ET_POINTER_MOVED)
      << " Non-move events cannot be merged yet.";
  // For mouse moves, the new event just replaces the old event, but we need to
  // use the latency from the old event.
  second->set_latency(*first->latency());
  second->latency()->set_coalesced();
  return second;
}

base::TimeDelta GetDefaultAckTimerDelay() {
#if defined(NDEBUG)
  return base::TimeDelta::FromMilliseconds(100);
#else
  return base::TimeDelta::FromMilliseconds(1000);
#endif
}

}  // namespace

// See EventDispatcherImpl::EventTask::Type::kProcessedEvent for details on
// this.
class EventDispatcherImpl::ProcessedEventTarget {
 public:
  ProcessedEventTarget(
      AsyncEventDispatcherLookup* async_event_dispatcher_lookup,
      ServerWindow* window,
      ClientSpecificId client_id,
      Accelerator* accelerator)
      : async_event_dispatcher_lookup_(async_event_dispatcher_lookup),
        client_id_(client_id) {
    DCHECK(async_event_dispatcher_lookup_);
    DCHECK(window);
    tracker_.Add(window);
    if (accelerator)
      accelerator_ = accelerator->GetWeakPtr();
  }

  ~ProcessedEventTarget() {}

  // Return true if the event is still valid. The event becomes invalid if
  // the window is destroyed while waiting to dispatch.
  bool IsValid() {
    return window() &&
           async_event_dispatcher_lookup_->GetAsyncEventDispatcherById(
               client_id_);
  }

  ServerWindow* window() {
    return tracker_.windows().empty() ? nullptr : tracker_.windows().front();
  }

  ClientSpecificId client_id() const { return client_id_; }

  base::WeakPtr<Accelerator> accelerator() { return accelerator_; }

 private:
  AsyncEventDispatcherLookup* async_event_dispatcher_lookup_;
  ServerWindowTracker tracker_;
  const ClientSpecificId client_id_;
  base::WeakPtr<Accelerator> accelerator_;

  DISALLOW_COPY_AND_ASSIGN(ProcessedEventTarget);
};

EventDispatcherImpl::InFlightEventDispatchDetails::InFlightEventDispatchDetails(
    EventDispatcherImpl* event_dispatcher,
    AsyncEventDispatcher* async_event_dispatcher,
    int64_t display_id,
    const Event& event,
    EventDispatchPhase phase)
    : async_event_dispatcher(async_event_dispatcher),
      display_id(display_id),
      event(Event::Clone(event)),
      phase(phase),
      weak_factory_(event_dispatcher) {}

EventDispatcherImpl::InFlightEventDispatchDetails::
    ~InFlightEventDispatchDetails() {}

// Contains data used for event processing that needs to happen. See enum for
// details.
struct EventDispatcherImpl::EventTask {
  enum class Type {
    // ProcessEvent() was called while waiting on a client or EventProcessor
    // to complete processing. |event| is non-null and |processed_target| is
    // null.
    kEvent,

    // In certain situations EventProcessor::ProcessEvent() generates more than
    // one event. When that happens, |kProcessedEvent| is used for all events
    // after the first. For example, a move may result in an exit for one
    // Window and and an enter for another Window. The event generated for the
    // enter results in an EventTask of type |kProcessedEvent|. In this case
    // both |event| and |processed_target| are valid.
    kProcessedEvent,

    // ScheduleCallbackWhenDoneProcessingEvents() is called while waiting on
    // a client or EventProcessor. |event| and |processed_target| are null.
    kClosure
  };

  EventTask() = default;
  ~EventTask() = default;

  Type type() const {
    if (done_closure)
      return Type::kClosure;
    if (processed_target) {
      DCHECK(event);
      return Type::kProcessedEvent;
    }
    DCHECK(event);
    return Type::kEvent;
  }

  std::unique_ptr<Event> event;
  std::unique_ptr<ProcessedEventTarget> processed_target;
  EventLocation event_location;
  base::OnceClosure done_closure;
};

EventDispatcherImpl::EventDispatcherImpl(
    AsyncEventDispatcherLookup* async_event_dispatcher_lookup,
    AsyncEventDispatcher* accelerator_dispatcher,
    EventDispatcherDelegate* delegate)
    : async_event_dispatcher_lookup_(async_event_dispatcher_lookup),
      accelerator_dispatcher_(accelerator_dispatcher),
      delegate_(delegate) {
  DCHECK(async_event_dispatcher_lookup_);
}

EventDispatcherImpl::~EventDispatcherImpl() = default;

void EventDispatcherImpl::Init(EventProcessor* event_processor) {
  DCHECK(event_processor);
  DCHECK(!event_processor_);
  event_processor_ = event_processor;
}

void EventDispatcherImpl::ProcessEvent(ui::Event* event,
                                       const EventLocation& event_location) {
  // If this is still waiting for an ack from a previously sent event, then
  // queue the event so it's dispatched once the ack is received.
  if (IsProcessingEvent()) {
    if (!event_tasks_.empty() &&
        event_tasks_.back()->type() == EventTask::Type::kEvent &&
        CanEventsBeCoalesced(*event_tasks_.back()->event, *event)) {
      event_tasks_.back()->event = CoalesceEvents(
          std::move(event_tasks_.back()->event), ui::Event::Clone(*event));
      event_tasks_.back()->event_location = event_location;
      return;
    }
    QueueEvent(*event, nullptr, event_location);
    return;
  }

  QueueEvent(*event, nullptr, event_location);
  ProcessEventTasks();
}

bool EventDispatcherImpl::IsProcessingEvent() const {
  return in_flight_event_dispatch_details_ ||
         event_processor_->IsProcessingEvent();
}

const ui::Event* EventDispatcherImpl::GetInFlightEvent() const {
  return in_flight_event_dispatch_details_
             ? in_flight_event_dispatch_details_->event.get()
             : nullptr;
}

void EventDispatcherImpl::ScheduleCallbackWhenDoneProcessingEvents(
    base::OnceClosure closure) {
  DCHECK(closure);
  if (!IsProcessingEvent()) {
    std::move(closure).Run();
    return;
  }

  // TODO(sky): use make_unique (presubmit check fails on make_unique).
  std::unique_ptr<EventTask> event_task(new EventTask());
  event_task->done_closure = std::move(closure);
  event_tasks_.push(std::move(event_task));
}

void EventDispatcherImpl::OnWillDestroyAsyncEventDispatcher(
    AsyncEventDispatcher* target) {
  if (!in_flight_event_dispatch_details_ ||
      in_flight_event_dispatch_details_->async_event_dispatcher != target) {
    return;
  }

  // The target is going to be deleted and won't ack the event, simulate an ack
  // so we don't wait for the timer to fire.
  OnDispatchInputEventDone(mojom::EventResult::UNHANDLED);
}

void EventDispatcherImpl::ScheduleInputEventTimeout(
    AsyncEventDispatcher* async_event_dispatcher,
    int64_t display_id,
    const Event& event,
    EventDispatchPhase phase) {
  DCHECK(!in_flight_event_dispatch_details_);
  std::unique_ptr<InFlightEventDispatchDetails> details =
      std::make_unique<InFlightEventDispatchDetails>(
          this, async_event_dispatcher, display_id, event, phase);

  // TODO(sad): Adjust this delay, possibly make this dynamic.
  const base::TimeDelta max_delay = base::debug::BeingDebugged()
                                        ? base::TimeDelta::FromDays(1)
                                        : GetDefaultAckTimerDelay();
  details->timer.Start(
      FROM_HERE, max_delay,
      base::Bind(&EventDispatcherImpl::OnDispatchInputEventTimeout,
                 details->weak_factory_.GetWeakPtr()));
  in_flight_event_dispatch_details_ = std::move(details);
}

void EventDispatcherImpl::ProcessEventTasks() {
  // Loop through |event_tasks_| stopping after dispatching the first valid
  // event.
  while (!event_tasks_.empty() && !IsProcessingEvent()) {
    std::unique_ptr<EventTask> task = std::move(event_tasks_.front());
    event_tasks_.pop();

    switch (task->type()) {
      case EventTask::Type::kClosure:
        std::move(task->done_closure).Run();
        break;
      case EventTask::Type::kEvent:
        delegate_->OnWillProcessEvent(*task->event, task->event_location);
        event_processor_->ProcessEvent(
            *task->event, task->event_location,
            EventProcessor::AcceleratorMatchPhase::ANY);
        break;
      case EventTask::Type::kProcessedEvent:
        if (task->processed_target->IsValid()) {
          DispatchInputEventToWindowImpl(task->processed_target->window(),
                                         task->processed_target->client_id(),
                                         task->event_location, *task->event,
                                         task->processed_target->accelerator());
        }
        break;
    }
  }
}

// TODO(riajiang): We might want to do event targeting for the next event while
// waiting for the current event to be dispatched. https://crbug.com/724521
void EventDispatcherImpl::DispatchInputEventToWindowImpl(
    ServerWindow* target,
    ClientSpecificId client_id,
    const EventLocation& event_location,
    const ui::Event& event,
    base::WeakPtr<Accelerator> accelerator) {
  DCHECK(!in_flight_event_dispatch_details_);
  DCHECK(target);
  target = delegate_->OnWillDispatchInputEvent(target, client_id,
                                               event_location, event);
  AsyncEventDispatcher* async_event_dispatcher =
      async_event_dispatcher_lookup_->GetAsyncEventDispatcherById(client_id);
  DCHECK(async_event_dispatcher);
  ScheduleInputEventTimeout(async_event_dispatcher, event_location.display_id,
                            event, EventDispatchPhase::TARGET);
  in_flight_event_dispatch_details_->post_target_accelerator = accelerator;

  async_event_dispatcher->DispatchEvent(
      target, event, event_location,
      base::BindOnce(
          &EventDispatcherImpl::OnDispatchInputEventDone,
          in_flight_event_dispatch_details_->weak_factory_.GetWeakPtr()));
}

void EventDispatcherImpl::OnDispatchInputEventTimeout() {
  DCHECK(in_flight_event_dispatch_details_);
  delegate_->OnEventDispatchTimedOut(
      in_flight_event_dispatch_details_->async_event_dispatcher);
  if (in_flight_event_dispatch_details_->phase ==
      EventDispatchPhase::PRE_TARGET_ACCELERATOR) {
    OnAcceleratorDone(mojom::EventResult::UNHANDLED, {});
  } else {
    OnDispatchInputEventDone(mojom::EventResult::UNHANDLED);
  }
}

void EventDispatcherImpl::OnDispatchInputEventDone(mojom::EventResult result) {
  DCHECK(in_flight_event_dispatch_details_);
  std::unique_ptr<InFlightEventDispatchDetails> details =
      std::move(in_flight_event_dispatch_details_);

  if (result == mojom::EventResult::UNHANDLED &&
      details->post_target_accelerator) {
    OnAccelerator(details->post_target_accelerator->id(), details->display_id,
                  *details->event, AcceleratorPhase::kPost);
  }

  ProcessEventTasks();
}

void EventDispatcherImpl::OnAcceleratorDone(
    mojom::EventResult result,
    const base::flat_map<std::string, std::vector<uint8_t>>& properties) {
  DCHECK(in_flight_event_dispatch_details_);
  DCHECK_EQ(EventDispatchPhase::PRE_TARGET_ACCELERATOR,
            in_flight_event_dispatch_details_->phase);

  std::unique_ptr<InFlightEventDispatchDetails> details =
      std::move(in_flight_event_dispatch_details_);

  if (result == mojom::EventResult::UNHANDLED) {
    DCHECK(details->event->IsKeyEvent());
    if (!properties.empty())
      details->event->AsKeyEvent()->SetProperties(properties);
    event_processor_->ProcessEvent(
        *details->event, EventLocation(details->display_id),
        EventProcessor::AcceleratorMatchPhase::POST_ONLY);
  } else {
    // We're not going to process the event any further, notify the delegate.
    delegate_->OnAsyncEventDispatcherHandledAccelerator(*details->event,
                                                        details->display_id);
    ProcessEventTasks();
  }
}

void EventDispatcherImpl::QueueEvent(
    const ui::Event& event,
    std::unique_ptr<ProcessedEventTarget> processed_event_target,
    const EventLocation& event_location) {
  std::unique_ptr<EventTask> queued_event(new EventTask);
  queued_event->event = ui::Event::Clone(event);
  queued_event->processed_target = std::move(processed_event_target);
  queued_event->event_location = event_location;
  event_tasks_.push(std::move(queued_event));
}

void EventDispatcherImpl::DispatchInputEventToWindow(
    ServerWindow* target,
    ClientSpecificId client_id,
    const EventLocation& event_location,
    const Event& event,
    Accelerator* accelerator) {
  if (in_flight_event_dispatch_details_) {
    std::unique_ptr<ProcessedEventTarget> processed_event_target =
        std::make_unique<ProcessedEventTarget>(async_event_dispatcher_lookup_,
                                               target, client_id, accelerator);
    QueueEvent(event, std::move(processed_event_target), event_location);
    return;
  }

  base::WeakPtr<Accelerator> weak_accelerator;
  if (accelerator)
    weak_accelerator = accelerator->GetWeakPtr();
  DispatchInputEventToWindowImpl(target, client_id, event_location, event,
                                 weak_accelerator);
}

void EventDispatcherImpl::OnAccelerator(uint32_t accelerator_id,
                                        int64_t display_id,
                                        const ui::Event& event,
                                        AcceleratorPhase phase) {
  const bool needs_ack = phase == AcceleratorPhase::kPre;
  AsyncEventDispatcher::AcceleratorCallback ack_callback;
  if (needs_ack) {
    ScheduleInputEventTimeout(accelerator_dispatcher_, display_id, event,
                              EventDispatchPhase::PRE_TARGET_ACCELERATOR);
    ack_callback = base::BindOnce(
        &EventDispatcherImpl::OnAcceleratorDone,
        in_flight_event_dispatch_details_->weak_factory_.GetWeakPtr());
  }
  accelerator_dispatcher_->DispatchAccelerator(accelerator_id, event,
                                               std::move(ack_callback));
}

}  // namespace ws
}  // namespace ui
