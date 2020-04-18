// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef EXTENSIONS_RENDERER_EASY_UNLOCK_PROXIMITY_REQUIRED_STUB_H_
#define EXTENSIONS_RENDERER_EASY_UNLOCK_PROXIMITY_REQUIRED_STUB_H_

#include <string>

#include "base/macros.h"
#include "extensions/renderer/bindings/argument_spec.h"
#include "gin/wrappable.h"
#include "v8/include/v8.h"

namespace base {
class ListValue;
}

namespace gin {
class Arguments;
}

namespace extensions {
class APIEventHandler;
class APIRequestHandler;
class BindingAccessChecker;

// A stub object for the EasyUnlockProxmityRequired object. This only needs to
// support a single property, an event for 'onChange' (which will never be
// triggered).
// TODO(devlin): Remove this once the preferencesPrivate API is fully removed.
// https://crbug.com/593166
class EasyUnlockProximityRequiredStub final
    : public gin::Wrappable<EasyUnlockProximityRequiredStub> {
 public:
  ~EasyUnlockProximityRequiredStub() override;

  static v8::Local<v8::Object> Create(
      v8::Isolate* isolate,
      const std::string& property_name,
      const base::ListValue* property_values,
      APIRequestHandler* request_handler,
      APIEventHandler* event_handler,
      APITypeReferenceMap* type_refs,
      const BindingAccessChecker* access_checker);

  static gin::WrapperInfo kWrapperInfo;

  gin::ObjectTemplateBuilder GetObjectTemplateBuilder(
      v8::Isolate* isolate) override;

 private:
  EasyUnlockProximityRequiredStub(APIEventHandler* event_handler);

  // Returns the onChange event.
  v8::Local<v8::Value> GetOnChangeEvent(gin::Arguments* arguments);

  // The binding system's event handler; guaranteed to outlive this object.
  APIEventHandler* const event_handler_;

  DISALLOW_COPY_AND_ASSIGN(EasyUnlockProximityRequiredStub);
};

}  // namespace extensions

#endif  // EXTENSIONS_RENDERER_EASY_UNLOCK_PROXIMITY_REQUIRED_STUB_H_
