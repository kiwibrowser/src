// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/renderer/extensions/webstore_bindings.h"

#include <stdint.h>

#include "base/macros.h"
#include "base/strings/string_util.h"
#include "chrome/common/extensions/api/webstore/webstore_api_constants.h"
#include "components/crx_file/id_util.h"
#include "content/public/renderer/render_frame.h"
#include "extensions/common/extension.h"
#include "extensions/common/extension_urls.h"
#include "extensions/renderer/script_context.h"
#include "third_party/blink/public/common/associated_interfaces/associated_interface_provider.h"
#include "third_party/blink/public/web/web_document.h"
#include "third_party/blink/public/web/web_element.h"
#include "third_party/blink/public/web/web_local_frame.h"
#include "third_party/blink/public/web/web_node.h"
#include "third_party/blink/public/web/web_user_gesture_indicator.h"
#include "url/gurl.h"
#include "v8/include/v8.h"

using blink::WebDocument;
using blink::WebElement;
using blink::WebNode;
using blink::WebUserGestureIndicator;

namespace extensions {

namespace {

const char kWebstoreLinkRelation[] = "chrome-webstore-item";

const char kNotInTopFrameError[] =
    "Chrome Web Store installations can only be started by the top frame.";
const char kNotUserGestureError[] =
    "Chrome Web Store installations can only be initated by a user gesture.";
const char kNoWebstoreItemLinkFoundError[] =
    "No Chrome Web Store item link found.";
const char kInvalidWebstoreItemUrlError[] =
    "Invalid Chrome Web Store item URL.";

// chrome.webstore.install() calls generate an install ID so that the install's
// callbacks may be fired when the browser notifies us of install completion
// (successful or not) via |InlineInstallResponse|.
int g_next_install_id = 0;

} // anonymous namespace

WebstoreBindings::WebstoreBindings(ScriptContext* context)
    : ObjectBackedNativeHandler(context) {
  content::RenderFrame* render_frame = context->GetRenderFrame();
  if (render_frame)
    render_frame->GetRemoteAssociatedInterfaces()->GetInterface(
        &inline_installer_);
}

WebstoreBindings::~WebstoreBindings() {}

void WebstoreBindings::AddRoutes() {
  RouteHandlerFunction(
      "Install", "webstore",
      base::Bind(&WebstoreBindings::Install, base::Unretained(this)));
}

void WebstoreBindings::InlineInstallResponse(int install_id,
                                             bool success,
                                             const std::string& error,
                                             webstore_install::Result result) {
  v8::Isolate* isolate = context()->isolate();
  v8::HandleScope handle_scope(isolate);
  v8::Context::Scope context_scope(context()->v8_context());
  v8::Local<v8::Value> argv[] = {
      v8::Integer::New(isolate, install_id), v8::Boolean::New(isolate, success),
      v8::String::NewFromUtf8(isolate, error.c_str()),
      v8::String::NewFromUtf8(
          isolate,
          api::webstore::kInstallResultCodes[static_cast<int>(result)])};
  context()->module_system()->CallModuleMethodSafe(
      "webstore", "onInstallResponse", arraysize(argv), argv);
}

void WebstoreBindings::InlineInstallStageChanged(
    api::webstore::InstallStage stage) {
  const char* stage_string = nullptr;
  switch (stage) {
    case api::webstore::INSTALL_STAGE_DOWNLOADING:
      stage_string = api::webstore::kInstallStageDownloading;
      break;
    case api::webstore::INSTALL_STAGE_INSTALLING:
      stage_string = api::webstore::kInstallStageInstalling;
      break;
  }
  v8::Isolate* isolate = context()->isolate();
  v8::HandleScope handle_scope(isolate);
  v8::Context::Scope context_scope(context()->v8_context());
  v8::Local<v8::Value> argv[] = {
      v8::String::NewFromUtf8(isolate, stage_string)};
  context()->module_system()->CallModuleMethodSafe(
      "webstore", "onInstallStageChanged", arraysize(argv), argv);
}

void WebstoreBindings::InlineInstallDownloadProgress(int percent_downloaded) {
  v8::Isolate* isolate = context()->isolate();
  v8::HandleScope handle_scope(isolate);
  v8::Context::Scope context_scope(context()->v8_context());
  v8::Local<v8::Value> argv[] = {
      v8::Number::New(isolate, percent_downloaded / 100.0)};
  context()->module_system()->CallModuleMethodSafe(
      "webstore", "onDownloadProgress", arraysize(argv), argv);
}

void WebstoreBindings::Install(
    const v8::FunctionCallbackInfo<v8::Value>& args) {
  content::RenderFrame* render_frame = context()->GetRenderFrame();
  if (!render_frame)
    return;

  // The first two arguments indicate whether or not there are install stage
  // or download progress listeners.
  int listener_mask = 0;
  CHECK(args[0]->IsBoolean());
  if (args[0]->BooleanValue())
    listener_mask |= api::webstore::INSTALL_STAGE_LISTENER;
  CHECK(args[1]->IsBoolean());
  if (args[1]->BooleanValue())
    listener_mask |= api::webstore::DOWNLOAD_PROGRESS_LISTENER;

  std::string preferred_store_link_url;
  if (!args[2]->IsUndefined()) {
    CHECK(args[2]->IsString());
    preferred_store_link_url =
        std::string(*v8::String::Utf8Value(args.GetIsolate(), args[2]));
  }

  std::string webstore_item_id;
  std::string error;
  blink::WebLocalFrame* frame = context()->web_frame();

  if (!GetWebstoreItemIdFromFrame(
      frame, preferred_store_link_url, &webstore_item_id, &error)) {
    args.GetIsolate()->ThrowException(
        v8::String::NewFromUtf8(args.GetIsolate(), error.c_str()));
    return;
  }

  int install_id = g_next_install_id++;

  mojom::InlineInstallProgressListenerPtr install_progress_listener;
  install_progress_listener_bindings_.AddBinding(
      this, mojo::MakeRequest(&install_progress_listener));

  inline_installer_->DoInlineInstall(
      webstore_item_id, listener_mask, std::move(install_progress_listener),
      base::Bind(&WebstoreBindings::InlineInstallResponse,
                 base::Unretained(this), install_id));
  args.GetReturnValue().Set(static_cast<int32_t>(install_id));
}

// static
bool WebstoreBindings::GetWebstoreItemIdFromFrame(
    blink::WebLocalFrame* frame,
    const std::string& preferred_store_link_url,
    std::string* webstore_item_id,
    std::string* error) {
  if (frame != frame->Top()) {
    *error = kNotInTopFrameError;
    return false;
  }

  if (!WebUserGestureIndicator::IsProcessingUserGesture(frame)) {
    *error = kNotUserGestureError;
    return false;
  }

  WebDocument document = frame->GetDocument();
  if (document.IsNull()) {
    *error = kNoWebstoreItemLinkFoundError;
    return false;
  }

  WebElement head = document.Head();
  if (head.IsNull()) {
    *error = kNoWebstoreItemLinkFoundError;
    return false;
  }

  GURL webstore_base_url =
      GURL(extension_urls::GetWebstoreItemDetailURLPrefix());
  for (WebNode child = head.FirstChild(); !child.IsNull();
       child = child.NextSibling()) {
    if (!child.IsElementNode())
      continue;
    WebElement elem = child.To<WebElement>();

    if (!elem.HasHTMLTagName("link") || !elem.HasAttribute("rel") ||
        !elem.HasAttribute("href"))
      continue;

    std::string rel = elem.GetAttribute("rel").Utf8();
    if (!base::LowerCaseEqualsASCII(rel, kWebstoreLinkRelation))
      continue;

    std::string webstore_url_string(elem.GetAttribute("href").Utf8());

    if (!preferred_store_link_url.empty() &&
        preferred_store_link_url != webstore_url_string) {
      continue;
    }

    GURL webstore_url = GURL(webstore_url_string);
    if (!webstore_url.is_valid()) {
      *error = kInvalidWebstoreItemUrlError;
      return false;
    }

    if (webstore_url.scheme() != webstore_base_url.scheme() ||
        webstore_url.host() != webstore_base_url.host() ||
        !base::StartsWith(webstore_url.path(), webstore_base_url.path(),
                          base::CompareCase::SENSITIVE)) {
      *error = kInvalidWebstoreItemUrlError;
      return false;
    }

    std::string candidate_webstore_item_id = webstore_url.path().substr(
        webstore_base_url.path().length());
    if (!crx_file::id_util::IdIsValid(candidate_webstore_item_id)) {
      *error = kInvalidWebstoreItemUrlError;
      return false;
    }

    std::string reconstructed_webstore_item_url_string =
        extension_urls::GetWebstoreItemDetailURLPrefix() +
            candidate_webstore_item_id;
    if (reconstructed_webstore_item_url_string != webstore_url_string) {
      *error = kInvalidWebstoreItemUrlError;
      return false;
    }

    *webstore_item_id = candidate_webstore_item_id;
    return true;
  }

  *error = kNoWebstoreItemLinkFoundError;
  return false;
}

void WebstoreBindings::Invalidate() {
  // We should close all mojo pipes when we invalidate the WebstoreBindings
  // object and before its associated v8::context is destroyed. This is to
  // ensure there are no mojo calls that try to access the v8::context after its
  // destruction.
  inline_installer_.reset();
  install_progress_listener_bindings_.CloseAllBindings();
  ObjectBackedNativeHandler::Invalidate();
}

}  // namespace extensions
