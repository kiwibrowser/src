// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "extensions/renderer/css_native_handler.h"

#include "extensions/renderer/script_context.h"
#include "extensions/renderer/v8_helpers.h"
#include "third_party/blink/public/platform/web_string.h"
#include "third_party/blink/public/web/web_selector.h"

namespace extensions {

using blink::WebString;

CssNativeHandler::CssNativeHandler(ScriptContext* context)
    : ObjectBackedNativeHandler(context) {}

void CssNativeHandler::AddRoutes() {
  RouteHandlerFunction(
      "CanonicalizeCompoundSelector", "declarativeContent",
      base::Bind(&CssNativeHandler::CanonicalizeCompoundSelector,
                 base::Unretained(this)));
}

void CssNativeHandler::CanonicalizeCompoundSelector(
    const v8::FunctionCallbackInfo<v8::Value>& args) {
  CHECK_EQ(args.Length(), 1);
  CHECK(args[0]->IsString());
  v8::Isolate* isolate = args.GetIsolate();
  std::string input_selector = *v8::String::Utf8Value(isolate, args[0]);
  // TODO(esprehn): This API shouldn't exist, the extension code should be
  // moved into blink.
  WebString output_selector = blink::CanonicalizeSelector(
      WebString::FromUTF8(input_selector), blink::kWebSelectorTypeCompound);
  args.GetReturnValue().Set(
      v8_helpers::ToV8StringUnsafe(isolate, output_selector.Utf8().c_str()));
}

}  // namespace extensions
