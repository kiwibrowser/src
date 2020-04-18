// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "components/translate/ios/browser/translate_controller.h"

#include "base/bind.h"
#include "base/bind_helpers.h"
#include "base/logging.h"
#include "base/strings/sys_string_conversions.h"
#include "base/values.h"
#import "components/translate/ios/browser/js_translate_manager.h"
#include "ios/web/public/web_state/web_state.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

namespace translate {

namespace {
// Prefix for the translate javascript commands. Must be kept in sync with
// translate_ios.js.
const char kCommandPrefix[] = "translate";
}

TranslateController::TranslateController(web::WebState* web_state,
                                         JsTranslateManager* manager)
    : web_state_(web_state),
      observer_(nullptr),
      js_manager_(manager),
      weak_method_factory_(this) {
  DCHECK(js_manager_);
  DCHECK(web_state_);
  web_state_->AddObserver(this);
  web_state_->AddScriptCommandCallback(
      base::Bind(&TranslateController::OnJavascriptCommandReceived,
                 base::Unretained(this)),
      kCommandPrefix);
}

TranslateController::~TranslateController() {
  if (web_state_) {
    web_state_->RemoveObserver(this);
    web_state_ = nullptr;
  }
}

void TranslateController::InjectTranslateScript(
    const std::string& translate_script) {
  [js_manager_ setScript:base::SysUTF8ToNSString(translate_script)];
  [js_manager_ inject];
  [js_manager_ injectWaitUntilTranslateReadyScript];
}

void TranslateController::RevertTranslation() {
  [js_manager_ revertTranslation];
}

void TranslateController::StartTranslation(const std::string& source_language,
                                           const std::string& target_language) {
  [js_manager_ startTranslationFrom:source_language to:target_language];
}

void TranslateController::CheckTranslateStatus() {
  [js_manager_ injectTranslateStatusScript];
}

void TranslateController::SetJsTranslateManagerForTesting(
    JsTranslateManager* manager) {
  js_manager_.reset(manager);
}

bool TranslateController::OnJavascriptCommandReceived(
    const base::DictionaryValue& command,
    const GURL& url,
    bool interacting) {
  const base::Value* value = nullptr;
  command.Get("command", &value);
  if (!value) {
    return false;
  }

  std::string out_string;
  value->GetAsString(&out_string);
  if (out_string == "translate.ready")
    return OnTranslateReady(command);
  if (out_string == "translate.status")
    return OnTranslateComplete(command);

  NOTREACHED();
  return false;
}

bool TranslateController::OnTranslateReady(
    const base::DictionaryValue& command) {
  if (!command.HasKey("timeout")) {
    NOTREACHED();
    return false;
  }

  bool timeout = false;
  double load_time = 0.;
  double ready_time = 0.;

  command.GetBoolean("timeout", &timeout);
  if (!timeout) {
    if (!command.HasKey("loadTime") || !command.HasKey("readyTime")) {
      NOTREACHED();
      return false;
    }
    command.GetDouble("loadTime", &load_time);
    command.GetDouble("readyTime", &ready_time);
  }
  if (observer_)
    observer_->OnTranslateScriptReady(!timeout, load_time, ready_time);
  return true;
}

bool TranslateController::OnTranslateComplete(
    const base::DictionaryValue& command) {
  if (!command.HasKey("success")) {
    NOTREACHED();
    return false;
  }

  bool success = false;
  std::string original_language;
  double translation_time = 0.;

  command.GetBoolean("success", &success);
  if (success) {
    if (!command.HasKey("originalPageLanguage") ||
        !command.HasKey("translationTime")) {
      NOTREACHED();
      return false;
    }
    command.GetString("originalPageLanguage", &original_language);
    command.GetDouble("translationTime", &translation_time);
  }

  if (observer_)
    observer_->OnTranslateComplete(success, original_language,
                                   translation_time);
  return true;
}

// web::WebStateObserver implementation.

void TranslateController::WebStateDestroyed(web::WebState* web_state) {
  DCHECK_EQ(web_state_, web_state);
  web_state_->RemoveScriptCommandCallback(kCommandPrefix);
  web_state_->RemoveObserver(this);
  web_state_ = nullptr;
}

}  // namespace translate
