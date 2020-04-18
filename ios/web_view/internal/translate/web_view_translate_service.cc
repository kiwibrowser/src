// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ios/web_view/internal/translate/web_view_translate_service.h"

#include "base/memory/ptr_util.h"
#include "base/memory/singleton.h"
#include "components/translate/core/browser/translate_download_manager.h"
#include "ios/web_view/internal/app/application_context.h"

namespace ios_web_view {

WebViewTranslateService::TranslateRequestsAllowedListener::
    TranslateRequestsAllowedListener()
    : resource_request_allowed_notifier_(
          ios_web_view::ApplicationContext::GetInstance()->GetLocalState(),
          /*disable_network_switch=*/nullptr) {
  resource_request_allowed_notifier_.Init(this);
}

WebViewTranslateService::TranslateRequestsAllowedListener::
    ~TranslateRequestsAllowedListener() {}

void WebViewTranslateService::TranslateRequestsAllowedListener::
    OnResourceRequestsAllowed() {
  translate::TranslateLanguageList* language_list =
      translate::TranslateDownloadManager::GetInstance()->language_list();
  DCHECK(language_list);

  language_list->SetResourceRequestsAllowed(
      resource_request_allowed_notifier_.ResourceRequestsAllowed());
}

WebViewTranslateService* WebViewTranslateService::GetInstance() {
  return base::Singleton<WebViewTranslateService>::get();
}

WebViewTranslateService::WebViewTranslateService() {}

WebViewTranslateService::~WebViewTranslateService() = default;

void WebViewTranslateService::Initialize() {
  // Initialize the allowed state for resource requests.
  translate_requests_allowed_listener_.OnResourceRequestsAllowed();

  // Initialize translate.
  translate::TranslateDownloadManager* download_manager =
      translate::TranslateDownloadManager::GetInstance();
  download_manager->set_request_context(
      ios_web_view::ApplicationContext::GetInstance()
          ->GetSystemURLRequestContext());
  download_manager->set_application_locale(
      ios_web_view::ApplicationContext::GetInstance()->GetApplicationLocale());
}

void WebViewTranslateService::Shutdown() {
  translate::TranslateDownloadManager* download_manager =
      translate::TranslateDownloadManager::GetInstance();
  download_manager->Shutdown();
}

}  // namespace ios_web_view
