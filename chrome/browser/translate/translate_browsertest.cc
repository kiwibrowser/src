// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/base/ui_base_features.h"

// This entire test suite relies on the translate infobar which has been removed
// from Aura. The file should be ported to use the bubble.
#if !defined(USE_AURA) && !BUILDFLAG(MAC_VIEWS_BROWSER)

#include <stddef.h>

#include "base/command_line.h"
#include "base/files/file_path.h"
#include "base/macros.h"
#include "base/path_service.h"
#include "base/strings/string_util.h"
#include "base/strings/utf_string_conversions.h"
#include "base/test/scoped_feature_list.h"
#include "chrome/browser/infobars/infobar_observer.h"
#include "chrome/browser/infobars/infobar_service.h"
#include "chrome/browser/translate/translate_service.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/common/chrome_paths.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/test_switches.h"
#include "chrome/test/base/ui_test_utils.h"
#include "components/infobars/core/infobar.h"
#include "components/translate/core/browser/translate_infobar_delegate.h"
#include "components/translate/core/browser/translate_manager.h"
#include "components/translate/core/browser/translate_script.h"
#include "components/translate/core/common/translate_switches.h"
#include "content/public/test/browser_test_utils.h"
#include "net/http/http_status_code.h"
#include "net/test/embedded_test_server/embedded_test_server.h"
#include "net/url_request/test_url_fetcher_factory.h"
#include "net/url_request/url_fetcher_delegate.h"

namespace {

const base::FilePath::CharType kTranslateRoot[] =
    FILE_PATH_LITERAL("chrome/test/data/translate");
const char kFrenchTestPath[] = "/fr_test.html";
const char kBasicFrenchTestPath[] = "/basic_fr_test.html";
const char kRefreshMetaTagTestPath[] = "/refresh_meta_tag.html";
const char kRefreshMetaTagLongDelayTestPath[] = "/refresh_meta_tag_long.html";
const char kRefreshMetaTagCaseInsensitiveTestPath[] =
    "/refresh_meta_tag_casei.html";
const char kRefreshMetaTagAtOnloadTestPath[] =
    "/refresh_meta_tag_at_onload.html";
const char kUpdateLocationTestPath[] = "/update_location.html";
const char kUpdateLocationAtOnloadTestPath[] =
    "/update_location_at_onload.html";
const char kMainScriptPath[] = "/pseudo_main.js";
const char kElementMainScriptPath[] = "/pseudo_element_main.js";

};  // namespace

// Basic translate browser test with an embedded non-secure test server.
class TranslateBaseBrowserTest : public InProcessBrowserTest {
 public:
  TranslateBaseBrowserTest() {}

  void SetUp() override {
    // --secondary-ui-md forces the bubble to be enabled, so explicitly
    // disable it since this test deals with the infobar.
    scoped_feature_list_.InitAndDisableFeature(features::kSecondaryUiMd);
    set_open_about_blank_on_browser_launch(false);
    translate::TranslateManager::SetIgnoreMissingKeyForTesting(true);
    InProcessBrowserTest::SetUp();
  }

  void SetUpOnMainThread() override {
    net::EmbeddedTestServer* test_server = embedded_test_server();
    test_server->ServeFilesFromSourceDirectory(kTranslateRoot);
    ASSERT_TRUE(test_server->Start());
  }

 protected:
  GURL GetNonSecureURL(const std::string& path) const {
    return embedded_test_server()->GetURL(path);
  }

 private:
  base::test::ScopedFeatureList scoped_feature_list_;
  DISALLOW_COPY_AND_ASSIGN(TranslateBaseBrowserTest);
};

// Translate browser test for loading a page and checking infobar UI.
class TranslateBrowserTest : public TranslateBaseBrowserTest {
 public:
  TranslateBrowserTest() : infobar_service_(NULL) {}

