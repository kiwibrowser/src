// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/translate/core/browser/translate_manager.h"

#include <memory>

#include "build/build_config.h"
#include "chrome/browser/chrome_notification_types.h"
#include "chrome/browser/prefs/session_startup_pref.h"
#include "chrome/browser/translate/chrome_translate_client.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/ui_test_utils.h"
#include "components/translate/core/browser/translate_error_details.h"
#include "components/translate/core/common/language_detection_details.h"
#include "content/public/browser/notification_service.h"
#include "net/url_request/test_url_fetcher_factory.h"
#include "net/url_request/url_fetcher_delegate.h"
#include "url/gurl.h"

namespace {

static const char kTestValidScript[] =
    "var google = {};"
    "google.translate = (function() {"
    "  return {"
    "    TranslateService: function() {"
    "      return {"
    "        isAvailable : function() {"
    "          return true;"
    "        },"
    "        restore : function() {"
    "          return;"
    "        },"
    "        getDetectedLanguage : function() {"
    "          return \"fr\";"
    "        },"
    "        translatePage : function(originalLang, targetLang,"
    "                                 onTranslateProgress) {"
    "          var error = (originalLang == 'auto') ? true : false;"
    "          onTranslateProgress(100, true, error);"
    "        }"
    "      };"
    "    }"
    "  };"
    "})();"
    "cr.googleTranslate.onTranslateElementLoad();";

static const char kTestScriptInitializationError[] =
    "var google = {};"
    "google.translate = (function() {"
    "  return {"
    "    TranslateService: function() {"
    "      return error;"
    "    }"
    "  };"
    "})();"
    "cr.googleTranslate.onTranslateElementLoad();";

static const char kTestScriptIdenticalLanguages[] =
    "var google = {};"
    "google.translate = (function() {"
    "  return {"
    "    TranslateService: function() {"
    "      return {"
    "        isAvailable : function() {"
    "          return true;"
    "        },"
    "        restore : function() {"
    "          return;"
    "        },"
    "        getDetectedLanguage : function() {"
    "          return \"en\";"
    "        },"
    "        translatePage : function(originalLang, targetLang,"
    "                                 onTranslateProgress) {"
    "          onTranslateProgress(100, true, 0);"
    "        }"
    "      };"
    "    }"
    "  };"
    "})();"
    "cr.googleTranslate.onTranslateElementLoad();";

static const char kTestScriptTimeout[] =
    "var google = {};"
    "google.translate = (function() {"
    "  return {"
    "    TranslateService: function() {"
    "      return {"
    "        isAvailable : function() {"
    "          return false;"
    "        },"
    "      };"
    "    }"
    "  };"
    "})();"
    "cr.googleTranslate.onTranslateElementLoad();";

static const char kTestScriptUnexpectedScriptError[] =
    "var google = {};"
    "google.translate = (function() {"
    "  return {"
    "    TranslateService: function() {"
    "      return {"
    "        isAvailable : function() {"
    "          return true;"
    "        },"
    "        restore : function() {"
    "          return;"
    "        },"
    "        getDetectedLanguage : function() {"
    "          return \"fr\";"
    "        },"
    "        translatePage : function(originalLang, targetLang,"
    "                                 onTranslateProgress) {"
    "          return error;"
    "        }"
    "      };"
    "    }"
    "  };"
    "})();"
    "cr.googleTranslate.onTranslateElementLoad();";

static const char kTestScriptBadOrigin[] =
    "var google = {};"
    "google.translate = (function() {"
    "  return {"
    "    TranslateService: function() {"
    "      return {"
    "        isAvailable : function() {"
    "          return true;"
    "        },"
    "        restore : function() {"
    "          return;"
    "        },"
    "        getDetectedLanguage : function() {"
    "          return \"fr\";"
    "        },"
    "        translatePage : function(originalLang, targetLang,"
    "                                 onTranslateProgress) {"
    "          var url = \"\";"
    "          cr.googleTranslate.onLoadJavascript(url);"
    "        }"
    "      };"
    "    }"
    "  };"
    "})();"
    "cr.googleTranslate.onTranslateElementLoad();";

static const char kTestScriptLoadError[] =
    "var google = {};"
    "google.translate = (function() {"
    "  return {"
    "    TranslateService: function() {"
    "      return {"
    "        isAvailable : function() {"
    "          return true;"
    "        },"
    "        restore : function() {"
    "          return;"
    "        },"
    "        getDetectedLanguage : function() {"
    "          return \"fr\";"
    "        },"
    "        translatePage : function(originalLang, targetLang,"
    "                                 onTranslateProgress) {"
    "          var url = \"https://translate.googleapis.com/INVALID\";"
    "          cr.googleTranslate.onLoadJavascript(url);"
    "        }"
    "      };"
    "    }"
    "  };"
    "})();"
    "cr.googleTranslate.onTranslateElementLoad();";

}  // namespace

