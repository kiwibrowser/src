// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_TRANSLATE_IOS_BROWSER_IOS_TRANSLATE_DRIVER_H_
#define COMPONENTS_TRANSLATE_IOS_BROWSER_IOS_TRANSLATE_DRIVER_H_

#include <memory>
#include <string>

#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "components/language/ios/browser/ios_language_detection_tab_helper.h"
#include "components/translate/core/browser/translate_driver.h"
#include "components/translate/ios/browser/language_detection_controller.h"
#include "components/translate/ios/browser/translate_controller.h"
#include "ios/web/public/web_state/web_state_observer.h"

@class CRWJSInjectionReceiver;

namespace web {
class NavigationManager;
class WebState;
}

namespace translate {

class TranslateManager;

// Content implementation of TranslateDriver.
class IOSTranslateDriver : public TranslateDriver,
                           public TranslateController::Observer,
                           public web::WebStateObserver {
 public:
  IOSTranslateDriver(web::WebState* web_state,
                     web::NavigationManager* navigation_manager,
                     TranslateManager* translate_manager);
  ~IOSTranslateDriver() override;

  LanguageDetectionController* language_detection_controller() {
    return language_detection_controller_.get();
  }

  TranslateController* translate_controller() {
    return translate_controller_.get();
  }

  // Creates a callback to be used when language detection occurs.
  language::IOSLanguageDetectionTabHelper::Callback
  CreateLanguageDetectionCallback();

  // web::WebStateObserver methods.
  void DidFinishNavigation(web::WebState* web_state,
                           web::NavigationContext* navigation_context) override;
  void WebStateDestroyed(web::WebState* web_state) override;

  // TranslateDriver methods.
  void OnIsPageTranslatedChanged() override;
  void OnTranslateEnabledChanged() override;
  bool IsLinkNavigation() override;
  void TranslatePage(int page_seq_no,
                     const std::string& translate_script,
                     const std::string& source_lang,
                     const std::string& target_lang) override;
  void RevertTranslation(int page_seq_no) override;
  bool IsIncognito() override;
  const std::string& GetContentsMimeType() override;
  const GURL& GetLastCommittedURL() override;
  const GURL& GetVisibleURL() override;
  bool HasCurrentPage() override;
  void OpenUrlInNewTab(const GURL& url) override;

 private:
  // Called when the translation was successful.
  void TranslationDidSucceed(const std::string& source_lang,
                             const std::string& target_lang,
                             int page_seq_no,
                             const std::string& original_page_language,
                             double translation_time);
  // Checks if the current running page translation is finished or errored and
  // notifies the browser accordingly.  If the translation has not terminated,
  // posts a task to check again later.
  // Similar to TranslateHelper::CheckTranslateStatus on desktop.
  void CheckTranslateStatus(const std::string& source_language,
                            const std::string& target_language,
                            int page_seq_no);

  // Returns true if the user has not navigated away and the the page is not
  // being destroyed.
  bool IsPageValid(int page_seq_no) const;

  // Used to kick off translation process.
  void OnLanguageDetermined(const LanguageDetectionDetails& details);

  // TranslateController::Observer methods.
  void OnTranslateScriptReady(bool success,
                              double load_time,
                              double ready_time) override;
  void OnTranslateComplete(bool success,
                           const std::string& original_language,
                           double translation_time) override;

  // The WebState this instance is observing. Will be null after
  // WebStateDestroyed has been called.
  web::WebState* web_state_ = nullptr;

  // The navigation manager of the tab we are associated with.
  web::NavigationManager* navigation_manager_;

  base::WeakPtr<TranslateManager> translate_manager_;
  std::unique_ptr<TranslateController> translate_controller_;
  std::unique_ptr<LanguageDetectionController> language_detection_controller_;

  // An ever-increasing sequence number of the current page, used to match up
  // translation requests with responses.
  // This matches the similar field in TranslateHelper in the renderer on other
  // platforms.
  int page_seq_no_;

  // When a translation is in progress, its page sequence number is stored in
  // |pending_page_seq_no_|.
  int pending_page_seq_no_;

  // Parameters of the current translation.
  std::string source_language_;
  std::string target_language_;

  base::WeakPtrFactory<IOSTranslateDriver> weak_method_factory_;

  DISALLOW_COPY_AND_ASSIGN(IOSTranslateDriver);
};

}  // namespace translate

#endif  // COMPONENTS_TRANSLATE_IOS_BROWSER_IOS_TRANSLATE_DRIVER_H_
