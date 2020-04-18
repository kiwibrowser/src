// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "extensions/renderer/api_activity_logger.h"

#include "base/run_loop.h"
#include "content/public/test/mock_render_thread.h"
#include "extensions/common/extension_builder.h"
#include "extensions/common/extension_messages.h"
#include "extensions/common/features/feature.h"
#include "extensions/renderer/bindings/api_binding_test.h"
#include "extensions/renderer/bindings/api_binding_test_util.h"
#include "extensions/renderer/script_context.h"
#include "extensions/renderer/script_context_set.h"
#include "extensions/renderer/test_extensions_renderer_client.h"
#include "ipc/ipc_message.h"
#include "ipc/ipc_test_sink.h"

namespace extensions {

namespace {

class ScopedAllowActivityLogging {
 public:
  ScopedAllowActivityLogging() { APIActivityLogger::set_log_for_testing(true); }

  ~ScopedAllowActivityLogging() {
    APIActivityLogger::set_log_for_testing(false);
  }

 private:
  DISALLOW_COPY_AND_ASSIGN(ScopedAllowActivityLogging);
};

}  // namespace

using ActivityLoggerTest = APIBindingTest;

// Regression test for crbug.com/740866.
TEST_F(ActivityLoggerTest, DontCrashOnUnconvertedValues) {
  content::MockRenderThread mock_render_thread;
  TestExtensionsRendererClient client;
  std::set<ExtensionId> extension_ids;
  ScriptContextSet script_context_set(&extension_ids);

  ScopedAllowActivityLogging scoped_allow_activity_logging;

  v8::HandleScope handle_scope(isolate());
  v8::Local<v8::Context> context = MainContext();

  scoped_refptr<const Extension> extension = ExtensionBuilder("Test").Build();
  extension_ids.insert(extension->id());
  const Feature::Context kContextType = Feature::BLESSED_EXTENSION_CONTEXT;
  script_context_set.AddForTesting(std::make_unique<ScriptContext>(
      context, nullptr, extension.get(), kContextType, extension.get(),
      kContextType));

  std::vector<v8::Local<v8::Value>> args = {v8::Undefined(isolate())};

  IPC::TestSink& sink = mock_render_thread.sink();
  sink.ClearMessages();

  APIActivityLogger::LogAPICall(context, "someApiMethod", args);

  ASSERT_EQ(1u, sink.message_count());
  const IPC::Message* message = sink.GetMessageAt(0u);
  ASSERT_EQ(
      static_cast<uint32_t>(ExtensionHostMsg_AddAPIActionToActivityLog::ID),
      message->type());
  ExtensionHostMsg_AddAPIActionToActivityLog::Param full_params;
  ASSERT_TRUE(
      ExtensionHostMsg_AddAPIActionToActivityLog::Read(message, &full_params));
  std::string extension_id = std::get<0>(full_params);
  ExtensionHostMsg_APIActionOrEvent_Params params;
  params.api_call = std::get<1>(full_params).api_call;
  params.arguments =
      base::ListValue(std::get<1>(full_params).arguments.GetList());
  params.extra = std::get<1>(full_params).extra;
  EXPECT_EQ(extension->id(), extension_id);
  ASSERT_EQ(1u, params.arguments.GetList().size());
  EXPECT_EQ(base::Value::Type::NONE, params.arguments.GetList()[0].type());

  ScriptContext* script_context = script_context_set.GetByV8Context(context);
  script_context_set.Remove(script_context);
  base::RunLoop().RunUntilIdle();  // Let script context destruction complete.
}

}  // namespace extensions
