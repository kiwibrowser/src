// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/renderer/extensions/app_bindings_core.h"

#include <memory>

#include "base/values.h"
#include "chrome/common/extensions/extension_constants.h"
#include "content/public/renderer/render_frame.h"
#include "content/public/renderer/v8_value_converter.h"
#include "extensions/common/constants.h"
#include "extensions/common/extension_messages.h"
#include "extensions/common/extension_set.h"
#include "extensions/common/manifest.h"
#include "extensions/renderer/dispatcher.h"
#include "extensions/renderer/renderer_extension_registry.h"
#include "extensions/renderer/script_context.h"
#include "third_party/blink/public/web/web_document.h"
#include "third_party/blink/public/web/web_local_frame.h"
#include "v8/include/v8.h"

namespace extensions {

AppBindingsCore::AppBindingsCore(Dispatcher* dispatcher)
    : dispatcher_(dispatcher) {}
AppBindingsCore::~AppBindingsCore() {}

bool AppBindingsCore::GetIsInstalled(ScriptContext* script_context) const {
  const Extension* extension = script_context->extension();

  // TODO(aa): Why only hosted app?
  return extension && extension->is_hosted_app() &&
         dispatcher_->IsExtensionActive(extension->id());
}

v8::Local<v8::Value> AppBindingsCore::GetDetails(
    ScriptContext* script_context) const {
  blink::WebLocalFrame* web_frame = script_context->web_frame();
  CHECK(web_frame);

  v8::Isolate* isolate = script_context->isolate();
  if (web_frame->GetDocument().GetSecurityOrigin().IsUnique())
    return v8::Null(isolate);

  const Extension* extension =
      RendererExtensionRegistry::Get()->GetExtensionOrAppByURL(
          web_frame->GetDocument().Url());

  if (!extension)
    return v8::Null(isolate);

  std::unique_ptr<base::DictionaryValue> manifest_copy =
      extension->manifest()->value()->CreateDeepCopy();
  manifest_copy->SetString("id", extension->id());
  return content::V8ValueConverter::Create()->ToV8Value(
      manifest_copy.get(), script_context->v8_context());
}

void AppBindingsCore::GetInstallState(ScriptContext* script_context,
                                      GetInstallStateCallback callback) {
  content::RenderFrame* render_frame = script_context->GetRenderFrame();
  CHECK(render_frame);

  int callback_id = next_callback_id_++;
  callbacks_[callback_id] = std::move(callback);

  Send(new ExtensionHostMsg_GetAppInstallState(
      render_frame->GetRoutingID(),
      script_context->web_frame()->GetDocument().Url(), GetRoutingID(),
      callback_id));
}

const char* AppBindingsCore::GetRunningState(
    ScriptContext* script_context) const {
  // To distinguish between ready_to_run and cannot_run states, we need the app
  // from the top frame.
  const RendererExtensionRegistry* extensions =
      RendererExtensionRegistry::Get();

  url::Origin top_origin =
      script_context->web_frame()->Top()->GetSecurityOrigin();
  // The app associated with the top level frame.
  const Extension* top_app = extensions->GetHostedAppByURL(top_origin.GetURL());

  // The app associated with this frame.
  const Extension* this_app = extensions->GetHostedAppByURL(
      script_context->web_frame()->GetDocument().Url());

  if (!this_app || !top_app)
    return extension_misc::kAppStateCannotRun;

  const char* state = nullptr;
  if (dispatcher_->IsExtensionActive(top_app->id())) {
    if (top_app == this_app)
      state = extension_misc::kAppStateRunning;
    else
      state = extension_misc::kAppStateCannotRun;
  } else if (top_app == this_app) {
    state = extension_misc::kAppStateReadyToRun;
  } else {
    state = extension_misc::kAppStateCannotRun;
  }

  return state;
}

bool AppBindingsCore::OnMessageReceived(const IPC::Message& message) {
  IPC_BEGIN_MESSAGE_MAP(AppBindingsCore, message)
    IPC_MESSAGE_HANDLER(ExtensionMsg_GetAppInstallStateResponse,
                        OnAppInstallStateResponse)
    IPC_MESSAGE_UNHANDLED(CHECK(false) << "Unhandled IPC message")
  IPC_END_MESSAGE_MAP()
  return true;
}

void AppBindingsCore::OnAppInstallStateResponse(const std::string& state,
                                                int callback_id) {
  auto iter = callbacks_.find(callback_id);
  DCHECK(iter != callbacks_.end());
  GetInstallStateCallback callback = std::move(iter->second);
  callbacks_.erase(iter);
  std::move(callback).Run(state);
}

}  // namespace extensions