 protected:
  translate::TranslateInfoBarDelegate* GetExistingTranslateInfoBarDelegate() {
    if (!infobar_service_) {
      content::WebContents* web_contents =
          browser()->tab_strip_model()->GetActiveWebContents();
      if (web_contents)
        infobar_service_ = InfoBarService::FromWebContents(web_contents);
    }
    if (!infobar_service_) {
      ADD_FAILURE() << "infobar service is not available";
      return NULL;
    }

    translate::TranslateInfoBarDelegate* delegate = NULL;
    for (size_t i = 0; i < infobar_service_->infobar_count(); ++i) {
      // Check if the shown infobar is a confirm infobar coming from the
      // |kTranslateSecurityOrigin| flag specified in SetUpCommandLine().
      // This infobar appears in all tests of TranslateBrowserTest and can be
      // ignored here.
      if (infobar_service_->infobar_at(i)->delegate()->
          AsConfirmInfoBarDelegate()) {
        continue;
      }

      translate::TranslateInfoBarDelegate* translate =
          infobar_service_->infobar_at(i)->delegate()->
          AsTranslateInfoBarDelegate();
      if (translate) {
        EXPECT_FALSE(delegate) << "multiple infobars are shown unexpectedly";
        delegate = translate;
        continue;
      }

      // Other infobar should not be shown.
      EXPECT_TRUE(delegate);
    }
    return delegate;
  }

  void LoadPageWithInfobar(const std::string& path) {
    // Setup infobar observer.
    InfoBarObserver observer(infobar_service_,
                             InfoBarObserver::Type::kInfoBarAdded);

    // Visit non-secure page which is going to be translated.
    ui_test_utils::NavigateToURL(browser(), GetNonSecureURL(path));

    // Wait for Chrome Translate infobar.
    observer.Wait();
  }

  void LoadPageWithoutInfobar(const std::string& path) {
    // If infobar won't be triggered, we need to use another observer.
    // Setup page title observer.
    content::WebContents* web_contents =
        browser()->tab_strip_model()->GetActiveWebContents();
    ASSERT_TRUE(web_contents);
    content::TitleWatcher watcher(web_contents, base::ASCIIToUTF16("PASS"));
    watcher.AlsoWaitForTitle(base::ASCIIToUTF16("FAIL"));

    // Visit non-secure page which is going to be translated.
    ui_test_utils::NavigateToURL(browser(), GetNonSecureURL(path));

    // Wait for the page title is changed after the test finished.
    const base::string16 result = watcher.WaitAndGetTitle();
    EXPECT_EQ("PASS", base::UTF16ToASCII(result));
  }

  void CheckForTranslateUI(const std::string& path, bool should_trigger_UI) {
    ASSERT_FALSE(TranslateService::IsTranslateBubbleEnabled());
    // Check if there is no Translate infobar.
    translate::TranslateInfoBarDelegate* delegate =
        GetExistingTranslateInfoBarDelegate();
    EXPECT_FALSE(delegate);

    if (should_trigger_UI) {
      ASSERT_NO_FATAL_FAILURE(LoadPageWithInfobar(path));
    } else {
      ASSERT_NO_FATAL_FAILURE(LoadPageWithoutInfobar(path));
    }

    // Check whether the translate infobar showed up.
    delegate = GetExistingTranslateInfoBarDelegate();
    if (should_trigger_UI) {
      // Check if there is a translate infobar.
      EXPECT_TRUE(delegate);
    } else {
      // Check if there is no translate infobar.
      EXPECT_FALSE(delegate);
    }
  }

  void Translate() {
    translate::TranslateInfoBarDelegate* delegate =
        GetExistingTranslateInfoBarDelegate();
    ASSERT_TRUE(delegate);
    delegate->Translate();
  }

 private:
  InfoBarService* infobar_service_;
};

// Translate browser test with a seperate secure server setup.
class TranslateWithSecureServerBrowserTest : public TranslateBrowserTest {
 public:
  TranslateWithSecureServerBrowserTest()
      : https_server_(net::EmbeddedTestServer::TYPE_HTTPS) {
    https_server_.ServeFilesFromSourceDirectory(kTranslateRoot);
  }

  void SetUpCommandLine(base::CommandLine* command_line) override {
    ASSERT_TRUE(https_server_.Start());
    // Setup alternate security origin for testing in order to allow XHR against
    // local test server. Note that this flag shows a confirm infobar in tests.
    GURL base_url = GetSecureURL("/");
    command_line->AppendSwitchASCII(
        translate::switches::kTranslateSecurityOrigin,
        base_url.GetOrigin().spec());
    TranslateBrowserTest::SetUpCommandLine(command_line);
  }

 protected:
  GURL GetSecureURL(const std::string& path) const {
    return https_server_.GetURL(path);
  }

 private:
  net::EmbeddedTestServer https_server_;

  DISALLOW_COPY_AND_ASSIGN(TranslateWithSecureServerBrowserTest);
};