class TranslateManagerBrowserTest : public InProcessBrowserTest {
 public:
  TranslateManagerBrowserTest() {
    error_subscription_ =
        translate::TranslateManager::RegisterTranslateErrorCallback(
            base::Bind(&TranslateManagerBrowserTest::OnTranslateError,
                       base::Unretained(this)));
  }
  ~TranslateManagerBrowserTest() override {}

  void WaitUntilLanguageDetected() { language_detected_signal_->Wait(); }
  void WaitUntilPageTranslated() { page_translated_signal_->Wait(); }

  void ResetObserver() {
    language_detected_signal_.reset(new LangageDetectionObserver(
        chrome::NOTIFICATION_TAB_LANGUAGE_DETERMINED,
        content::NotificationService::AllSources()));
    page_translated_signal_.reset(new content::WindowedNotificationObserver(
        chrome::NOTIFICATION_PAGE_TRANSLATED,
        content::NotificationService::AllSources()));
  }

  void SimulateURLFetch(const std::string& script, bool success) {
    net::TestURLFetcher* fetcher = url_fetcher_factory_.GetFetcherByID(0);
    ASSERT_TRUE(fetcher);
    net::Error error = success ? net::OK : net::ERR_FAILED;

    fetcher->set_url(fetcher->GetOriginalURL());
    fetcher->set_status(net::URLRequestStatus::FromError(error));
    fetcher->set_response_code(success ? 200 : 500);
    fetcher->SetResponseString(script);
    fetcher->delegate()->OnURLFetchComplete(fetcher);
  }

  void OnTranslateError(const translate::TranslateErrorDetails& details) {
    error_type_ = details.error;
  }

  translate::TranslateErrors::Type GetPageTranslatedResult() {
    return error_type_;
  }

  ChromeTranslateClient* GetChromeTranslateClient() {
    return ChromeTranslateClient::FromWebContents(
        browser()->tab_strip_model()->GetActiveWebContents());
  }

 protected:
  // InProcessBrowserTest members.
  void SetUp() override {
    InProcessBrowserTest::SetUp();
  }
  void SetUpOnMainThread() override {
    ResetObserver();
    error_type_ = translate::TranslateErrors::NONE;
  }

 private:
  net::TestURLFetcherFactory url_fetcher_factory_;

  translate::TranslateErrors::Type error_type_;

  std::unique_ptr<
      translate::TranslateManager::TranslateErrorCallbackList::Subscription>
      error_subscription_;

  using LangageDetectionObserver =
      ui_test_utils::WindowedNotificationObserverWithDetails<
          translate::LanguageDetectionDetails>;

  std::unique_ptr<LangageDetectionObserver> language_detected_signal_;
  std::unique_ptr<content::WindowedNotificationObserver>
      page_translated_signal_;
};

// Tests that the CLD (Compact Language Detection) works properly.
IN_PROC_BROWSER_TEST_F(TranslateManagerBrowserTest, PageLanguageDetection) {
  ASSERT_TRUE(embedded_test_server()->Start());

  ChromeTranslateClient* chrome_translate_client = GetChromeTranslateClient();

  // The InProcessBrowserTest opens a new tab, let's wait for that first.
  // There is a possible race condition, when the language is not yet detected,
  // so we check for that and wait if necessary.
  if (chrome_translate_client->GetLanguageState().original_language().empty())
    WaitUntilLanguageDetected();

  EXPECT_EQ("und",
            chrome_translate_client->GetLanguageState().original_language());

  // Open a new tab with a page in English.
  ResetObserver();
  AddTabAtIndex(0, GURL(embedded_test_server()->GetURL("/english_page.html")),
                ui::PAGE_TRANSITION_TYPED);
  chrome_translate_client = GetChromeTranslateClient();
  WaitUntilLanguageDetected();

  EXPECT_EQ("en",
            chrome_translate_client->GetLanguageState().original_language());

  ResetObserver();
  // Now navigate to a page in French.
  ui_test_utils::NavigateToURL(
      browser(), GURL(embedded_test_server()->GetURL("/french_page.html")));
  WaitUntilLanguageDetected();

  EXPECT_EQ("fr",
            chrome_translate_client->GetLanguageState().original_language());
}

