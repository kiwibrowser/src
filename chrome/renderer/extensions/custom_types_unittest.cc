// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "extensions/renderer/storage_area.h"

#include "base/command_line.h"
#include "components/crx_file/id_util.h"
#include "extensions/common/extension_builder.h"
#include "extensions/common/features/simple_feature.h"
#include "extensions/common/switches.h"
#include "extensions/renderer/bindings/api_binding_test_util.h"
#include "extensions/renderer/bindings/api_binding_util.h"
#include "extensions/renderer/native_extension_bindings_system.h"
#include "extensions/renderer/native_extension_bindings_system_test_base.h"
#include "extensions/renderer/script_context.h"

namespace extensions {

class CustomTypesTest : public NativeExtensionBindingsSystemUnittest {
 public:
  CustomTypesTest()
      : extension_id_(crx_file::id_util::GenerateId("id")),
        allowlisted_extension_id_(extension_id_) {}

  ~CustomTypesTest() override = default;

  // Checks behavior of script after the main context is invalidated.
  // Creates an extension with the given |permission|, and then runs
  // |use_api_script| as a function with a single argument, the result of
  // |api_script|. This expects the function to succeed while the function is
  // valid, and then fail when the function is invalidated with the expected
  // error.
  // Note that no other validations are made (e.g., around the correctness of
  // the call made to the API).
  void RunContextInvalidationTest(const char* permission,
                                  const char* api_script,
                                  const char* use_api_script) {
    scoped_refptr<Extension> extension = ExtensionBuilder("foo")
                                             .AddPermission(permission)
                                             .SetID(extension_id_)
                                             .Build();
    RegisterExtension(extension);

    v8::HandleScope handle_scope(isolate());
    v8::Local<v8::Context> context = MainContext();

    ScriptContext* script_context = CreateScriptContext(
        context, extension.get(), Feature::BLESSED_EXTENSION_CONTEXT);
    script_context->set_url(extension->url());

    bindings_system()->UpdateBindingsForContext(script_context);

    v8::Local<v8::Value> api_object =
        V8ValueFromScriptSource(context, api_script);
    ASSERT_TRUE(api_object->IsObject());

    v8::Local<v8::Function> use_api =
        FunctionFromString(context, use_api_script);
    v8::Local<v8::Value> args[] = {api_object};
    RunFunction(use_api, context, arraysize(args), args);

    DisposeContext(context);

    EXPECT_FALSE(binding::IsContextValid(context));
    RunFunctionAndExpectError(use_api, context, arraysize(args), args,
                              "Uncaught Error: Extension context invalidated.");
  }

 private:
  std::string extension_id_;
  SimpleFeature::ScopedThreadUnsafeAllowlistForTest allowlisted_extension_id_;

  DISALLOW_COPY_AND_ASSIGN(CustomTypesTest);
};

TEST_F(CustomTypesTest, ContentSettingsUseAfterInvalidation) {
  RunContextInvalidationTest("contentSettings",
                             "chrome.contentSettings.javascript",
                             R"((function(setting) {
                                setting.set({
                                  primaryPattern: '<all_urls>',
                                  setting: 'block' });
                                });)");
}

TEST_F(CustomTypesTest, ChromeSettingsAPIUseAfterInvalidation) {
  RunContextInvalidationTest(
      "privacy", "chrome.privacy.websites.doNotTrackEnabled",
      R"((function(setting) { setting.set({value: true}); }))");
}

TEST_F(CustomTypesTest, ChromeSettingsEventUseAfterInvalidation) {
  RunContextInvalidationTest("privacy",
                             "chrome.privacy.websites.doNotTrackEnabled",
                             R"((function(setting) {
                                  setting.onChange.addListener(function() {});
                                });)");
}

TEST_F(CustomTypesTest, EasyUnlockProximityRequiredUseAfterInvalidation) {
  RunContextInvalidationTest(
      "preferencesPrivate",
      "chrome.preferencesPrivate.easyUnlockProximityRequired",
      R"((function(setting) {
           setting.onChange.addListener(function() {});
         });)");
}

}  // namespace extensions
