// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef EXTENSIONS_RENDERER_MESSAGING_BINDINGS_H_
#define EXTENSIONS_RENDERER_MESSAGING_BINDINGS_H_

#include <string>

#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "extensions/renderer/object_backed_native_handler.h"

namespace extensions {
class ExtensionPort;
class ScriptContext;
struct PortId;

// Manually implements JavaScript bindings for extension messaging.
class MessagingBindings : public ObjectBackedNativeHandler {
 public:
  explicit MessagingBindings(ScriptContext* script_context);
  ~MessagingBindings() override;

  // ObjectBackedNativeHandler:
  void AddRoutes() override;

  // Returns the MessagingBindings associated with the given |context|.
  static MessagingBindings* ForContext(ScriptContext* context);

  // Returns an existing port with the given |id|, or null.
  ExtensionPort* GetPortWithId(const PortId& id);

  // Creates a new port with the given |id|. MessagingBindings owns the
  // returned port.
  ExtensionPort* CreateNewPortWithId(const PortId& id);

 private:
  using PortMap = std::map<int, std::unique_ptr<ExtensionPort>>;

  // JS Exposed Function: Sends a message along the given channel. If an error
  // occurs, returns the error, else returns nothing.
  void PostMessage(const v8::FunctionCallbackInfo<v8::Value>& args);

  // JS Exposed Function: Close a port, optionally forcefully (i.e. close the
  // whole channel instead of just the given port).
  void CloseChannel(const v8::FunctionCallbackInfo<v8::Value>& args);

  // JS Exposed Function: Binds |callback| to be invoked *sometime after*
  // |object| is garbage collected. We don't call the method re-entrantly so as
  // to avoid executing JS in some bizarro undefined mid-GC state, nor do we
  // then call into the script context if it's been invalidated.
  void BindToGC(const v8::FunctionCallbackInfo<v8::Value>& args);

  // JS Exposed Function: Opens a new channel to an extension.
  void OpenChannelToExtension(const v8::FunctionCallbackInfo<v8::Value>& args);

  // JS Exposed Function: Opens a new channel to a native application.
  void OpenChannelToNativeApp(const v8::FunctionCallbackInfo<v8::Value>& args);

  // JS Exposed Function: Opens a new channel to a tab.
  void OpenChannelToTab(const v8::FunctionCallbackInfo<v8::Value>& args);

  // Helper function to close a port. See CloseChannel() for |force_close|
  // documentation.
  void ClosePort(int local_port_id, bool force_close);

  int GetNextJsId();

  // Active ports, mapped by local port id.
  PortMap ports_;

  // The next available js id for a port.
  size_t next_js_id_ = 0;

  // The number of extension ports created.
  size_t num_extension_ports_ = 0;

  base::WeakPtrFactory<MessagingBindings> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(MessagingBindings);
};

}  // namespace extensions

#endif  // EXTENSIONS_RENDERER_MESSAGING_BINDINGS_H_
