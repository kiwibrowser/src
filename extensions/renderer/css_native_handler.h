// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef EXTENSIONS_RENDERER_CSS_NATIVE_HANDLER_H_
#define EXTENSIONS_RENDERER_CSS_NATIVE_HANDLER_H_

#include "extensions/renderer/object_backed_native_handler.h"

namespace extensions {
class ScriptContext;

class CssNativeHandler : public ObjectBackedNativeHandler {
 public:
  explicit CssNativeHandler(ScriptContext* context);

  // ObjectBackedNativeHandler:
  void AddRoutes() override;

 private:
  // Expects one string argument that's a comma-separated list of compound CSS
  // selectors (http://dev.w3.org/csswg/selectors4/#compound), and returns its
  // Blink-canonicalized form. If the selector is invalid, returns an empty
  // string.
  void CanonicalizeCompoundSelector(
      const v8::FunctionCallbackInfo<v8::Value>& args);
};

}  // namespace extensions

#endif  // EXTENSIONS_RENDERER_CSS_NATIVE_HANDLER_H_
