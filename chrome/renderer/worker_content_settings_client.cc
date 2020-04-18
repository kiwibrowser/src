// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/renderer/worker_content_settings_client.h"

#include "chrome/common/render_messages.h"
#include "chrome/renderer/content_settings_observer.h"
#include "content/public/renderer/render_frame.h"
#include "content/public/renderer/render_thread.h"
#include "ipc/ipc_sync_message_filter.h"
#include "third_party/blink/public/platform/url_conversion.h"
#include "third_party/blink/public/platform/web_security_origin.h"
#include "third_party/blink/public/web/web_document.h"
#include "third_party/blink/public/web/web_local_frame.h"
#include "url/origin.h"

WorkerContentSettingsClient::WorkerContentSettingsClient(
    content::RenderFrame* render_frame)
    : routing_id_(render_frame->GetRoutingID()), is_unique_origin_(false) {
  blink::WebLocalFrame* frame = render_frame->GetWebFrame();
  if (frame->GetDocument().GetSecurityOrigin().IsUnique() ||
      frame->Top()->GetSecurityOrigin().IsUnique())
    is_unique_origin_ = true;
  sync_message_filter_ = content::RenderThread::Get()->GetSyncMessageFilter();
  document_origin_url_ =
      url::Origin(frame->GetDocument().GetSecurityOrigin()).GetURL();
  top_frame_origin_url_ =
      url::Origin(frame->Top()->GetSecurityOrigin()).GetURL();
  allow_running_insecure_content_ = ContentSettingsObserver::Get(render_frame)
                                        ->allow_running_insecure_content();
  content_setting_rules_ =
      ContentSettingsObserver::Get(render_frame)->GetContentSettingRules();
}

WorkerContentSettingsClient::WorkerContentSettingsClient(
    const WorkerContentSettingsClient& other)
    : routing_id_(other.routing_id_),
      is_unique_origin_(other.is_unique_origin_),
      document_origin_url_(other.document_origin_url_),
      top_frame_origin_url_(other.top_frame_origin_url_),
      allow_running_insecure_content_(other.allow_running_insecure_content_),
      sync_message_filter_(other.sync_message_filter_),
      content_setting_rules_(other.content_setting_rules_) {}

WorkerContentSettingsClient::~WorkerContentSettingsClient() {}

std::unique_ptr<blink::WebContentSettingsClient>
WorkerContentSettingsClient::Clone() {
  return base::WrapUnique(new WorkerContentSettingsClient(*this));
}

bool WorkerContentSettingsClient::RequestFileSystemAccessSync() {
  if (is_unique_origin_)
    return false;

  bool result = false;
  sync_message_filter_->Send(new ChromeViewHostMsg_RequestFileSystemAccessSync(
      routing_id_, document_origin_url_, top_frame_origin_url_, &result));
  return result;
}

bool WorkerContentSettingsClient::AllowIndexedDB(
    const blink::WebString& name,
    const blink::WebSecurityOrigin&) {
  if (is_unique_origin_)
    return false;

  bool result = false;
  sync_message_filter_->Send(new ChromeViewHostMsg_AllowIndexedDB(
      routing_id_, document_origin_url_, top_frame_origin_url_, name.Utf16(),
      &result));
  return result;
}

bool WorkerContentSettingsClient::AllowRunningInsecureContent(
    bool allowed_per_settings,
    const blink::WebSecurityOrigin& context,
    const blink::WebURL& url) {
  if (!allow_running_insecure_content_ && !allowed_per_settings) {
    sync_message_filter_->Send(new ChromeViewHostMsg_ContentBlocked(
        routing_id_, CONTENT_SETTINGS_TYPE_MIXEDSCRIPT, base::string16()));
    return false;
  }

  return true;
}

bool WorkerContentSettingsClient::AllowScriptFromSource(
    bool enabled_per_settings,
    const blink::WebURL& script_url) {
  bool allow = enabled_per_settings;
  if (allow && content_setting_rules_) {
    for (const auto& rule : content_setting_rules_->script_rules) {
      if (rule.primary_pattern.Matches(top_frame_origin_url_) &&
          rule.secondary_pattern.Matches(script_url)) {
        allow = rule.GetContentSetting() != CONTENT_SETTING_BLOCK;
        break;
      }
    }
  }

  if (!allow) {
    sync_message_filter_->Send(new ChromeViewHostMsg_ContentBlocked(
        routing_id_, CONTENT_SETTINGS_TYPE_JAVASCRIPT, base::string16()));
    return false;
  }

  return true;
}
