// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef EXTENSIONS_RENDERER_PROCESS_INFO_NATIVE_HANDLER_H_
#define EXTENSIONS_RENDERER_PROCESS_INFO_NATIVE_HANDLER_H_

#include <string>

#include "extensions/renderer/object_backed_native_handler.h"

namespace extensions {

class ProcessInfoNativeHandler : public ObjectBackedNativeHandler {
 public:
  ProcessInfoNativeHandler(ScriptContext* context,
                           const std::string& extension_id,
                           const std::string& context_type,
                           bool is_incognito_context,
                           bool is_component_extension,
                           int manifest_version,
                           bool send_request_disabled);

  // ObjectBackedNativeHandler:
  void AddRoutes() override;

 private:
  void GetExtensionId(const v8::FunctionCallbackInfo<v8::Value>& args);
  void GetContextType(const v8::FunctionCallbackInfo<v8::Value>& args);
  void InIncognitoContext(const v8::FunctionCallbackInfo<v8::Value>& args);
  void IsComponentExtension(const v8::FunctionCallbackInfo<v8::Value>& args);
  void GetManifestVersion(const v8::FunctionCallbackInfo<v8::Value>& args);
  void IsSendRequestDisabled(const v8::FunctionCallbackInfo<v8::Value>& args);
  void HasSwitch(const v8::FunctionCallbackInfo<v8::Value>& args);
  void GetPlatform(const v8::FunctionCallbackInfo<v8::Value>& args);

  std::string extension_id_;
  std::string context_type_;
  bool is_incognito_context_;
  bool is_component_extension_;
  int manifest_version_;
  bool send_request_disabled_;
};

}  // namespace extensions

#endif  // EXTENSIONS_RENDERER_PROCESS_INFO_NATIVE_HANDLER_H_