// Tests that the language detection / HTML attribute override works correctly.
// For languages in the whitelist, the detected language should override the
// HTML attribute. For all other languages, the HTML attribute should be used.
IN_PROC_BROWSER_TEST_F(TranslateManagerBrowserTest,
                       PageLanguageDetectionConflict) {
  ASSERT_TRUE(embedded_test_server()->Start());

  ChromeTranslateClient* chrome_translate_client = GetChromeTranslateClient();

  // The InProcessBrowserTest opens a new tab, let's wait for that first.
  // There is a possible race condition, when the language is not yet detected,
  // so we check for that and wait if necessary.
  if (chrome_translate_client->GetLanguageState().original_language().empty())
    WaitUntilLanguageDetected();

  EXPECT_EQ("und",
            chrome_translate_client->GetLanguageState().original_language());

  // Open a new tab with a page in French with incorrect HTML language
  // attribute specified. The language attribute should be overridden by the
  // language detection.
  ResetObserver();
  AddTabAtIndex(
      0,
      GURL(embedded_test_server()->GetURL("/french_page_lang_conflict.html")),
      ui::PAGE_TRANSITION_TYPED);
  chrome_translate_client = GetChromeTranslateClient();
  WaitUntilLanguageDetected();

  EXPECT_EQ("fr",
            chrome_translate_client->GetLanguageState().original_language());

  // Open a new tab with a page in Korean with incorrect HTML language
  // attribute specified. The language attribute should not be overridden by the
  // language detection.
  ResetObserver();
  AddTabAtIndex(
      0,
      GURL(embedded_test_server()->GetURL("/korean_page_lang_conflict.html")),
      ui::PAGE_TRANSITION_TYPED);
  chrome_translate_client = GetChromeTranslateClient();
  WaitUntilLanguageDetected();

  EXPECT_EQ("en",
            chrome_translate_client->GetLanguageState().original_language());
}

// Test that the translation was successful.
IN_PROC_BROWSER_TEST_F(TranslateManagerBrowserTest, PageTranslationSuccess) {
  ASSERT_TRUE(embedded_test_server()->Start());

  ChromeTranslateClient* chrome_translate_client = GetChromeTranslateClient();

  // There is a possible race condition, when the language is not yet detected,
  // so we check for that and wait if necessary.
  if (chrome_translate_client->GetLanguageState().original_language().empty())
    WaitUntilLanguageDetected();

  EXPECT_EQ("und",
            chrome_translate_client->GetLanguageState().original_language());

  // Open a new tab with a page in French.
  ResetObserver();
  AddTabAtIndex(0, GURL(embedded_test_server()->GetURL("/french_page.html")),
                ui::PAGE_TRANSITION_TYPED);
  chrome_translate_client = GetChromeTranslateClient();
  WaitUntilLanguageDetected();

  EXPECT_EQ("fr",
            chrome_translate_client->GetLanguageState().original_language());

  // Translate the page through TranslateManager.
  translate::TranslateManager* manager =
      chrome_translate_client->GetTranslateManager();
  manager->TranslatePage(
      chrome_translate_client->GetLanguageState().original_language(), "en",
      true);

  SimulateURLFetch(kTestValidScript, true);

  // Wait for NOTIFICATION_PAGE_TRANSLATED notification.
  WaitUntilPageTranslated();

  EXPECT_FALSE(chrome_translate_client->GetLanguageState().translation_error());
  EXPECT_EQ(translate::TranslateErrors::NONE, GetPageTranslatedResult());
}

