// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Defines the Chrome Extensions Debugger API functions for attaching debugger
// to the page.

#ifndef CHROME_BROWSER_EXTENSIONS_API_DEBUGGER_DEBUGGER_API_H_
#define CHROME_BROWSER_EXTENSIONS_API_DEBUGGER_DEBUGGER_API_H_

#include <string>
#include <vector>

#include "chrome/browser/extensions/chrome_extension_function.h"
#include "chrome/common/extensions/api/debugger.h"
#include "content/public/browser/devtools_agent_host.h"

using extensions::api::debugger::Debuggee;

// Base debugger function.

namespace base {
class DictionaryValue;
}

namespace extensions {
class ExtensionDevToolsClientHost;

class DebuggerFunction : public ChromeAsyncExtensionFunction {
 protected:
  DebuggerFunction();
  ~DebuggerFunction() override;

  void FormatErrorMessage(const std::string& format);

  bool InitAgentHost();
  bool InitClientHost();
  ExtensionDevToolsClientHost* FindClientHost();

  Debuggee debuggee_;
  scoped_refptr<content::DevToolsAgentHost> agent_host_;
  ExtensionDevToolsClientHost* client_host_;
};

// Implements the debugger.attach() extension function.
class DebuggerAttachFunction : public DebuggerFunction {
 public:
  DECLARE_EXTENSION_FUNCTION("debugger.attach", DEBUGGER_ATTACH)

  DebuggerAttachFunction();

 protected:
  ~DebuggerAttachFunction() override;

  // ExtensionFunction:
  bool RunAsync() override;
};

// Implements the debugger.detach() extension function.
class DebuggerDetachFunction : public DebuggerFunction {
 public:
  DECLARE_EXTENSION_FUNCTION("debugger.detach", DEBUGGER_DETACH)

  DebuggerDetachFunction();

 protected:
  ~DebuggerDetachFunction() override;

  // ExtensionFunction:
  bool RunAsync() override;
};

// Implements the debugger.sendCommand() extension function.
class DebuggerSendCommandFunction : public DebuggerFunction {
 public:
  DECLARE_EXTENSION_FUNCTION("debugger.sendCommand", DEBUGGER_SENDCOMMAND)

  DebuggerSendCommandFunction();
  void SendResponseBody(base::DictionaryValue* result);
  void SendDetachedError();

 protected:
  ~DebuggerSendCommandFunction() override;

  // ExtensionFunction:
  bool RunAsync() override;
};

// Implements the debugger.getTargets() extension function.
class DebuggerGetTargetsFunction : public DebuggerFunction {
 public:
  DECLARE_EXTENSION_FUNCTION("debugger.getTargets", DEBUGGER_GETTARGETS)

  DebuggerGetTargetsFunction();

 protected:
  ~DebuggerGetTargetsFunction() override;

  // ExtensionFunction:
  bool RunAsync() override;

 private:
  void SendTargetList(const content::DevToolsAgentHost::List& target_list);
};

}  // namespace extensions

#endif  // CHROME_BROWSER_EXTENSIONS_API_DEBUGGER_DEBUGGER_API_H_
