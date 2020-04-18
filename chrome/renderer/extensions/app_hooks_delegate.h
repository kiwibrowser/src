// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_RENDERER_EXTENSIONS_APP_HOOKS_DELEGATE_H_
#define CHROME_RENDERER_EXTENSIONS_APP_HOOKS_DELEGATE_H_

#include <string>

#include "base/macros.h"
#include "chrome/renderer/extensions/app_bindings_core.h"
#include "extensions/renderer/bindings/api_binding_hooks_delegate.h"
#include "v8/include/v8.h"

namespace extensions {
class APIRequestHandler;
class Dispatcher;

// The custom hooks for the chrome.app API.
class AppHooksDelegate : public APIBindingHooksDelegate {
 public:
  using GetterCallback =
      base::Callback<void(const v8::PropertyCallbackInfo<v8::Value>& info)>;

  AppHooksDelegate(Dispatcher* dispatcher, APIRequestHandler* request_handler);
  ~AppHooksDelegate() override;

  // APIBindingHooksDelegate:
  APIBindingHooks::RequestResult HandleRequest(
      const std::string& method_name,
      const APISignature* signature,
      v8::Local<v8::Context> context,
      std::vector<v8::Local<v8::Value>>* arguments,
      const APITypeReferenceMap& refs) override;
  void InitializeTemplate(v8::Isolate* isolate,
                          v8::Local<v8::ObjectTemplate> object_template,
                          const APITypeReferenceMap& type_refs) override;

 private:
  void OnGotInstallState(int request_id, const std::string& install_state);

  AppBindingsCore app_core_;

  APIRequestHandler* request_handler_ = nullptr;

  GetterCallback callback_;

  DISALLOW_COPY_AND_ASSIGN(AppHooksDelegate);
};

}  // namespace extensions

#endif  // CHROME_RENDERER_EXTENSIONS_APP_HOOKS_DELEGATE_H_
