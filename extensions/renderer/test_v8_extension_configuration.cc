// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "extensions/renderer/test_v8_extension_configuration.h"

#include "base/lazy_instance.h"
#include "extensions/renderer/safe_builtins.h"
#include "v8/include/v8.h"

namespace extensions {

namespace {

base::LazyInstance<TestV8ExtensionConfiguration>::Leaky
    g_v8_extension_configuration = LAZY_INSTANCE_INITIALIZER;

}  // namespace

TestV8ExtensionConfiguration::TestV8ExtensionConfiguration()
    : safe_builtins_(SafeBuiltins::CreateV8Extension()),
      v8_extension_names_(1, safe_builtins_->name()),
      v8_extension_configuration_(new v8::ExtensionConfiguration(
          static_cast<int>(v8_extension_names_.size()),
          v8_extension_names_.data())) {
  v8::RegisterExtension(safe_builtins_.get());
}

TestV8ExtensionConfiguration::~TestV8ExtensionConfiguration() {}

// static
v8::ExtensionConfiguration* TestV8ExtensionConfiguration::GetConfiguration() {
  return g_v8_extension_configuration.Get().v8_extension_configuration_.get();
}

}  // namespace extensions
