// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef EXTENSIONS_RENDERER_EVENT_BINDINGS_H_
#define EXTENSIONS_RENDERER_EVENT_BINDINGS_H_

#include <memory>
#include <set>
#include <string>

#include "base/macros.h"
#include "extensions/renderer/object_backed_native_handler.h"
#include "v8/include/v8.h"

namespace base {
class ListValue;
}

namespace extensions {
class IPCMessageSender;
struct EventFilteringInfo;

// This class deals with the javascript bindings related to Event objects.
class EventBindings : public ObjectBackedNativeHandler {
 public:
  EventBindings(ScriptContext* context, IPCMessageSender* ipc_message_sender);
  ~EventBindings() override;

  // ObjectBackedNativeHandler:
  void AddRoutes() override;

  // Dispatches the event in the given |context| with the provided
  // |event_args| and |filtering_info|.
  static void DispatchEventInContext(const std::string& event_name,
                                     const base::ListValue* event_args,
                                     const EventFilteringInfo* filtering_info,
                                     ScriptContext* context);

 private:
  // JavaScript handler which forwards to AttachEvent().
  // args[0] forwards to |event_name|.
  void AttachEventHandler(const v8::FunctionCallbackInfo<v8::Value>& args);

  // Attach an event name to an object.
  // |event_name| The name of the event to attach.
  void AttachEvent(const std::string& event_name, bool supports_lazy_listeners);

  // JavaScript handler which forwards to DetachEvent().
  // args[0] forwards to |event_name|.
  // args[1] forwards to |is_manual|.
  void DetachEventHandler(const v8::FunctionCallbackInfo<v8::Value>& args);

  // Detaches an event name from an object.
  // |event_name| The name of the event to stop listening to.
  // |is_manual| True if this detach was done by the user via removeListener()
  // as opposed to automatically during shutdown, in which case we should inform
  // the browser we are no longer interested in that event.
  void DetachEvent(const std::string& event_name, bool remove_lazy_listener);

  // MatcherID AttachFilteredEvent(string event_name, object filter)
  // |event_name| Name of the event to attach.
  // |filter| Which instances of the named event are we interested in.
  // returns the id assigned to the listener, which will be returned from calls
  // to MatchAgainstEventFilter where this listener matches.
  void AttachFilteredEvent(const v8::FunctionCallbackInfo<v8::Value>& args);

  // JavaScript handler which forwards to DetachFilteredEvent.
  // void DetachFilteredEvent(int id, bool manual)
  // args[0] forwards to |matcher_id|
  // args[1] forwards to |is_manual|
  void DetachFilteredEventHandler(
      const v8::FunctionCallbackInfo<v8::Value>& args);

  // Detaches a filtered event. Unlike a normal event, a filtered event is
  // identified by a unique ID per filter, not its name.
  // |matcher_id| The ID of the filtered event.
  // |is_manual| false if this is part of the extension unload process where all
  // listeners are automatically detached.
  void DetachFilteredEvent(int matcher_id, bool remove_lazy_listener);

  void AttachUnmanagedEvent(const v8::FunctionCallbackInfo<v8::Value>& args);
  void DetachUnmanagedEvent(const v8::FunctionCallbackInfo<v8::Value>& args);

  // Called when our context, and therefore us, is invalidated. Run any cleanup.
  void OnInvalidated();

  // The associated message sender. Guaranteed to outlive this object.
  IPCMessageSender* const ipc_message_sender_;

  // The set of attached events and filtered events. Maintain these so that we
  // can detch them on unload.
  std::set<std::string> attached_event_names_;
  std::set<int> attached_matcher_ids_;

  DISALLOW_COPY_AND_ASSIGN(EventBindings);
};

}  // namespace extensions

#endif  // EXTENSIONS_RENDERER_EVENT_BINDINGS_H_
