// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "extensions/renderer/document_custom_bindings.h"

#include <string>

#include "base/bind.h"
#include "extensions/renderer/script_context.h"
#include "third_party/blink/public/web/web_document.h"
#include "third_party/blink/public/web/web_local_frame.h"
#include "v8/include/v8.h"

namespace extensions {

DocumentCustomBindings::DocumentCustomBindings(ScriptContext* context)
    : ObjectBackedNativeHandler(context) {}

void DocumentCustomBindings::AddRoutes() {
  RouteHandlerFunction("RegisterElement",
                       base::Bind(&DocumentCustomBindings::RegisterElement,
                                  base::Unretained(this)));
}

// Attach an event name to an object.
void DocumentCustomBindings::RegisterElement(
    const v8::FunctionCallbackInfo<v8::Value>& args) {
  if (args.Length() != 2 || !args[0]->IsString() || !args[1]->IsObject()) {
    NOTREACHED();
    return;
  }

  std::string element_name(*v8::String::Utf8Value(args.GetIsolate(), args[0]));
  v8::Local<v8::Object> options = v8::Local<v8::Object>::Cast(args[1]);

  blink::WebDocument document = context()->web_frame()->GetDocument();
  v8::Local<v8::Value> constructor = document.RegisterEmbedderCustomElement(
      blink::WebString::FromUTF8(element_name), options);
  args.GetReturnValue().Set(constructor);
}

}  // namespace extensions