IN_PROC_BROWSER_TEST_F(TranslateWithSecureServerBrowserTest,
                       TranslateInIsolatedWorld) {
  net::TestURLFetcherFactory factory;
  ASSERT_NO_FATAL_FAILURE(CheckForTranslateUI(kFrenchTestPath, true));

  // Setup page title observer.
  content::WebContents* web_contents =
      browser()->tab_strip_model()->GetActiveWebContents();
  ASSERT_TRUE(web_contents);
  content::TitleWatcher watcher(web_contents, base::ASCIIToUTF16("PASS"));
  watcher.AlsoWaitForTitle(base::ASCIIToUTF16("FAIL"));

  // Perform translate.
  ASSERT_NO_FATAL_FAILURE(Translate());

  // Hook URLFetcher for element.js.
  GURL script1_url = GetSecureURL(kMainScriptPath);
  GURL script2_url = GetSecureURL(kElementMainScriptPath);
  std::string element_js = "main_script_url = '" + script1_url.spec() + "';\n";
  element_js += "element_main_script_url = '" + script2_url.spec() + "';\n";
  element_js +=
    "google = { 'translate' : { 'TranslateService' : function() { return {\n"
    "  isAvailable: function() {\n"
    "    cr.googleTranslate.onLoadJavascript(main_script_url);\n"
    "    return true;\n"
    "  },\n"
    "  translatePage: function(sl, tl, cb) {\n"
    "    cb(1, true);\n"
    "  }\n"
    "} } } };\n"
    "cr.googleTranslate.onTranslateElementLoad();\n";
  net::TestURLFetcher* fetcher =
      factory.GetFetcherByID(translate::TranslateScript::kFetcherId);
  ASSERT_TRUE(fetcher);
  fetcher->set_status(net::URLRequestStatus());
  fetcher->set_url(fetcher->GetOriginalURL());
  fetcher->set_response_code(net::HTTP_OK);
  fetcher->SetResponseString(element_js);
  fetcher->delegate()->OnURLFetchComplete(fetcher);

  // Wait for the page title is changed after the test finished.
  const base::string16 result = watcher.WaitAndGetTitle();
  EXPECT_EQ("PASS", base::UTF16ToASCII(result));
}

IN_PROC_BROWSER_TEST_F(TranslateBrowserTest, BasicTranslation) {
  ASSERT_NO_FATAL_FAILURE(CheckForTranslateUI(kBasicFrenchTestPath, true));
}

IN_PROC_BROWSER_TEST_F(TranslateBrowserTest, IgnoreRefreshMetaTag) {
  ASSERT_NO_FATAL_FAILURE(CheckForTranslateUI(
      kRefreshMetaTagTestPath, false));
}

IN_PROC_BROWSER_TEST_F(TranslateBrowserTest, TranslateRefreshMetaTagLongDelay) {
  ASSERT_NO_FATAL_FAILURE(
      CheckForTranslateUI(kRefreshMetaTagLongDelayTestPath, true));
}

IN_PROC_BROWSER_TEST_F(TranslateBrowserTest,
                       IgnoreRefreshMetaTagInCaseInsensitive) {
  ASSERT_NO_FATAL_FAILURE(CheckForTranslateUI(
      kRefreshMetaTagCaseInsensitiveTestPath, false));
}

IN_PROC_BROWSER_TEST_F(TranslateBrowserTest, IgnoreRefreshMetaTagAtOnload) {
  ASSERT_NO_FATAL_FAILURE(CheckForTranslateUI(
      kRefreshMetaTagAtOnloadTestPath, false));
}

// TODO(toyoshim, creis): The infobar should be dismissed on client redirects.
// See https://crbug.com/781879.
IN_PROC_BROWSER_TEST_F(TranslateBrowserTest, DISABLED_UpdateLocation) {
  ASSERT_NO_FATAL_FAILURE(CheckForTranslateUI(kUpdateLocationTestPath, false));
}

// TODO(toyoshim, creis): The infobar should be dismissed on client redirects.
// See https://crbug.com/781879.
IN_PROC_BROWSER_TEST_F(TranslateBrowserTest, DISABLED_UpdateLocationAtOnload) {
  ASSERT_NO_FATAL_FAILURE(
      CheckForTranslateUI(kUpdateLocationAtOnloadTestPath, false));
}
#endif  // !defined(USE_AURA)
