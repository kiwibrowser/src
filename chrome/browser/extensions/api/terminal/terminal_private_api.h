// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_EXTENSIONS_API_TERMINAL_TERMINAL_PRIVATE_API_H_
#define CHROME_BROWSER_EXTENSIONS_API_TERMINAL_TERMINAL_PRIVATE_API_H_

#include <string>
#include <vector>

#include "extensions/browser/extension_function.h"

namespace extensions {

// Opens new terminal process. Returns the new terminal id.
class TerminalPrivateOpenTerminalProcessFunction
    : public UIThreadExtensionFunction {
 public:
  DECLARE_EXTENSION_FUNCTION("terminalPrivate.openTerminalProcess",
                             TERMINALPRIVATE_OPENTERMINALPROCESS)

  TerminalPrivateOpenTerminalProcessFunction();

 protected:
  ~TerminalPrivateOpenTerminalProcessFunction() override;

  ExtensionFunction::ResponseAction Run() override;

 private:
  using ProcessOutputCallback =
      base::Callback<void(int terminal_id,
                          const std::string& output_type,
                          const std::string& output)>;
  using OpenProcessCallback = base::Callback<void(int terminal_id)>;

  void OpenOnRegistryTaskRunner(const ProcessOutputCallback& output_callback,
                                const OpenProcessCallback& callback,
                                const std::vector<std::string>& arguments,
                                const std::string& user_id_hash);
  void RespondOnUIThread(int terminal_id);
};

// Send input to the terminal process specified by the terminal ID, which is set
// as an argument.
class TerminalPrivateSendInputFunction : public UIThreadExtensionFunction {
 public:
  DECLARE_EXTENSION_FUNCTION("terminalPrivate.sendInput",
                             TERMINALPRIVATE_SENDINPUT)

 protected:
  ~TerminalPrivateSendInputFunction() override;

  ExtensionFunction::ResponseAction Run() override;

 private:
  void SendInputOnRegistryTaskRunner(int terminal_id, const std::string& input);
  void RespondOnUIThread(bool success);
};

// Closes terminal process.
class TerminalPrivateCloseTerminalProcessFunction
    : public UIThreadExtensionFunction {
 public:
  DECLARE_EXTENSION_FUNCTION("terminalPrivate.closeTerminalProcess",
                             TERMINALPRIVATE_CLOSETERMINALPROCESS)

 protected:
  ~TerminalPrivateCloseTerminalProcessFunction() override;

  ExtensionFunction::ResponseAction Run() override;

 private:
  void CloseOnRegistryTaskRunner(int terminal_id);
  void RespondOnUIThread(bool success);
};

// Called by extension when terminal size changes.
class TerminalPrivateOnTerminalResizeFunction
    : public UIThreadExtensionFunction {
 public:
  DECLARE_EXTENSION_FUNCTION("terminalPrivate.onTerminalResize",
                             TERMINALPRIVATE_ONTERMINALRESIZE)

 protected:
  ~TerminalPrivateOnTerminalResizeFunction() override;

  ExtensionFunction::ResponseAction Run() override;

 private:
  void OnResizeOnRegistryTaskRunner(int terminal_id, int width, int height);
  void RespondOnUIThread(bool success);
};

class TerminalPrivateAckOutputFunction : public UIThreadExtensionFunction {
 public:
  DECLARE_EXTENSION_FUNCTION("terminalPrivate.ackOutput",
                             TERMINALPRIVATE_ACKOUTPUT)

 protected:
  ~TerminalPrivateAckOutputFunction() override;

  ExtensionFunction::ResponseAction Run() override;

 private:
  void AckOutputOnRegistryTaskRunner(int terminal_id);
};

}  // namespace extensions

#endif  // CHROME_BROWSER_EXTENSIONS_API_TERMINAL_TERMINAL_PRIVATE_API_H_
