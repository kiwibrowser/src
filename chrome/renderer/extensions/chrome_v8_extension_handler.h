// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_RENDERER_EXTENSIONS_CHROME_V8_EXTENSION_HANDLER_H_
#define CHROME_RENDERER_EXTENSIONS_CHROME_V8_EXTENSION_HANDLER_H_

#include <string>

#include "base/macros.h"
#include "ipc/ipc_listener.h"
#include "v8/include/v8.h"

namespace extensions {

// Base class for context-scoped handlers used with ChromeV8Extension.
// TODO(koz): Rename/refactor this somehow. Maybe just pull it into
// ChromeV8Extension.
class ChromeV8ExtensionHandler : public IPC::Listener {
 public:
  ~ChromeV8ExtensionHandler() override;

  // IPC::Listener
  bool OnMessageReceived(const IPC::Message& message) override = 0;

 protected:
  ChromeV8ExtensionHandler();
  int GetRoutingID();
  void Send(IPC::Message* message);

 private:
  int routing_id_;
  DISALLOW_COPY_AND_ASSIGN(ChromeV8ExtensionHandler);
};

}  // namespace extensions

#endif  // CHROME_RENDERER_EXTENSIONS_CHROME_V8_EXTENSION_HANDLER_H_
