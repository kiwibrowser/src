// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef EXTENSIONS_RENDERER_BINDINGS_API_EVENT_LISTENERS_H_
#define EXTENSIONS_RENDERER_BINDINGS_API_EVENT_LISTENERS_H_

#include <string>
#include <vector>

#include "base/callback.h"
#include "base/macros.h"
#include "extensions/common/value_counter.h"
#include "extensions/renderer/bindings/api_binding_types.h"
#include "v8/include/v8.h"

namespace base {
class DictionaryValue;
}

namespace extensions {
class EventFilter;
struct EventFilteringInfo;

// A base class to hold listeners for a given event. This allows for adding,
// removing, and querying listeners in the list, and calling a callback when
// transitioning from 0 -> 1 or 1 -> 0 listeners.
class APIEventListeners {
 public:
  // The callback called when listeners change. |update_lazy_listeners|
  // indicates that the lazy listener count for the event should potentially be
  // updated. This is true if a) the event supports lazy listeners and b) the
  // change was "manual" (i.e., triggered by a direct call from the extension
  // rather than something like the context being destroyed).
  using ListenersUpdated =
      base::Callback<void(binding::EventListenersChanged,
                          const base::DictionaryValue* filter,
                          bool update_lazy_listeners,
                          v8::Local<v8::Context> context)>;

  virtual ~APIEventListeners() = default;

  // Adds the given |listener| to the list, possibly associating it with the
  // given |filter|. Returns true if the listener is added. Populates |error|
  // with any errors encountered. Note that |error| is *not* always populated
  // if false is returned, since we don't consider trying to re-add a listener
  // to be an error.
  virtual bool AddListener(v8::Local<v8::Function> listener,
                           v8::Local<v8::Object> filter,
                           v8::Local<v8::Context> context,
                           std::string* error) = 0;

  // Removes the given |listener|, if it's present in the list.
  virtual void RemoveListener(v8::Local<v8::Function> listener,
                              v8::Local<v8::Context> context) = 0;

  // Returns true if the given |listener| is in the list.
  virtual bool HasListener(v8::Local<v8::Function> listener) = 0;

  // Returns the number of listeners in the list.
  virtual size_t GetNumListeners() = 0;

  // Returns the listeners that should be notified for the given |filter|.
  virtual std::vector<v8::Local<v8::Function>> GetListeners(
      const EventFilteringInfo* filter,
      v8::Local<v8::Context> context) = 0;

  // Invalidates the list.
  virtual void Invalidate(v8::Local<v8::Context> context) = 0;

 protected:
  APIEventListeners() {}

 private:
  DISALLOW_COPY_AND_ASSIGN(APIEventListeners);
};

// A listener list implementation that doesn't support filtering. Each event
// dispatched is dispatched to all the associated listeners.
class UnfilteredEventListeners final : public APIEventListeners {
 public:
  UnfilteredEventListeners(const ListenersUpdated& listeners_updated,
                           int max_listeners,
                           bool supports_lazy_listeners);
  ~UnfilteredEventListeners() override;

  bool AddListener(v8::Local<v8::Function> listener,
                   v8::Local<v8::Object> filter,
                   v8::Local<v8::Context> context,
                   std::string* error) override;
  void RemoveListener(v8::Local<v8::Function> listener,
                      v8::Local<v8::Context> context) override;
  bool HasListener(v8::Local<v8::Function> listener) override;
  size_t GetNumListeners() override;
  std::vector<v8::Local<v8::Function>> GetListeners(
      const EventFilteringInfo* filter,
      v8::Local<v8::Context> context) override;
  void Invalidate(v8::Local<v8::Context> context) override;

 private:
  // The event listeners associated with this event.
  // TODO(devlin): Having these listeners held as v8::Globals means that we
  // need to worry about cycles when a listener holds a reference to the event,
  // e.g. EventEmitter -> Listener -> EventEmitter. Right now, we handle that by
  // requiring Invalidate() to be called, but that means that events that aren't
  // Invalidate()'d earlier can leak until context destruction. We could
  // circumvent this by storing the listeners strongly in a private propery
  // (thus traceable by v8), and optionally keep a weak cache on this object.
  std::vector<v8::Global<v8::Function>> listeners_;

  ListenersUpdated listeners_updated_;

  // The maximum number of supported listeners.
  int max_listeners_;

  // Whether the event supports lazy listeners.
  bool supports_lazy_listeners_;

  DISALLOW_COPY_AND_ASSIGN(UnfilteredEventListeners);
};

// A listener list implementation that supports filtering. Events should only
// be dispatched to those listeners whose filters match. Additionally, the
// updated callback is triggered any time a listener with a new filter is
// added, or the last listener with a given filter is removed.
class FilteredEventListeners final : public APIEventListeners {
 public:
  FilteredEventListeners(const ListenersUpdated& listeners_updated,
                         const std::string& event_name,
                         int max_listeners,
                         bool supports_lazy_listeners,
                         EventFilter* event_filter);
  ~FilteredEventListeners() override;

  bool AddListener(v8::Local<v8::Function> listener,
                   v8::Local<v8::Object> filter,
                   v8::Local<v8::Context> context,
                   std::string* error) override;
  void RemoveListener(v8::Local<v8::Function> listener,
                      v8::Local<v8::Context> context) override;
  bool HasListener(v8::Local<v8::Function> listener) override;
  size_t GetNumListeners() override;
  std::vector<v8::Local<v8::Function>> GetListeners(
      const EventFilteringInfo* filter,
      v8::Local<v8::Context> context) override;
  void Invalidate(v8::Local<v8::Context> context) override;

 private:
  struct ListenerData;

  void InvalidateListener(const ListenerData& listener,
                          bool was_manual,
                          v8::Local<v8::Context> context);

  // Note: See TODO on UnfilteredEventListeners::listeners_.
  std::vector<ListenerData> listeners_;

  ListenersUpdated listeners_updated_;

  std::string event_name_;

  // The maximum number of supported listeners.
  int max_listeners_;

  // Whether the event supports lazy listeners.
  bool supports_lazy_listeners_;

  // The associated EventFilter; required to outlive this object.
  EventFilter* event_filter_ = nullptr;

  ValueCounter value_counter_;

  DISALLOW_COPY_AND_ASSIGN(FilteredEventListeners);
};

}  // namespace extensions

#endif  // EXTENSIONS_RENDERER_BINDINGS_API_EVENT_LISTENERS_H_
