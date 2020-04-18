// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/supervised_user_error_page/gin_wrapper.h"

#include "base/strings/utf_string_conversions.h"
#include "content/public/renderer/render_frame.h"
#include "gin/handle.h"
#include "gin/object_template_builder.h"
#include "third_party/blink/public/web/blink.h"
#include "third_party/blink/public/web/web_local_frame.h"

using web_restrictions::mojom::WebRestrictionsPtr;

namespace supervised_user_error_page {

GinWrapper::Loader::Loader(
    content::RenderFrame* render_frame,
    const std::string& url,
    const web_restrictions::mojom::WebRestrictionsPtr& web_restrictions_service)
    : content::RenderFrameObserver(render_frame),
      url_(url),
      web_restrictions_service_(web_restrictions_service) {}

void GinWrapper::Loader::DidClearWindowObject() {

  InstallGinWrapper();

  // Once the gin wrapper has been installed we don't need to observe the
  // render frame. Delete the loader so that the wrapper isn't re-installed when
  // something else is loaded into the frame.
  delete this;
}

void GinWrapper::Loader::InstallGinWrapper() {
  v8::Isolate* isolate = blink::MainThreadIsolate();
  v8::HandleScope handle_scope(isolate);
  v8::Local<v8::Context> context =
      render_frame()->GetWebFrame()->MainWorldScriptContext();
  if (context.IsEmpty())
    return;
  v8::Context::Scope context_scope(context);
  gin::Handle<GinWrapper> controller = gin::CreateHandle(
      isolate, new GinWrapper(render_frame(), url_, web_restrictions_service_));
  if (controller.IsEmpty())
    return;
  v8::Local<v8::Object> global = context->Global();
  global->Set(gin::StringToV8(isolate, "webRestrictions"), controller.ToV8());
}

void GinWrapper::Loader::OnDestruct() {
  delete this;
}

gin::WrapperInfo GinWrapper::kWrapperInfo = {gin::kEmbedderNativeGin};

// static
void GinWrapper::InstallWhenFrameReady(
    content::RenderFrame* render_frame,
    const std::string& url,
    const WebRestrictionsPtr& web_restrictions_service) {
  new Loader(render_frame, url, web_restrictions_service);
}

GinWrapper::GinWrapper(content::RenderFrame* render_frame,
                       const std::string& url,
                       const WebRestrictionsPtr& web_restrictions_service)
    : url_(url),
      web_restrictions_service_(web_restrictions_service),
      weak_ptr_factory_(this) {}

GinWrapper::~GinWrapper() {}

bool GinWrapper::RequestPermission(
    v8::Local<v8::Function> setRequestStatusCallback) {
  setRequestStatusCallback_.Reset(blink::MainThreadIsolate(),
                                  setRequestStatusCallback);
  web_restrictions_service_->RequestPermission(
      url_, base::Bind(&GinWrapper::OnAccessRequestAdded,
                       weak_ptr_factory_.GetWeakPtr()));
  return true;
}

void GinWrapper::OnAccessRequestAdded(bool success) {
  if (setRequestStatusCallback_.IsEmpty()) {
    return;
  }

  v8::Isolate* isolate = blink::MainThreadIsolate();
  v8::Local<v8::Value> args = v8::Boolean::New(isolate, success);
  v8::HandleScope handle_scope(isolate);
  v8::Local<v8::Function> callback =
      v8::Local<v8::Function>::New(isolate, setRequestStatusCallback_);
  v8::Local<v8::Context> context = callback->CreationContext();
  if (context.IsEmpty())
    return;

  v8::Context::Scope context_scope(context);
  v8::MicrotasksScope microtasks(isolate,
                                 v8::MicrotasksScope::kDoNotRunMicrotasks);

  callback->Call(context->Global(), 1, &args);
}

gin::ObjectTemplateBuilder GinWrapper::GetObjectTemplateBuilder(
    v8::Isolate* isolate) {
  return gin::Wrappable<GinWrapper>::GetObjectTemplateBuilder(isolate)
      .SetMethod("requestPermission", &GinWrapper::RequestPermission);
}

}  // namespace web_restrictions
