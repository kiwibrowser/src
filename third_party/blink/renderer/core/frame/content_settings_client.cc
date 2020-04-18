// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/core/frame/content_settings_client.h"

#include "third_party/blink/public/platform/web_content_setting_callbacks.h"
#include "third_party/blink/public/platform/web_content_settings_client.h"
#include "third_party/blink/public/platform/web_security_origin.h"
#include "third_party/blink/public/platform/web_url.h"
#include "third_party/blink/renderer/platform/content_setting_callbacks.h"
#include "third_party/blink/renderer/platform/weborigin/kurl.h"
#include "third_party/blink/renderer/platform/weborigin/security_origin.h"

namespace blink {

ContentSettingsClient::ContentSettingsClient() = default;

bool ContentSettingsClient::AllowDatabase(const String& name,
                                          const String& display_name,
                                          unsigned estimated_size) {
  if (client_)
    return client_->AllowDatabase(name, display_name, estimated_size);
  return true;
}

bool ContentSettingsClient::AllowIndexedDB(
    const String& name,
    const SecurityOrigin* security_origin) {
  if (client_)
    return client_->AllowIndexedDB(name, WebSecurityOrigin(security_origin));
  return true;
}

bool ContentSettingsClient::RequestFileSystemAccessSync() {
  if (client_)
    return client_->RequestFileSystemAccessSync();
  return true;
}

void ContentSettingsClient::RequestFileSystemAccessAsync(
    std::unique_ptr<ContentSettingCallbacks> callbacks) {
  if (client_)
    client_->RequestFileSystemAccessAsync(std::move(callbacks));
  else
    callbacks->OnAllowed();
}

bool ContentSettingsClient::AllowScript(bool enabled_per_settings) {
  if (client_)
    return client_->AllowScript(enabled_per_settings);
  return enabled_per_settings;
}

bool ContentSettingsClient::AllowScriptFromSource(bool enabled_per_settings,
                                                  const KURL& script_url) {
  if (client_)
    return client_->AllowScriptFromSource(enabled_per_settings, script_url);
  return enabled_per_settings;
}

void ContentSettingsClient::GetAllowedClientHintsFromSource(
    const KURL& url,
    WebEnabledClientHints* client_hints) {
  if (client_)
    client_->GetAllowedClientHintsFromSource(url, client_hints);
}

bool ContentSettingsClient::AllowImage(bool enabled_per_settings,
                                       const KURL& image_url) {
  if (client_)
    return client_->AllowImage(enabled_per_settings, image_url);
  return enabled_per_settings;
}

bool ContentSettingsClient::AllowReadFromClipboard(bool default_value) {
  if (client_)
    return client_->AllowReadFromClipboard(default_value);
  return default_value;
}

bool ContentSettingsClient::AllowWriteToClipboard(bool default_value) {
  if (client_)
    return client_->AllowWriteToClipboard(default_value);
  return default_value;
}

bool ContentSettingsClient::AllowStorage(StorageType type) {
  if (client_)
    return client_->AllowStorage(type == StorageType::kLocal);
  return true;
}

bool ContentSettingsClient::AllowRunningInsecureContent(
    bool enabled_per_settings,
    const SecurityOrigin* origin,
    const KURL& url) {
  if (client_) {
    return client_->AllowRunningInsecureContent(enabled_per_settings,
                                                WebSecurityOrigin(origin), url);
  }
  return enabled_per_settings;
}

bool ContentSettingsClient::AllowMutationEvents(bool default_value) {
  if (client_)
    return client_->AllowMutationEvents(default_value);
  return default_value;
}

bool ContentSettingsClient::AllowAutoplay(bool default_value) {
  if (client_)
    return client_->AllowAutoplay(default_value);
  return default_value;
}

bool ContentSettingsClient::AllowPopupsAndRedirects(bool default_value) {
  if (client_)
    return client_->AllowPopupsAndRedirects(default_value);
  return default_value;
}

void ContentSettingsClient::PassiveInsecureContentFound(const KURL& url) {
  if (client_)
    return client_->PassiveInsecureContentFound(url);
}

void ContentSettingsClient::DidNotAllowScript() {
  if (client_)
    client_->DidNotAllowScript();
}

void ContentSettingsClient::DidNotAllowPlugins() {
  if (client_)
    client_->DidNotAllowPlugins();
}

void ContentSettingsClient::PersistClientHints(
    const WebEnabledClientHints& enabled_client_hints,
    TimeDelta duration,
    const KURL& url) {
  if (client_) {
    return client_->PersistClientHints(enabled_client_hints, duration, url);
  }
}

}  // namespace blink