// Test if there was an error during translation.
IN_PROC_BROWSER_TEST_F(TranslateManagerBrowserTest, PageTranslationError) {
  ASSERT_TRUE(embedded_test_server()->Start());

  ChromeTranslateClient* chrome_translate_client = GetChromeTranslateClient();

  // There is a possible race condition, when the language is not yet detected,
  // so we check for that and wait if necessary.
  if (chrome_translate_client->GetLanguageState().original_language().empty())
    WaitUntilLanguageDetected();

  EXPECT_EQ("und",
            chrome_translate_client->GetLanguageState().original_language());

  // Open a new tab with about:blank page.
  ResetObserver();
  AddTabAtIndex(0, GURL("about:blank"), ui::PAGE_TRANSITION_TYPED);
  chrome_translate_client = GetChromeTranslateClient();
  WaitUntilLanguageDetected();

  EXPECT_EQ("und",
            chrome_translate_client->GetLanguageState().original_language());

  // Translate the page through TranslateManager.
  translate::TranslateManager* manager =
      chrome_translate_client->GetTranslateManager();
  manager->TranslatePage(
      chrome_translate_client->GetLanguageState().original_language(), "en",
      true);

  SimulateURLFetch(kTestValidScript, true);

  // Wait for NOTIFICATION_PAGE_TRANSLATED notification.
  WaitUntilPageTranslated();

  EXPECT_TRUE(chrome_translate_client->GetLanguageState().translation_error());
  EXPECT_EQ(translate::TranslateErrors::TRANSLATION_ERROR,
            GetPageTranslatedResult());
}

// Test if there was an error during translate library initialization.
IN_PROC_BROWSER_TEST_F(TranslateManagerBrowserTest,
                       PageTranslationInitializationError) {
  ASSERT_TRUE(embedded_test_server()->Start());

  ChromeTranslateClient* chrome_translate_client = GetChromeTranslateClient();

  // There is a possible race condition, when the language is not yet detected,
  // so we check for that and wait if necessary.
  if (chrome_translate_client->GetLanguageState().original_language().empty())
    WaitUntilLanguageDetected();

  EXPECT_EQ("und",
            chrome_translate_client->GetLanguageState().original_language());

  // Open a new tab with a page in French.
  ResetObserver();
  AddTabAtIndex(0, GURL(embedded_test_server()->GetURL("/french_page.html")),
                ui::PAGE_TRANSITION_TYPED);
  chrome_translate_client = GetChromeTranslateClient();
  WaitUntilLanguageDetected();

  EXPECT_EQ("fr",
            chrome_translate_client->GetLanguageState().original_language());

  // Translate the page through TranslateManager.
  translate::TranslateManager* manager =
      chrome_translate_client->GetTranslateManager();
  manager->TranslatePage(
      chrome_translate_client->GetLanguageState().original_language(), "en",
      true);

  SimulateURLFetch(kTestScriptInitializationError, true);

  // Wait for NOTIFICATION_PAGE_TRANSLATED notification.
  WaitUntilPageTranslated();

  EXPECT_TRUE(chrome_translate_client->GetLanguageState().translation_error());
  EXPECT_EQ(translate::TranslateErrors::INITIALIZATION_ERROR,
            GetPageTranslatedResult());
}

// Test the checks translate lib never gets ready and throws timeout.
IN_PROC_BROWSER_TEST_F(TranslateManagerBrowserTest,
                       PageTranslationTimeoutError) {
  ASSERT_TRUE(embedded_test_server()->Start());

  ChromeTranslateClient* chrome_translate_client = GetChromeTranslateClient();

  // There is a possible race condition, when the language is not yet detected,
  // so we check for that and wait if necessary.
  if (chrome_translate_client->GetLanguageState().original_language().empty())
    WaitUntilLanguageDetected();

  EXPECT_EQ("und",
            chrome_translate_client->GetLanguageState().original_language());

  // Open a new tab with a page in French.
  ResetObserver();
  AddTabAtIndex(0, GURL(embedded_test_server()->GetURL("/french_page.html")),
                ui::PAGE_TRANSITION_TYPED);
  chrome_translate_client = GetChromeTranslateClient();
  WaitUntilLanguageDetected();

  EXPECT_EQ("fr",
            chrome_translate_client->GetLanguageState().original_language());

  // Translate the page through TranslateManager.
  translate::TranslateManager* manager =
      chrome_translate_client->GetTranslateManager();
  manager->TranslatePage(
      chrome_translate_client->GetLanguageState().original_language(), "en",
      true);

  SimulateURLFetch(kTestScriptTimeout, true);

  // Wait for NOTIFICATION_PAGE_TRANSLATED notification.
  WaitUntilPageTranslated();

  EXPECT_TRUE(chrome_translate_client->GetLanguageState().translation_error());
  EXPECT_EQ(translate::TranslateErrors::TRANSLATION_TIMEOUT,
            GetPageTranslatedResult());
}

