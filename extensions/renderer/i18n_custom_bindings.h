// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef EXTENSIONS_RENDERER_I18N_CUSTOM_BINDINGS_H_
#define EXTENSIONS_RENDERER_I18N_CUSTOM_BINDINGS_H_

#include "extensions/renderer/object_backed_native_handler.h"

namespace extensions {
class ScriptContext;

// Implements custom bindings for the i18n API.
class I18NCustomBindings : public ObjectBackedNativeHandler {
 public:
  explicit I18NCustomBindings(ScriptContext* context);

  // ObjectBackedNativeHandler:
  void AddRoutes() override;

 private:
  void GetL10nMessage(const v8::FunctionCallbackInfo<v8::Value>& args);
  void GetL10nUILanguage(const v8::FunctionCallbackInfo<v8::Value>& args);
  void DetectTextLanguage(const v8::FunctionCallbackInfo<v8::Value>& args);
};

}  // namespace extensions

#endif  // EXTENSIONS_RENDERER_I18N_CUSTOM_BINDINGS_H_
