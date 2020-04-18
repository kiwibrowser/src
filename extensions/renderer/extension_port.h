// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef EXTENSIONS_RENDERER_EXTENSION_PORT_H_
#define EXTENSIONS_RENDERER_EXTENSION_PORT_H_

#include <memory>
#include <utility>
#include <vector>

#include "base/macros.h"
#include "extensions/common/api/messaging/port_id.h"

namespace extensions {
struct Message;
struct PortId;
class ScriptContext;

// A class representing information about a specific extension message port that
// handles sending related IPCs to the browser. This consists of a port id and
// a separate js_id which is exposed only to the JavaScript context.
class ExtensionPort {
 public:
  ExtensionPort(ScriptContext* script_context, const PortId& id, int js_id);
  ~ExtensionPort();

  // Posts a new message to the port. If the port is not initialized, the
  // message will be queued until it is.
  void PostExtensionMessage(std::unique_ptr<Message> message);

  // Closes the port. If there are pending messages, they will still be sent
  // assuming initialization completes (after which, the port will close).
  void Close(bool close_channel);

  const PortId& id() const { return id_; }
  int js_id() const { return js_id_; }

 private:
  // The associated ScriptContext for this port. Since these objects are owned
  // by a NativeHandler, this should always be valid.
  ScriptContext* script_context_ = nullptr;

  const PortId id_;

  // The id used in the JS bindings. If this is a receiver port, this is not the
  // same as the port_number in the PortId. This should be used only as an
  // identifier in the JS context this port is from.
  int js_id_ = 0;

  DISALLOW_COPY_AND_ASSIGN(ExtensionPort);
};

}  // namespace extensions

#endif  // EXTENSIONS_RENDERER_EXTENSION_PORT_H_