// Test the checks if both source and target languages mentioned are identical.
IN_PROC_BROWSER_TEST_F(TranslateManagerBrowserTest,
                       PageTranslationIdenticalLanguagesError) {
  ASSERT_TRUE(embedded_test_server()->Start());

  ChromeTranslateClient* chrome_translate_client = GetChromeTranslateClient();

  // There is a possible race condition, when the language is not yet detected,
  // so we check for that and wait if necessary.
  if (chrome_translate_client->GetLanguageState().original_language().empty())
    WaitUntilLanguageDetected();

  EXPECT_EQ("und",
            chrome_translate_client->GetLanguageState().original_language());

  // Open a new tab with a page in French.
  ResetObserver();
  AddTabAtIndex(0, GURL(embedded_test_server()->GetURL("/french_page.html")),
                ui::PAGE_TRANSITION_TYPED);
  chrome_translate_client = GetChromeTranslateClient();
  WaitUntilLanguageDetected();

  EXPECT_EQ("fr",
            chrome_translate_client->GetLanguageState().original_language());

  // Translate the page through TranslateManager.
  translate::TranslateManager* manager =
      chrome_translate_client->GetTranslateManager();
  manager->TranslatePage("aa", "en", true);

  SimulateURLFetch(kTestScriptIdenticalLanguages, true);

  // Wait for NOTIFICATION_PAGE_TRANSLATED notification.
  WaitUntilPageTranslated();

  EXPECT_TRUE(chrome_translate_client->GetLanguageState().translation_error());
  EXPECT_EQ(translate::TranslateErrors::IDENTICAL_LANGUAGES,
            GetPageTranslatedResult());
}

// Test if there was an error during translatePage script execution.
IN_PROC_BROWSER_TEST_F(TranslateManagerBrowserTest,
                       PageTranslationUnexpectedScriptError) {
  ASSERT_TRUE(embedded_test_server()->Start());

  ChromeTranslateClient* chrome_translate_client = GetChromeTranslateClient();

  // There is a possible race condition, when the language is not yet detected,
  // so we check for that and wait if necessary.
  if (chrome_translate_client->GetLanguageState().original_language().empty())
    WaitUntilLanguageDetected();

  EXPECT_EQ("und",
            chrome_translate_client->GetLanguageState().original_language());

  // Open a new tab with a page in French.
  ResetObserver();
  AddTabAtIndex(0, GURL(embedded_test_server()->GetURL("/french_page.html")),
                ui::PAGE_TRANSITION_TYPED);
  chrome_translate_client = GetChromeTranslateClient();
  WaitUntilLanguageDetected();

  EXPECT_EQ("fr",
            chrome_translate_client->GetLanguageState().original_language());

  // Translate the page through TranslateManager.
  translate::TranslateManager* manager =
      chrome_translate_client->GetTranslateManager();
  manager->TranslatePage(
      chrome_translate_client->GetLanguageState().original_language(), "en",
      true);

  SimulateURLFetch(kTestScriptUnexpectedScriptError, true);

  // Wait for NOTIFICATION_PAGE_TRANSLATED notification.
  WaitUntilPageTranslated();

  EXPECT_TRUE(chrome_translate_client->GetLanguageState().translation_error());
  EXPECT_EQ(translate::TranslateErrors::UNEXPECTED_SCRIPT_ERROR,
            GetPageTranslatedResult());
}

// Test if securityOrigin mentioned in url is valid.
IN_PROC_BROWSER_TEST_F(TranslateManagerBrowserTest,
                       PageTranslationBadOriginError) {
  ASSERT_TRUE(embedded_test_server()->Start());

  ChromeTranslateClient* chrome_translate_client = GetChromeTranslateClient();

  // There is a possible race condition, when the language is not yet detected,
  // so we check for that and wait if necessary.
  if (chrome_translate_client->GetLanguageState().original_language().empty())
    WaitUntilLanguageDetected();

  EXPECT_EQ("und",
            chrome_translate_client->GetLanguageState().original_language());

  // Open a new tab with a page in French.
  ResetObserver();
  AddTabAtIndex(0, GURL(embedded_test_server()->GetURL("/french_page.html")),
                ui::PAGE_TRANSITION_TYPED);
  chrome_translate_client = GetChromeTranslateClient();
  WaitUntilLanguageDetected();

  EXPECT_EQ("fr",
            chrome_translate_client->GetLanguageState().original_language());

  // Translate the page through TranslateManager.
  translate::TranslateManager* manager =
      chrome_translate_client->GetTranslateManager();
  manager->TranslatePage(
      chrome_translate_client->GetLanguageState().original_language(), "en",
      true);

  SimulateURLFetch(kTestScriptBadOrigin, true);

  // Wait for NOTIFICATION_PAGE_TRANSLATED notification.
  WaitUntilPageTranslated();

  EXPECT_TRUE(chrome_translate_client->GetLanguageState().translation_error());
  EXPECT_EQ(translate::TranslateErrors::BAD_ORIGIN, GetPageTranslatedResult());
}

