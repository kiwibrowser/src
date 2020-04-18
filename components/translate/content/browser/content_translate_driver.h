// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_TRANSLATE_CONTENT_BROWSER_CONTENT_TRANSLATE_DRIVER_H_
#define COMPONENTS_TRANSLATE_CONTENT_BROWSER_CONTENT_TRANSLATE_DRIVER_H_

#include <map>

#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "base/observer_list.h"
#include "components/translate/content/common/translate.mojom.h"
#include "components/translate/core/browser/translate_driver.h"
#include "components/translate/core/common/translate_errors.h"
#include "content/public/browser/web_contents_observer.h"

namespace content {
class NavigationController;
class WebContents;
}

namespace translate {

struct LanguageDetectionDetails;
class TranslateManager;

// Content implementation of TranslateDriver.
class ContentTranslateDriver : public TranslateDriver,
                               public content::WebContentsObserver {
 public:
  // The observer for the ContentTranslateDriver.
  class Observer {
   public:
    // Handles when the value of IsPageTranslated is changed.
    virtual void OnIsPageTranslatedChanged(content::WebContents* source) {};

    // Handles when the value of translate_enabled is changed.
    virtual void OnTranslateEnabledChanged(content::WebContents* source) {};

    // Called when the page language has been determined.
    virtual void OnLanguageDetermined(
        const translate::LanguageDetectionDetails& details) {};

    // Called when the page has been translated.
    virtual void OnPageTranslated(
        const std::string& original_lang,
        const std::string& translated_lang,
        translate::TranslateErrors::Type error_type) {};

   protected:
    virtual ~Observer() {}
  };

  explicit ContentTranslateDriver(
      content::NavigationController* nav_controller);
  ~ContentTranslateDriver() override;

  // Adds or Removes observers.
  void AddObserver(Observer* observer);
  void RemoveObserver(Observer* observer);

  // Number of attempts before waiting for a page to be fully reloaded.
  void set_translate_max_reload_attempts(int attempts) {
    max_reload_check_attempts_ = attempts;
  }

  // Sets the TranslateManager associated with this driver.
  void set_translate_manager(TranslateManager* manager) {
    translate_manager_ = manager;
  }

  // Initiates translation once the page is finished loading.
  void InitiateTranslation(const std::string& page_lang, int attempt);

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

  // content::WebContentsObserver implementation.
  void NavigationEntryCommitted(
      const content::LoadCommittedDetails& load_details) override;
  void DidFinishNavigation(
      content::NavigationHandle* navigation_handle) override;

  void OnPageTranslated(bool cancelled,
                        const std::string& original_lang,
                        const std::string& translated_lang,
                        TranslateErrors::Type error_type);

  // Called when a page has been loaded and can be potentially translated.
  void OnPageReady(mojom::PagePtr page,
                   const LanguageDetectionDetails& details,
                   bool page_needs_translation);

 private:
  void OnPageAway(int page_seq_no);

  // The navigation controller of the tab we are associated with.
  content::NavigationController* navigation_controller_;

  TranslateManager* translate_manager_;

  base::ObserverList<Observer, true> observer_list_;

  // Max number of attempts before checking if a page has been reloaded.
  int max_reload_check_attempts_;

  // Records mojo connections with all current alive pages.
  int next_page_seq_no_;
  std::map<int, mojom::PagePtr> pages_;

  base::WeakPtrFactory<ContentTranslateDriver> weak_pointer_factory_;

  DISALLOW_COPY_AND_ASSIGN(ContentTranslateDriver);
};

}  // namespace translate

#endif  // COMPONENTS_TRANSLATE_CONTENT_BROWSER_CONTENT_TRANSLATE_DRIVER_H_
