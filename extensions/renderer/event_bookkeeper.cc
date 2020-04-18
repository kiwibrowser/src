// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "extensions/renderer/event_bookkeeper.h"

#include "base/lazy_instance.h"
#include "base/values.h"
#include "components/crx_file/id_util.h"
#include "extensions/common/constants.h"
#include "extensions/common/value_counter.h"
#include "extensions/renderer/script_context.h"
#include "extensions/renderer/worker_thread_dispatcher.h"

namespace extensions {

namespace {

base::LazyInstance<EventBookkeeper>::DestructorAtExit
    g_main_thread_event_bookkeeper = LAZY_INSTANCE_INITIALIZER;

// Gets a unique string key identifier for a ScriptContext.
// TODO(kalman): Just use pointer equality...?
std::string GetKeyForScriptContext(ScriptContext* script_context) {
  const std::string& extension_id = script_context->GetExtensionID();
  CHECK(crx_file::id_util::IdIsValid(extension_id) ||
        script_context->url().is_valid());
  return crx_file::id_util::IdIsValid(extension_id)
             ? extension_id
             : script_context->url().spec();
}

}  // namespace

EventBookkeeper::~EventBookkeeper() {}
EventBookkeeper::EventBookkeeper() {}

// static
EventBookkeeper* EventBookkeeper::Get() {
  if (content::WorkerThread::GetCurrentId() == kMainThreadId)
    return &g_main_thread_event_bookkeeper.Get();
  return WorkerThreadDispatcher::Get()->event_bookkeeper();
}

int EventBookkeeper::IncrementEventListenerCount(
    ScriptContext* script_context,
    const std::string& event_name) {
  return ++listener_counts_[GetKeyForScriptContext(script_context)][event_name];
}

int EventBookkeeper::DecrementEventListenerCount(
    ScriptContext* script_context,
    const std::string& event_name) {
  return --listener_counts_[GetKeyForScriptContext(script_context)][event_name];
}

bool EventBookkeeper::AddFilter(const std::string& event_name,
                                const ExtensionId& extension_id,
                                const base::DictionaryValue& filter) {
  FilteredEventListenerKey key(extension_id, event_name);
  FilteredEventListenerCounts::const_iterator counts =
      filtered_listener_counts_.find(key);
  if (counts == filtered_listener_counts_.end()) {
    counts =
        filtered_listener_counts_.emplace(key, std::make_unique<ValueCounter>())
            .first;
  }
  return counts->second->Add(filter);
}

bool EventBookkeeper::RemoveFilter(const std::string& event_name,
                                   const ExtensionId& extension_id,
                                   base::DictionaryValue* filter) {
  FilteredEventListenerKey key(extension_id, event_name);
  FilteredEventListenerCounts::const_iterator counts =
      filtered_listener_counts_.find(key);
  if (counts == filtered_listener_counts_.end())
    return false;
  // Note: Remove() returns true if it removed the last filter equivalent to
  // |filter|. If there are more equivalent filters, or if there weren't any in
  // the first place, it returns false.
  if (counts->second->Remove(*filter)) {
    if (counts->second->is_empty()) {
      // Clean up if there are no more filters.
      filtered_listener_counts_.erase(counts);
    }
    return true;
  }
  return false;
}

bool EventBookkeeper::HasListener(ScriptContext* script_context,
                                  const std::string& event_name) {
  // Unmanaged event listeners.
  auto unmanaged_iter = unmanaged_listeners_.find(script_context);
  if (unmanaged_iter != unmanaged_listeners_.end() &&
      base::ContainsKey(unmanaged_iter->second, event_name)) {
    return true;
  }
  // Managed event listeners.
  auto managed_iter =
      listener_counts_.find(GetKeyForScriptContext(script_context));
  if (managed_iter != listener_counts_.end()) {
    auto event_iter = managed_iter->second.find(event_name);
    if (event_iter != managed_iter->second.end() && event_iter->second > 0)
      return true;
  }
  return false;
}

void EventBookkeeper::AddUnmanagedEvent(ScriptContext* context,
                                        const std::string& event_name) {
  unmanaged_listeners_[context].insert(event_name);
}

void EventBookkeeper::RemoveUnmanagedEvent(ScriptContext* context,
                                           const std::string& event_name) {
  unmanaged_listeners_[context].erase(event_name);
}

void EventBookkeeper::RemoveAllUnmanagedListeners(ScriptContext* context) {
  unmanaged_listeners_.erase(context);
}

}  // namespace extensions
