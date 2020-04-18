// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_RENDERER_EXTENSIONS_APP_BINDINGS_CORE_H_
#define CHROME_RENDERER_EXTENSIONS_APP_BINDINGS_CORE_H_

#include <map>

#include "base/callback.h"
#include "base/macros.h"
#include "chrome/renderer/extensions/chrome_v8_extension_handler.h"

namespace extensions {
class Dispatcher;
class ScriptContext;

// The common implementation for the chrome.app API, shared between native and
// js-based bindings.
// TODO(devlin): Add metrics to this and see if we can delete it. It's
// undocumented and very strange and inconsistent.
class AppBindingsCore : public ChromeV8ExtensionHandler {
 public:
  using GetInstallStateCallback = base::OnceCallback<void(const std::string&)>;

  explicit AppBindingsCore(Dispatcher* dispatcher);
  ~AppBindingsCore() override;

  // Total misnomer. Returns true if there is a hosted app associated with
  // |script_context| active in this process. This naming is to match the
  // chrome.app function it implements.
  bool GetIsInstalled(ScriptContext* script_context) const;

  // Returns the manifest of the extension associated with the frame.
  v8::Local<v8::Value> GetDetails(ScriptContext* script_context) const;

  // Determines the install state for the extension associated with the frame.
  // Note that this could be "disabled" for hosted apps when the user visits the
  // site but doesn't have the app enabled. Calls |callback| with the response.
  void GetInstallState(ScriptContext* script_context,
                       GetInstallStateCallback callback);

  // Returns the "running state" (one of running, cannot run, and ready to run)
  // for the extension associated with the frame of the script context.
  const char* GetRunningState(ScriptContext* script_context) const;

 private:
  // IPC::Listener:
  bool OnMessageReceived(const IPC::Message& message) override;

  // Handle for ExtensionMsg_GetAppInstallStateResponse.
  void OnAppInstallStateResponse(const std::string& state, int callback_id);

  // Dispatcher handle. Not owned.
  Dispatcher* dispatcher_ = nullptr;

  std::map<int, GetInstallStateCallback> callbacks_;

  int next_callback_id_ = 0;

  DISALLOW_COPY_AND_ASSIGN(AppBindingsCore);
};

}  // namespace extensions

#endif  // CHROME_RENDERER_EXTENSIONS_APP_BINDINGS_CORE_H_
