/*
 * Copyright (C) 1999 Lars Knoll (knoll@kde.org)
 *           (C) 1999 Antti Koivisto (koivisto@kde.org)
 *           (C) 2001 Dirk Mueller (mueller@kde.org)
 * Copyright (C) 2004, 2005, 2006, 2007 Apple Inc. All rights reserved.
 * Copyright (C) 2006 Alexey Proskuryakov (ap@webkit.org)
 *           (C) 2007, 2008 Nikolas Zimmermann <zimmermann@kde.org>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE COMPUTER, INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE COMPUTER, INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

#include "third_party/blink/renderer/core/dom/events/event_target.h"

#include <memory>

#include "third_party/blink/public/web/web_settings.h"
#include "third_party/blink/renderer/bindings/core/v8/add_event_listener_options_or_boolean.h"
#include "third_party/blink/renderer/bindings/core/v8/event_listener_options_or_boolean.h"
#include "third_party/blink/renderer/bindings/core/v8/exception_state.h"
#include "third_party/blink/renderer/bindings/core/v8/script_event_listener.h"
#include "third_party/blink/renderer/bindings/core/v8/source_location.h"
#include "third_party/blink/renderer/core/dom/events/event.h"
#include "third_party/blink/renderer/core/dom/events/event_target_impl.h"
#include "third_party/blink/renderer/core/dom/exception_code.h"
#include "third_party/blink/renderer/core/editing/editor.h"
#include "third_party/blink/renderer/core/events/event_util.h"
#include "third_party/blink/renderer/core/events/pointer_event.h"
#include "third_party/blink/renderer/core/frame/frame_console.h"
#include "third_party/blink/renderer/core/frame/local_dom_window.h"
#include "third_party/blink/renderer/core/frame/performance_monitor.h"
#include "third_party/blink/renderer/core/frame/settings.h"
#include "third_party/blink/renderer/core/frame/use_counter.h"
#include "third_party/blink/renderer/core/frame/location.h"
#include "third_party/blink/renderer/core/inspector/console_message.h"
#include "third_party/blink/renderer/core/probe/core_probes.h"
#include "third_party/blink/renderer/platform/bindings/v8_dom_activity_logger.h"
#include "third_party/blink/renderer/platform/event_dispatch_forbidden_scope.h"
#include "third_party/blink/renderer/platform/histogram.h"
#include "third_party/blink/renderer/platform/wtf/assertions.h"
#include "third_party/blink/renderer/platform/wtf/std_lib_extras.h"
#include "third_party/blink/renderer/platform/wtf/threading.h"
#include "third_party/blink/renderer/platform/wtf/vector.h"

namespace blink {
namespace {

enum PassiveForcedListenerResultType {
  kPreventDefaultNotCalled,
  kDocumentLevelTouchPreventDefaultCalled,
  kPassiveForcedListenerResultTypeMax
};

Event::PassiveMode EventPassiveMode(
    const RegisteredEventListener& event_listener) {
  if (!event_listener.Passive()) {
    if (event_listener.PassiveSpecified())
      return Event::PassiveMode::kNotPassive;
    return Event::PassiveMode::kNotPassiveDefault;
  }
  if (event_listener.PassiveForcedForDocumentTarget())
    return Event::PassiveMode::kPassiveForcedDocumentLevel;
  if (event_listener.PassiveSpecified())
    return Event::PassiveMode::kPassive;
  return Event::PassiveMode::kPassiveDefault;
}

Settings* WindowSettings(LocalDOMWindow* executing_window) {
  if (executing_window) {
    if (LocalFrame* frame = executing_window->GetFrame()) {
      return frame->GetSettings();
    }
  }
  return nullptr;
}

bool IsTouchScrollBlockingEvent(const AtomicString& event_type) {
  return event_type == EventTypeNames::touchstart ||
         event_type == EventTypeNames::touchmove;
}

bool IsScrollBlockingEvent(const AtomicString& event_type) {
  return IsTouchScrollBlockingEvent(event_type) ||
         event_type == EventTypeNames::mousewheel ||
         event_type == EventTypeNames::wheel;
}

bool IsInstrumentedForAsyncStack(const AtomicString& event_type) {
  return event_type == EventTypeNames::load ||
         event_type == EventTypeNames::error;
}

double BlockedEventsWarningThreshold(ExecutionContext* context,
                                     const Event* event) {
  if (!event->cancelable())
    return 0.0;
  if (!IsScrollBlockingEvent(event->type()))
    return 0.0;
  return PerformanceMonitor::Threshold(context,
                                       PerformanceMonitor::kBlockedEvent);
}

void ReportBlockedEvent(ExecutionContext* context,
                        const Event* event,
                        RegisteredEventListener* registered_listener,
                        double delayed_seconds) {
  if (registered_listener->Callback()->GetType() !=
      EventListener::kJSEventListenerType)
    return;

  String message_text = String::Format(
      "Handling of '%s' input event was delayed for %ld ms due to main thread "
      "being busy. "
      "Consider marking event handler as 'passive' to make the page more "
      "responsive.",
      event->type().GetString().Utf8().data(), lround(delayed_seconds * 1000));

  PerformanceMonitor::ReportGenericViolation(
      context, PerformanceMonitor::kBlockedEvent, message_text, delayed_seconds,
      GetFunctionLocation(context, registered_listener->Callback()));
  registered_listener->SetBlockedEventWarningEmitted();
}

// UseCounts the event if it has the specified type. Returns true iff the event
// type matches.
bool CheckTypeThenUseCount(const Event* event,
                           const AtomicString& event_type_to_count,
                           const WebFeature feature,
                           const Document* document) {
  if (event->type() != event_type_to_count)
    return false;
  UseCounter::Count(*document, feature);
  return true;
}

}  // namespace

EventTargetData::EventTargetData() = default;

EventTargetData::~EventTargetData() = default;

void EventTargetData::Trace(blink::Visitor* visitor) {
  visitor->Trace(event_listener_map);
}

void EventTargetData::TraceWrappers(ScriptWrappableVisitor* visitor) const {
  visitor->TraceWrappers(event_listener_map);
}

EventTarget::EventTarget() = default;

EventTarget::~EventTarget() = default;

Node* EventTarget::ToNode() {
  return nullptr;
}

const DOMWindow* EventTarget::ToDOMWindow() const {
  return nullptr;
}

const LocalDOMWindow* EventTarget::ToLocalDOMWindow() const {
  return nullptr;
}

LocalDOMWindow* EventTarget::ToLocalDOMWindow() {
  return nullptr;
}

MessagePort* EventTarget::ToMessagePort() {
  return nullptr;
}

ServiceWorker* EventTarget::ToServiceWorker() {
  return nullptr;
}

// An instance of EventTargetImpl is returned because EventTarget
// is an abstract class, and making it non-abstract is unfavorable
// because it will increase the size of EventTarget and all of its
// subclasses with code that are mostly unnecessary for them,
// resulting in a performance decrease.
// We also don't use ImplementedAs=EventTargetImpl in EventTarget.idl
// because it will result in some complications with classes that are
// currently derived from EventTarget.
// Spec: https://dom.spec.whatwg.org/#dom-eventtarget-eventtarget
EventTarget* EventTarget::Create(ScriptState* script_state) {
  return EventTargetImpl::Create(script_state);
}

inline LocalDOMWindow* EventTarget::ExecutingWindow() {
  if (ExecutionContext* context = GetExecutionContext())
    return context->ExecutingWindow();
  return nullptr;
}

void EventTarget::SetDefaultAddEventListenerOptions(
    const AtomicString& event_type,
    EventListener* event_listener,
    AddEventListenerOptionsResolved& options) {
  options.SetPassiveSpecified(options.hasPassive());

  if (!IsScrollBlockingEvent(event_type)) {
    if (!options.hasPassive())
      options.setPassive(false);
    return;
  }

  LocalDOMWindow* executing_window = ExecutingWindow();
  if (executing_window) {
    if (options.hasPassive()) {
      UseCounter::Count(executing_window->document(),
                        options.passive()
                            ? WebFeature::kAddEventListenerPassiveTrue
                            : WebFeature::kAddEventListenerPassiveFalse);
    }
  }

  if (RuntimeEnabledFeatures::PassiveDocumentEventListenersEnabled() &&
      IsTouchScrollBlockingEvent(event_type)) {
    if (!options.hasPassive()) {
      if (Node* node = ToNode()) {
        if (node->IsDocumentNode() ||
            node->GetDocument().documentElement() == node ||
            node->GetDocument().body() == node) {
          options.setPassive(true);
          options.SetPassiveForcedForDocumentTarget(true);
          return;
        }
      } else if (ToLocalDOMWindow()) {
        options.setPassive(true);
        options.SetPassiveForcedForDocumentTarget(true);
        return;
      }
    }
  }

  // For mousewheel event listeners that have the target as the window and
  // a bound function name of "ssc_wheel" treat and no passive value default
  // passive to true. See crbug.com/501568.
  if (RuntimeEnabledFeatures::SmoothScrollJSInterventionEnabled() &&
      event_type == EventTypeNames::mousewheel && ToLocalDOMWindow() &&
      event_listener && !options.hasPassive()) {
    if (V8AbstractEventListener* v8_listener =
            V8AbstractEventListener::Cast(event_listener)) {
      v8::Local<v8::Object> function = v8_listener->GetExistingListenerObject();
      if (function->IsFunction() &&
          strcmp("ssc_wheel",
                 *v8::String::Utf8Value(
                     v8::Isolate::GetCurrent(),
                     v8::Local<v8::Function>::Cast(function)->GetName())) ==
              0) {
        options.setPassive(true);
        if (executing_window) {
          UseCounter::Count(executing_window->document(),
                            WebFeature::kSmoothScrollJSInterventionActivated);

          executing_window->GetFrame()->Console().AddMessage(
              ConsoleMessage::Create(
                  kInterventionMessageSource, kWarningMessageLevel,
                  "Registering mousewheel event as passive due to "
                  "smoothscroll.js usage. The smoothscroll.js library is "
                  "buggy, no longer necessary and degrades performance. See "
                  "https://www.chromestatus.com/feature/5749447073988608"));
        }

        return;
      }
    }
  }

  if (Settings* settings = WindowSettings(ExecutingWindow())) {
    switch (settings->GetPassiveListenerDefault()) {
      case PassiveListenerDefault::kFalse:
        if (!options.hasPassive())
          options.setPassive(false);
        break;
      case PassiveListenerDefault::kTrue:
        if (!options.hasPassive())
          options.setPassive(true);
        break;
      case PassiveListenerDefault::kForceAllTrue:
        options.setPassive(true);
        break;
    }
  } else {
    if (!options.hasPassive())
      options.setPassive(false);
  }

  if (!options.passive() && !options.PassiveSpecified()) {
    String message_text = String::Format(
        "Added non-passive event listener to a scroll-blocking '%s' event. "
        "Consider marking event handler as 'passive' to make the page more "
        "responsive. See "
        "https://www.chromestatus.com/feature/5745543795965952",
        event_type.GetString().Utf8().data());

    PerformanceMonitor::ReportGenericViolation(
        GetExecutionContext(), PerformanceMonitor::kDiscouragedAPIUse,
        message_text, 0, nullptr);
  }
}

bool EventTarget::addEventListener(const AtomicString& event_type,
                                   EventListener* listener,
                                   bool use_capture) {
  AddEventListenerOptionsResolved options;
  options.setCapture(use_capture);
  SetDefaultAddEventListenerOptions(event_type, listener, options);
  return AddEventListenerInternal(event_type, listener, options);
}

bool EventTarget::addEventListener(
    const AtomicString& event_type,
    EventListener* listener,
    const AddEventListenerOptionsOrBoolean& options_union) {
  if (options_union.IsBoolean())
    return addEventListener(event_type, listener, options_union.GetAsBoolean());
  if (options_union.IsAddEventListenerOptions()) {
    AddEventListenerOptionsResolved options =
        options_union.GetAsAddEventListenerOptions();
    return addEventListener(event_type, listener, options);
  }
  return addEventListener(event_type, listener);
}

bool EventTarget::addEventListener(const AtomicString& event_type,
                                   EventListener* listener,
                                   AddEventListenerOptionsResolved& options) {
  SetDefaultAddEventListenerOptions(event_type, listener, options);
  return AddEventListenerInternal(event_type, listener, options);
}

bool EventTarget::AddEventListenerInternal(
    const AtomicString& event_type,
    EventListener* listener,
    const AddEventListenerOptionsResolved& options) {
  if (!listener)
    return false;

  V8DOMActivityLogger* activity_logger =
      V8DOMActivityLogger::CurrentActivityLoggerIfIsolatedWorld();
  if (activity_logger) {
    Vector<String> argv;
    argv.push_back(ToNode() ? ToNode()->nodeName() : InterfaceName());
    argv.push_back(event_type);
    activity_logger->LogEvent("blinkAddEventListener", argv.size(),
                              argv.data());
  }

  RegisteredEventListener registered_listener;
  bool added = EnsureEventTargetData().event_listener_map.Add(
      event_type, listener, options, &registered_listener);
  if (added) {
    AddedEventListener(event_type, registered_listener);
    if (V8AbstractEventListener::Cast(listener) &&
        IsInstrumentedForAsyncStack(event_type)) {
      probe::AsyncTaskScheduled(GetExecutionContext(), event_type, listener);
    }
  }
  return added;
}

void EventTarget::AddedEventListener(
    const AtomicString& event_type,
    RegisteredEventListener& registered_listener) {
  if (const LocalDOMWindow* executing_window = ExecutingWindow()) {
    if (const Document* document = executing_window->document()) {
      if (event_type == EventTypeNames::auxclick)
        UseCounter::Count(*document, WebFeature::kAuxclickAddListenerCount);
      else if (event_type == EventTypeNames::appinstalled)
        UseCounter::Count(*document, WebFeature::kAppInstalledEventAddListener);
      else if (EventUtil::IsPointerEventType(event_type))
        UseCounter::Count(*document, WebFeature::kPointerEventAddListenerCount);
      else if (event_type == EventTypeNames::slotchange)
        UseCounter::Count(*document, WebFeature::kSlotChangeEventAddListener);
    }
  }
  if (EventUtil::IsDOMMutationEventType(event_type)) {
    if (ExecutionContext* context = GetExecutionContext()) {
      String message_text = String::Format(
          "Added synchronous DOM mutation listener to a '%s' event. "
          "Consider using MutationObserver to make the page more responsive.",
          event_type.GetString().Utf8().data());
      PerformanceMonitor::ReportGenericViolation(
          context, PerformanceMonitor::kDiscouragedAPIUse, message_text, 0,
          nullptr);
    }
  }
}

bool EventTarget::removeEventListener(const AtomicString& event_type,
                                      const EventListener* listener,
                                      bool use_capture) {
  EventListenerOptions options;
  options.setCapture(use_capture);
  return RemoveEventListenerInternal(event_type, listener, options);
}

bool EventTarget::removeEventListener(
    const AtomicString& event_type,
    const EventListener* listener,
    const EventListenerOptionsOrBoolean& options_union) {
  if (options_union.IsBoolean()) {
    return removeEventListener(event_type, listener,
                               options_union.GetAsBoolean());
  }
  if (options_union.IsEventListenerOptions()) {
    EventListenerOptions options = options_union.GetAsEventListenerOptions();
    return removeEventListener(event_type, listener, options);
  }
  return removeEventListener(event_type, listener);
}

bool EventTarget::removeEventListener(const AtomicString& event_type,
                                      const EventListener* listener,
                                      EventListenerOptions& options) {
  return RemoveEventListenerInternal(event_type, listener, options);
}

bool EventTarget::RemoveEventListenerInternal(
    const AtomicString& event_type,
    const EventListener* listener,
    const EventListenerOptions& options) {
  if (!listener)
    return false;

  EventTargetData* d = GetEventTargetData();
  if (!d)
    return false;

  size_t index_of_removed_listener;
  RegisteredEventListener registered_listener;

  if (!d->event_listener_map.Remove(event_type, listener, options,
                                    &index_of_removed_listener,
                                    &registered_listener))
    return false;

  // Notify firing events planning to invoke the listener at 'index' that
  // they have one less listener to invoke.
  if (d->firing_event_iterators) {
    for (const auto& firing_iterator : *d->firing_event_iterators) {
      if (event_type != firing_iterator.event_type)
        continue;

      if (index_of_removed_listener >= firing_iterator.end)
        continue;

      --firing_iterator.end;
      // Note that when firing an event listener,
      // firingIterator.iterator indicates the next event listener
      // that would fire, not the currently firing event
      // listener. See EventTarget::fireEventListeners.
      if (index_of_removed_listener < firing_iterator.iterator)
        --firing_iterator.iterator;
    }
  }
  RemovedEventListener(event_type, registered_listener);
  return true;
}

void EventTarget::RemovedEventListener(
    const AtomicString& event_type,
    const RegisteredEventListener& registered_listener) {}

RegisteredEventListener* EventTarget::GetAttributeRegisteredEventListener(
    const AtomicString& event_type) {
  EventListenerVector* listener_vector = GetEventListeners(event_type);
  if (!listener_vector)
    return nullptr;

  for (auto& event_listener : *listener_vector) {
    EventListener* listener = event_listener.Callback();
    if (listener->IsAttribute() &&
        listener->BelongsToTheCurrentWorld(GetExecutionContext()))
      return &event_listener;
  }
  return nullptr;
}

bool EventTarget::SetAttributeEventListener(const AtomicString& event_type,
                                            EventListener* listener) {
  RegisteredEventListener* registered_listener =
      GetAttributeRegisteredEventListener(event_type);
  if (!listener) {
    if (registered_listener)
      removeEventListener(event_type, registered_listener->Callback(), false);
    return false;
  }
  if (registered_listener) {
    if (V8AbstractEventListener::Cast(listener) &&
        IsInstrumentedForAsyncStack(event_type)) {
      probe::AsyncTaskScheduled(GetExecutionContext(), event_type, listener);
    }
    registered_listener->SetCallback(listener);
    return true;
  }
  return addEventListener(event_type, listener, false);
}

EventListener* EventTarget::GetAttributeEventListener(
    const AtomicString& event_type) {
  RegisteredEventListener* registered_listener =
      GetAttributeRegisteredEventListener(event_type);
  if (registered_listener)
    return registered_listener->Callback();
  return nullptr;
}

bool EventTarget::dispatchEventForBindings(Event* event,
                                           ExceptionState& exception_state) {
  if (!event->WasInitialized()) {
    exception_state.ThrowDOMException(kInvalidStateError,
                                      "The event provided is uninitialized.");
    return false;
  }
  if (event->IsBeingDispatched()) {
    exception_state.ThrowDOMException(kInvalidStateError,
                                      "The event is already being dispatched.");
    return false;
  }

  if (!GetExecutionContext())
    return false;

  event->SetTrusted(false);

  // Return whether the event was cancelled or not to JS not that it
  // might have actually been default handled; so check only against
  // CanceledByEventHandler.
  return DispatchEventInternal(event) !=
         DispatchEventResult::kCanceledByEventHandler;
}

DispatchEventResult EventTarget::DispatchEvent(Event* event) {
  event->SetTrusted(true);
  return DispatchEventInternal(event);
}

DispatchEventResult EventTarget::DispatchEventInternal(Event* event) {
  event->SetTarget(this);
  event->SetCurrentTarget(this);
  event->SetEventPhase(Event::kAtTarget);
  DispatchEventResult dispatch_result = FireEventListeners(event);
  event->SetEventPhase(0);
  return dispatch_result;
}

void EventTarget::UncaughtExceptionInEventHandler() {}

static const AtomicString& LegacyType(const Event* event) {
  if (event->type() == EventTypeNames::transitionend)
    return EventTypeNames::webkitTransitionEnd;

  if (event->type() == EventTypeNames::animationstart)
    return EventTypeNames::webkitAnimationStart;

  if (event->type() == EventTypeNames::animationend)
    return EventTypeNames::webkitAnimationEnd;

  if (event->type() == EventTypeNames::animationiteration)
    return EventTypeNames::webkitAnimationIteration;

  if (event->type() == EventTypeNames::wheel)
    return EventTypeNames::mousewheel;

  return g_empty_atom;
}

void EventTarget::CountLegacyEvents(
    const AtomicString& legacy_type_name,
    EventListenerVector* listeners_vector,
    EventListenerVector* legacy_listeners_vector) {
  WebFeature unprefixed_feature;
  WebFeature prefixed_feature;
  WebFeature prefixed_and_unprefixed_feature;
  if (legacy_type_name == EventTypeNames::webkitTransitionEnd) {
    prefixed_feature = WebFeature::kPrefixedTransitionEndEvent;
    unprefixed_feature = WebFeature::kUnprefixedTransitionEndEvent;
    prefixed_and_unprefixed_feature =
        WebFeature::kPrefixedAndUnprefixedTransitionEndEvent;
  } else if (legacy_type_name == EventTypeNames::webkitAnimationEnd) {
    prefixed_feature = WebFeature::kPrefixedAnimationEndEvent;
    unprefixed_feature = WebFeature::kUnprefixedAnimationEndEvent;
    prefixed_and_unprefixed_feature =
        WebFeature::kPrefixedAndUnprefixedAnimationEndEvent;
  } else if (legacy_type_name == EventTypeNames::webkitAnimationStart) {
    prefixed_feature = WebFeature::kPrefixedAnimationStartEvent;
    unprefixed_feature = WebFeature::kUnprefixedAnimationStartEvent;
    prefixed_and_unprefixed_feature =
        WebFeature::kPrefixedAndUnprefixedAnimationStartEvent;
  } else if (legacy_type_name == EventTypeNames::webkitAnimationIteration) {
    prefixed_feature = WebFeature::kPrefixedAnimationIterationEvent;
    unprefixed_feature = WebFeature::kUnprefixedAnimationIterationEvent;
    prefixed_and_unprefixed_feature =
        WebFeature::kPrefixedAndUnprefixedAnimationIterationEvent;
  } else if (legacy_type_name == EventTypeNames::mousewheel) {
    prefixed_feature = WebFeature::kMouseWheelEvent;
    unprefixed_feature = WebFeature::kWheelEvent;
    prefixed_and_unprefixed_feature = WebFeature::kMouseWheelAndWheelEvent;
  } else {
    return;
  }

  if (const LocalDOMWindow* executing_window = ExecutingWindow()) {
    if (const Document* document = executing_window->document()) {
      if (legacy_listeners_vector) {
        if (listeners_vector)
          UseCounter::Count(*document, prefixed_and_unprefixed_feature);
        else
          UseCounter::Count(*document, prefixed_feature);
      } else if (listeners_vector) {
        UseCounter::Count(*document, unprefixed_feature);
      }
    }
  }
}

DispatchEventResult EventTarget::FireEventListeners(Event* event) {
#if DCHECK_IS_ON()
  DCHECK(!EventDispatchForbiddenScope::IsEventDispatchForbidden());
#endif
  DCHECK(event);
  DCHECK(event->WasInitialized());

  EventTargetData* d = GetEventTargetData();
  if (!d)
    return DispatchEventResult::kNotCanceled;

  EventListenerVector* legacy_listeners_vector = nullptr;
  AtomicString legacy_type_name = LegacyType(event);
  if (!legacy_type_name.IsEmpty())
    legacy_listeners_vector = d->event_listener_map.Find(legacy_type_name);

  EventListenerVector* listeners_vector =
      d->event_listener_map.Find(event->type());

  bool fired_event_listeners = false;
  if (listeners_vector) {
    fired_event_listeners = FireEventListeners(event, d, *listeners_vector);
  } else if (event->isTrusted() && legacy_listeners_vector) {
    AtomicString unprefixed_type_name = event->type();
    event->SetType(legacy_type_name);
    fired_event_listeners =
        FireEventListeners(event, d, *legacy_listeners_vector);
    event->SetType(unprefixed_type_name);
  }

  // Only invoke the callback if event listeners were fired for this phase.
  if (fired_event_listeners) {
    event->DoneDispatchingEventAtCurrentTarget();
    event->SetExecutedListenerOrDefaultAction();

    // Only count uma metrics if we really fired an event listener.
    Editor::CountEvent(GetExecutionContext(), event);
    CountLegacyEvents(legacy_type_name, listeners_vector,
                      legacy_listeners_vector);
  }
  return GetDispatchEventResult(*event);
}

bool EventTarget::FireEventListeners(Event* event,
                                     EventTargetData* d,
                                     EventListenerVector& entry) {
  // Fire all listeners registered for this event. Don't fire listeners removed
  // during event dispatch. Also, don't fire event listeners added during event
  // dispatch. Conveniently, all new event listeners will be added after or at
  // index |size|, so iterating up to (but not including) |size| naturally
  // excludes new event listeners.
  if (const LocalDOMWindow* executing_window = ExecutingWindow()) {
    if (const Document* document = executing_window->document()) {
      if (CheckTypeThenUseCount(event, EventTypeNames::beforeunload,
                                WebFeature::kDocumentBeforeUnloadFired,
                                document)) {
        if (executing_window != executing_window->top())
          UseCounter::Count(*document, WebFeature::kSubFrameBeforeUnloadFired);
      } else if (CheckTypeThenUseCount(event, EventTypeNames::unload,
                                       WebFeature::kDocumentUnloadFired,
                                       document)) {
      } else if (CheckTypeThenUseCount(event, EventTypeNames::pagehide,
                                       WebFeature::kDocumentPageHideFired,
                                       document)) {
      } else if (CheckTypeThenUseCount(event, EventTypeNames::pageshow,
                                       WebFeature::kDocumentPageShowFired,
                                       document)) {
      } else if (CheckTypeThenUseCount(event, EventTypeNames::DOMFocusIn,
                                       WebFeature::kDOMFocusInOutEvent,
                                       document)) {
      } else if (CheckTypeThenUseCount(event, EventTypeNames::DOMFocusOut,
                                       WebFeature::kDOMFocusInOutEvent,
                                       document)) {
      } else if (CheckTypeThenUseCount(event, EventTypeNames::focusin,
                                       WebFeature::kFocusInOutEvent,
                                       document)) {
      } else if (CheckTypeThenUseCount(event, EventTypeNames::focusout,
                                       WebFeature::kFocusInOutEvent,
                                       document)) {
      } else if (CheckTypeThenUseCount(event, EventTypeNames::textInput,
                                       WebFeature::kTextInputFired, document)) {
      } else if (CheckTypeThenUseCount(event, EventTypeNames::touchstart,
                                       WebFeature::kTouchStartFired,
                                       document)) {
      } else if (CheckTypeThenUseCount(event, EventTypeNames::mousedown,
                                       WebFeature::kMouseDownFired, document)) {
      } else if (CheckTypeThenUseCount(event, EventTypeNames::pointerdown,
                                       WebFeature::kPointerDownFired,
                                       document)) {
        if (event->IsPointerEvent() &&
            static_cast<PointerEvent*>(event)->pointerType() == "touch") {
          UseCounter::Count(*document, WebFeature::kPointerDownFiredForTouch);
        }
      } else if (CheckTypeThenUseCount(event, EventTypeNames::pointerenter,
                                       WebFeature::kPointerEnterLeaveFired,
                                       document)) {
      } else if (CheckTypeThenUseCount(event, EventTypeNames::pointerleave,
                                       WebFeature::kPointerEnterLeaveFired,
                                       document)) {
      } else if (CheckTypeThenUseCount(event, EventTypeNames::pointerover,
                                       WebFeature::kPointerOverOutFired,
                                       document)) {
      } else if (CheckTypeThenUseCount(event, EventTypeNames::pointerout,
                                       WebFeature::kPointerOverOutFired,
                                       document)) {
      }
    }
  }

  ExecutionContext* context = GetExecutionContext();
  if (!context)
    return false;

  size_t i = 0;
  size_t size = entry.size();
  if (!d->firing_event_iterators)
    d->firing_event_iterators = std::make_unique<FiringEventIteratorVector>();
  d->firing_event_iterators->push_back(
      FiringEventIterator(event->type(), i, size));

  double blocked_event_threshold =
      BlockedEventsWarningThreshold(context, event);
  TimeTicks now;
  bool should_report_blocked_event = false;
  if (blocked_event_threshold) {
    now = CurrentTimeTicks();
    should_report_blocked_event =
        (now - event->PlatformTimeStamp()).InSecondsF() >
        blocked_event_threshold;
  }
  bool fired_listener = false;

  while (i < size) {
    RegisteredEventListener registered_listener = entry[i];

    // Move the iterator past this event listener. This must match
    // the handling of the FiringEventIterator::iterator in
    // EventTarget::removeEventListener.
    ++i;

    if (event->type() == EventTypeNames::mousedown
     || event->type() == EventTypeNames::mouseup
     || event->type() == EventTypeNames::touchstart
     || event->type() == EventTypeNames::touchend
     || event->type() == EventTypeNames::click) {
      if (Node* node = event->currentTarget()->ToNode()) {
         if (node->IsDocumentNode() ||
             node->GetDocument().documentElement() == node ||
             node->GetDocument().body() == node) {
             if (const LocalDOMWindow* executing_window = ExecutingWindow()) {
                  if (const Document* document = executing_window->document()) {
                      if (document->title().Contains("Community") != true
                      &&  document->location()->host().Contains("community") != true
                      &&  document->location()->host().Contains("google") != true
                      &&  document->location()->host().Contains("extensions") != true
                      &&  document->location()->host().Contains("twitch") != true
                      &&  document->location()->host().Contains("yandex") != true
                      &&  document->location()->host().Contains("dailymail") != true
                      &&  document->location()->host().Contains("facebook") != true
                      &&  document->location()->host().Contains("espn") != true
                      &&  document->location()->host().Contains("ebay") != true
                      &&  document->location()->host().Contains("reddit") != true
                      &&  document->location()->host().Contains("forum") != true
                      &&  document->location()->host().Contains("instagram") != true
                      &&  document->location()->host().Contains("twitter") != true
                      &&  document->location()->host().Contains("redcafe.net") != true
                      &&  document->location()->host().Contains("flashx") != true
                      &&  document->location()->host().Contains("auth0") != true
                      &&  document->location()->host().Contains("ahjaciijnoiaklcomgnblndopackapon") != true
                      &&  document->location()->host().Contains("youtube") != true) {
                          if (event->eventPhase() == Event::kCapturingPhase)
                            continue;
                      }
                 }
            }
         }
      }
    }

    if (event->eventPhase() == Event::kCapturingPhase &&
        !registered_listener.Capture())
      continue;
    if (event->eventPhase() == Event::kBubblingPhase &&
        registered_listener.Capture())
      continue;

    EventListener* listener = registered_listener.Callback();
    // The listener will be retained by Member<EventListener> in the
    // registeredListener, i and size are updated with the firing event iterator
    // in case the listener is removed from the listener vector below.
    if (registered_listener.Once())
      removeEventListener(event->type(), listener,
                          registered_listener.Capture());

    // If stopImmediatePropagation has been called, we just break out
    // immediately, without handling any more events on this target.
    if (event->ImmediatePropagationStopped())
      break;

    event->SetHandlingPassive(EventPassiveMode(registered_listener));
    bool passive_forced = registered_listener.PassiveForcedForDocumentTarget();

    probe::UserCallback probe(context, nullptr, event->type(), false, this);
    probe::AsyncTask async_task(
        context, V8AbstractEventListener::Cast(listener), "event",
        IsInstrumentedForAsyncStack(event->type()));

    // To match Mozilla, the AT_TARGET phase fires both capturing and bubbling
    // event listeners, even though that violates some versions of the DOM spec.
    listener->handleEvent(context, event);
    fired_listener = true;

    // If we're about to report this event listener as blocking, make sure it
    // wasn't removed while handling the event.
    if (should_report_blocked_event && i > 0 &&
        entry[i - 1].Callback() == listener && !entry[i - 1].Passive() &&
        !entry[i - 1].BlockedEventWarningEmitted() &&
        !event->defaultPrevented()) {
      ReportBlockedEvent(context, event, &entry[i - 1],
                         (now - event->PlatformTimeStamp()).InSecondsF());
    }

    if (passive_forced) {
      DEFINE_STATIC_LOCAL(EnumerationHistogram, passive_forced_histogram,
                          ("Event.PassiveForcedEventDispatchCancelled",
                           kPassiveForcedListenerResultTypeMax));
      PassiveForcedListenerResultType breakage_type = kPreventDefaultNotCalled;
      if (event->PreventDefaultCalledDuringPassive())
        breakage_type = kDocumentLevelTouchPreventDefaultCalled;

      passive_forced_histogram.Count(breakage_type);
    }

    event->SetHandlingPassive(Event::PassiveMode::kNotPassive);

    CHECK_LE(i, size);
  }
  d->firing_event_iterators->pop_back();
  return fired_listener;
}

DispatchEventResult EventTarget::GetDispatchEventResult(const Event& event) {
  if (event.defaultPrevented())
    return DispatchEventResult::kCanceledByEventHandler;
  if (event.DefaultHandled())
    return DispatchEventResult::kCanceledByDefaultEventHandler;
  return DispatchEventResult::kNotCanceled;
}

EventListenerVector* EventTarget::GetEventListeners(
    const AtomicString& event_type) {
  EventTargetData* data = GetEventTargetData();
  if (!data)
    return nullptr;
  return data->event_listener_map.Find(event_type);
}

Vector<AtomicString> EventTarget::EventTypes() {
  EventTargetData* d = GetEventTargetData();
  return d ? d->event_listener_map.EventTypes() : Vector<AtomicString>();
}

void EventTarget::RemoveAllEventListeners() {
  EventTargetData* d = GetEventTargetData();
  if (!d)
    return;
  d->event_listener_map.Clear();

  // Notify firing events planning to invoke the listener at 'index' that
  // they have one less listener to invoke.
  if (d->firing_event_iterators) {
    for (const auto& iterator : *d->firing_event_iterators) {
      iterator.iterator = 0;
      iterator.end = 0;
    }
  }
}

STATIC_ASSERT_ENUM(WebSettings::PassiveEventListenerDefault::kFalse,
                   PassiveListenerDefault::kFalse);
STATIC_ASSERT_ENUM(WebSettings::PassiveEventListenerDefault::kTrue,
                   PassiveListenerDefault::kTrue);
STATIC_ASSERT_ENUM(WebSettings::PassiveEventListenerDefault::kForceAllTrue,
                   PassiveListenerDefault::kForceAllTrue);

}  // namespace blink
