// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef EXTENSIONS_RENDERER_EVENT_BOOKKEEPER_H_
#define EXTENSIONS_RENDERER_EVENT_BOOKKEEPER_H_

#include <map>
#include <memory>
#include <set>
#include <string>

#include "base/macros.h"
#include "extensions/common/event_filter.h"
#include "extensions/common/extension_id.h"

namespace base {
class DictionaryValue;
}

namespace extensions {
class EventFilter;
class ScriptContext;
class ValueCounter;

// Class to manage thread (main or worker) specific event global data.
//
// TODO(lazyboy): This class could use an IPCMessageSender to also notify the
// browser/ about the changes in the event data it manages. Currently this is
// done from EventBindings, i.e. EventBindings updates/queries EventBookkeeper
// and then notifies the browser/.
// TODO(devlin/lazyboy): This class is only necessary with JS-based bindings.
// Remove it when Native bindings launch.
class EventBookkeeper {
 public:
  EventBookkeeper();
  ~EventBookkeeper();

  // Returns the instance of EventBookkeeper that belongs to current thread.
  static EventBookkeeper* Get();

  // Increments the number of event-listeners for the given |event_name| and
  // ScriptContext. Returns the count after the increment.
  int IncrementEventListenerCount(ScriptContext* script_context,
                                  const std::string& event_name);
  // Decrements the number of event-listeners for the given |event_name| and
  // ScriptContext. Returns the count after the decrement.
  int DecrementEventListenerCount(ScriptContext* script_context,
                                  const std::string& event_name);

  // Returns the EventFilter.
  EventFilter& event_filter() { return event_filter_; }

  // Adds a filter to |event_name| in |extension_id|, returning true if it
  // was the first filter for that event in that extension.
  bool AddFilter(const std::string& event_name,
                 const ExtensionId& extension_id,
                 const base::DictionaryValue& filter);
  // Removes a filter from |event_name| in |extension_id|, returning true if it
  // was the last filter for that event in that extension.
  bool RemoveFilter(const std::string& event_name,
                    const ExtensionId& extension_id,
                    base::DictionaryValue* filter);

  // Returns true if there is an event (managed or unmanaged) with the given
  // context and event name.
  bool HasListener(ScriptContext* script_context,
                   const std::string& event_name);

  void AddUnmanagedEvent(ScriptContext* context, const std::string& event_name);
  void RemoveUnmanagedEvent(ScriptContext* context,
                            const std::string& event_name);
  void RemoveAllUnmanagedListeners(ScriptContext* context);

 private:
  using EventName = std::string;

  // A map of event name to the number of contexts listening to that event.
  using EventListenerCounts = std::map<EventName, int>;
  // Maps context id -> (map of event name -> listener count).
  using AllCounts = std::map<std::string, EventListenerCounts>;

  using FilteredEventListenerKey = std::pair<ExtensionId, EventName>;
  // A map of <extension id, event name> pair to listener counts.
  using FilteredEventListenerCounts =
      std::map<FilteredEventListenerKey, std::unique_ptr<ValueCounter>>;

  // A map of context to name of events the context has.
  using UnmanagedListenerMap = std::map<ScriptContext*, std::set<std::string>>;

  // Used to notify the browser about event listeners when we transition between
  // 0 and 1.
  AllCounts listener_counts_;

  // The event filter.
  EventFilter event_filter_;

  // A map of (extension ID, event name) pairs to the filtered listener counts
  // for that pair. The map is used to keep track of which filters are in effect
  // for which events.  Used to notify the browser about filtered event
  // listeners when we transition between 0 and 1.
  FilteredEventListenerCounts filtered_listener_counts_;

  // A collection of the unmanaged events (i.e., those for which the browser is
  // not notified of changes) that have listeners, by context.
  UnmanagedListenerMap unmanaged_listeners_;

  DISALLOW_COPY_AND_ASSIGN(EventBookkeeper);
};

}  // namespace extensions

#endif  // EXTENSIONS_RENDERER_EVENT_BOOKKEEPER_H_