// Test if there was an error during script load.
IN_PROC_BROWSER_TEST_F(TranslateManagerBrowserTest,
                       PageTranslationScriptLoadError) {
  ASSERT_TRUE(embedded_test_server()->Start());

  ChromeTranslateClient* chrome_translate_client = GetChromeTranslateClient();

  // There is a possible race condition, when the language is not yet detected,
  // so we check for that and wait if necessary.
  if (chrome_translate_client->GetLanguageState().original_language().empty())
    WaitUntilLanguageDetected();

  EXPECT_EQ("und",
            chrome_translate_client->GetLanguageState().original_language());

  // Open a new tab with a page in French.
  ResetObserver();
  AddTabAtIndex(0, GURL(embedded_test_server()->GetURL("/french_page.html")),
                ui::PAGE_TRANSITION_TYPED);
  chrome_translate_client = GetChromeTranslateClient();
  WaitUntilLanguageDetected();

  EXPECT_EQ("fr",
            chrome_translate_client->GetLanguageState().original_language());

  // Translate the page through TranslateManager.
  translate::TranslateManager* manager =
      chrome_translate_client->GetTranslateManager();
  manager->TranslatePage(
      chrome_translate_client->GetLanguageState().original_language(), "en",
      true);

  SimulateURLFetch(kTestScriptLoadError, true);

  // Wait for NOTIFICATION_PAGE_TRANSLATED notification.
  WaitUntilPageTranslated();

  EXPECT_TRUE(chrome_translate_client->GetLanguageState().translation_error());
  EXPECT_EQ(translate::TranslateErrors::SCRIPT_LOAD_ERROR,
            GetPageTranslatedResult());
}

// Test that session restore restores the translate infobar and other translate
// settings.
IN_PROC_BROWSER_TEST_F(TranslateManagerBrowserTest,
                       PRE_TranslateSessionRestore) {
  SessionStartupPref pref(SessionStartupPref::LAST);
  SessionStartupPref::SetStartupPref(browser()->profile(), pref);

  ChromeTranslateClient* chrome_translate_client = GetChromeTranslateClient();

  // There is a possible race condition, when the language is not yet detected,
  // so we check for that and wait if necessary.
  if (chrome_translate_client->GetLanguageState().original_language().empty())
    WaitUntilLanguageDetected();

  EXPECT_EQ("und",
            chrome_translate_client->GetLanguageState().original_language());

  ResetObserver();

  GURL french_url = ui_test_utils::GetTestUrl(
      base::FilePath(), base::FilePath(FILE_PATH_LITERAL("french_page.html")));
  ui_test_utils::NavigateToURL(browser(), french_url);

  WaitUntilLanguageDetected();
  EXPECT_EQ("fr",
            chrome_translate_client->GetLanguageState().original_language());
}

IN_PROC_BROWSER_TEST_F(TranslateManagerBrowserTest,
                       TranslateSessionRestore) {
  ChromeTranslateClient* active_translate_client = GetChromeTranslateClient();
  if (active_translate_client->GetLanguageState().current_language().empty())
    WaitUntilLanguageDetected();
  EXPECT_EQ("und",
            active_translate_client->GetLanguageState().current_language());

  // Make restored tab active to (on some platforms) initiate language
  // detection.
  browser()->tab_strip_model()->ActivateTabAt(0, true);

  content::WebContents* restored_web_contents =
      browser()->tab_strip_model()->GetWebContentsAt(0);
  ChromeTranslateClient* restored_translate_client =
      ChromeTranslateClient::FromWebContents(restored_web_contents);
  if (restored_translate_client->GetLanguageState()
          .current_language()
          .empty()) {
    ResetObserver();
    WaitUntilLanguageDetected();
  }
  EXPECT_EQ("fr",
            restored_translate_client->GetLanguageState().current_language());
}
