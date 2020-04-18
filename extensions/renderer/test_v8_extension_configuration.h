// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef EXTENSIONS_RENDERER_TEST_V8_EXTENSION_CONFIGURATION_H_
#define EXTENSIONS_RENDERER_TEST_V8_EXTENSION_CONFIGURATION_H_

#include <memory>
#include <vector>

#include "base/macros.h"

namespace v8 {
class Extension;
class ExtensionConfiguration;
}

namespace extensions {

// A test helper to allow for the instantiation of the SafeBuiltins
// v8::Extension, which is needed by most/all of our custom bindings.
class TestV8ExtensionConfiguration {
 public:
  TestV8ExtensionConfiguration();
  ~TestV8ExtensionConfiguration();

  static v8::ExtensionConfiguration* GetConfiguration();

 private:
  std::unique_ptr<v8::Extension> safe_builtins_;
  std::vector<const char*> v8_extension_names_;
  std::unique_ptr<v8::ExtensionConfiguration> v8_extension_configuration_;

  DISALLOW_COPY_AND_ASSIGN(TestV8ExtensionConfiguration);
};

}  // namespace extensions

#endif  // EXTENSIONS_RENDERER_TEST_V8_EXTENSION_CONFIGURATION_H_
