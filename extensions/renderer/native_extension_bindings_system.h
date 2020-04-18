// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef EXTENSIONS_RENDERER_NATIVE_EXTENSION_BINDINGS_SYSTEM_H_
#define EXTENSIONS_RENDERER_NATIVE_EXTENSION_BINDINGS_SYSTEM_H_

#include <memory>
#include <string>

#include "base/memory/weak_ptr.h"
#include "extensions/renderer/bindings/api_binding_types.h"
#include "extensions/renderer/bindings/api_bindings_system.h"
#include "extensions/renderer/bindings/event_emitter.h"
#include "extensions/renderer/extension_bindings_system.h"
#include "extensions/renderer/feature_cache.h"
#include "extensions/renderer/native_renderer_messaging_service.h"
#include "v8/include/v8.h"

namespace extensions {
class IPCMessageSender;
class ScriptContext;

// The implementation of the Bindings System for extensions code with native
// implementations (rather than JS hooks). Handles permission/availability
// checking and creates all bindings available for a given context. Sending the
// IPC is still abstracted out for testing purposes, but otherwise this should
// be a plug-and-play version for use in the Extensions system.
// Designed to be used in a single thread, but for all contexts on that thread.
class NativeExtensionBindingsSystem : public ExtensionBindingsSystem {
 public:
  explicit NativeExtensionBindingsSystem(
      std::unique_ptr<IPCMessageSender> ipc_message_sender);
  ~NativeExtensionBindingsSystem() override;

  // ExtensionBindingsSystem:
  void DidCreateScriptContext(ScriptContext* context) override;
  void WillReleaseScriptContext(ScriptContext* context) override;
  void UpdateBindingsForContext(ScriptContext* context) override;
  void DispatchEventInContext(const std::string& event_name,
                              const base::ListValue* event_args,
                              const EventFilteringInfo* filtering_info,
                              ScriptContext* context) override;
  bool HasEventListenerInContext(const std::string& event_name,
                                 ScriptContext* context) override;
  void HandleResponse(int request_id,
                      bool success,
                      const base::ListValue& response,
                      const std::string& error) override;
  RequestSender* GetRequestSender() override;
  IPCMessageSender* GetIPCMessageSender() override;
  RendererMessagingService* GetMessagingService() override;
  void OnExtensionPermissionsUpdated(const ExtensionId& id) override;
  void OnExtensionRemoved(const ExtensionId& id) override;

  APIBindingsSystem* api_system() { return &api_system_; }
  NativeRendererMessagingService* messaging_service() {
    return &messaging_service_;
  }

  // Returns the API with the given |name| for the given |context|. Used for
  // testing purposes.
  v8::Local<v8::Object> GetAPIObjectForTesting(ScriptContext* context,
                                               const std::string& api_name);

 private:
  // Handles sending a given |request|, forwarding it on to the send_ipc_ after
  // adding additional info.
  void SendRequest(std::unique_ptr<APIRequestHandler::Request> request,
                   v8::Local<v8::Context> context);

  // Called when listeners for a given event have changed, and forwards it along
  // to |send_event_listener_ipc_|.
  void OnEventListenerChanged(const std::string& event_name,
                              binding::EventListenersChanged change,
                              const base::DictionaryValue* filter,
                              bool was_manual,
                              v8::Local<v8::Context> context);

  // Getter callback for an extension API, since APIs are constructed lazily.
  static void BindingAccessor(v8::Local<v8::Name> name,
                              const v8::PropertyCallbackInfo<v8::Value>& info);

  // Creates and returns the API binding for the given |name|.
  static v8::Local<v8::Object> GetAPIHelper(v8::Local<v8::Context> context,
                                            v8::Local<v8::String> name);

  // Gets the chrome.runtime API binding.
  static v8::Local<v8::Object> GetLastErrorParents(
      v8::Local<v8::Context> context,
      v8::Local<v8::Object>* secondary_parent);

  // Callback to get an API binding for an internal API.
  static void GetInternalAPI(const v8::FunctionCallbackInfo<v8::Value>& info);

  // Helper method to get a APIBindingJSUtil object for the current context,
  // and populate |binding_util_out|. We use an out parameter instead of
  // returning it in order to let us use weak ptrs, which can't be used on a
  // method with a return value.
  void GetJSBindingUtil(v8::Local<v8::Context> context,
                        v8::Local<v8::Value>* binding_util_out);

  std::unique_ptr<IPCMessageSender> ipc_message_sender_;

  // The APIBindingsSystem associated with this class.
  APIBindingsSystem api_system_;

  NativeRendererMessagingService messaging_service_;

  FeatureCache feature_cache_;

  // A function to acquire an internal API.
  v8::Eternal<v8::FunctionTemplate> get_internal_api_;

  base::WeakPtrFactory<NativeExtensionBindingsSystem> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(NativeExtensionBindingsSystem);
};

}  // namespace extensions

#endif  // EXTENSIONS_RENDERER_NATIVE_EXTENSION_BINDINGS_SYSTEM_H_
