// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <string>

#include "base/base_switches.h"
#include "base/command_line.h"
#include "base/files/file_path.h"
#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "base/run_loop.h"
#include "base/strings/string_util.h"
#include "base/strings/utf_string_conversions.h"
#include "build/build_config.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/extensions/component_loader.h"
#include "chrome/browser/first_run/first_run.h"
#include "chrome/browser/importer/importer_list.h"
#include "chrome/browser/prefs/chrome_pref_service_factory.h"
#include "chrome/browser/profiles/profile_manager.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/common/chrome_switches.h"
#include "chrome/common/pref_names.h"
#include "chrome/common/url_constants.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/ui_test_utils.h"
#include "components/prefs/pref_service.h"
#include "components/user_prefs/user_prefs.h"
#include "content/public/browser/web_contents.h"
#include "content/public/common/content_switches.h"
#include "content/public/test/browser_test_utils.h"
#include "content/public/test/test_launcher.h"
#include "testing/gtest/include/gtest/gtest.h"

typedef InProcessBrowserTest FirstRunBrowserTest;

IN_PROC_BROWSER_TEST_F(FirstRunBrowserTest, SetShouldShowWelcomePage) {
  EXPECT_FALSE(first_run::ShouldShowWelcomePage());
  first_run::SetShouldShowWelcomePage();
  EXPECT_TRUE(first_run::ShouldShowWelcomePage());
  EXPECT_FALSE(first_run::ShouldShowWelcomePage());
}

#if !defined(OS_CHROMEOS)
namespace {

// A generic test class to be subclassed by test classes testing specific
// master_preferences. All subclasses must call SetMasterPreferencesForTest()
// from their SetUp() method before deferring the remainder of Setup() to this
// class.
class FirstRunMasterPrefsBrowserTestBase : public InProcessBrowserTest {
 public:
  FirstRunMasterPrefsBrowserTestBase() {}

 protected:
  void SetUp() override {
    // All users of this test class need to call SetMasterPreferencesForTest()
    // before this class' SetUp() is invoked.
    ASSERT_TRUE(text_.get());

    ASSERT_TRUE(base::CreateTemporaryFile(&prefs_file_));
    EXPECT_EQ(static_cast<int>(text_->size()),
              base::WriteFile(prefs_file_, text_->c_str(), text_->size()));
    first_run::SetMasterPrefsPathForTesting(prefs_file_);

    // This invokes BrowserMain, and does the import, so must be done last.
    InProcessBrowserTest::SetUp();
  }

  void TearDown() override {
    EXPECT_TRUE(base::DeleteFile(prefs_file_, false));
    InProcessBrowserTest::TearDown();
  }

  void SetUpCommandLine(base::CommandLine* command_line) override {
    command_line->AppendSwitch(switches::kForceFirstRun);
    EXPECT_EQ(first_run::AUTO_IMPORT_NONE, first_run::auto_import_state());

    extensions::ComponentLoader::EnableBackgroundExtensionsForTesting();
  }

  void SetMasterPreferencesForTest(const char text[]) {
    text_.reset(new std::string(text));
  }

 private:
  base::FilePath prefs_file_;
  std::unique_ptr<std::string> text_;

  DISALLOW_COPY_AND_ASSIGN(FirstRunMasterPrefsBrowserTestBase);
};

template<const char Text[]>
class FirstRunMasterPrefsBrowserTestT
    : public FirstRunMasterPrefsBrowserTestBase {
 public:
  FirstRunMasterPrefsBrowserTestT() {}

 protected:
  void SetUp() override {
    SetMasterPreferencesForTest(Text);
    FirstRunMasterPrefsBrowserTestBase::SetUp();
  }

 private:
  DISALLOW_COPY_AND_ASSIGN(FirstRunMasterPrefsBrowserTestT);
};

// Returns the true expected import state, derived from the original
// |expected_import_state|, for the current test machine's configuration. Some
// bot configurations do not have another profile (browser) to import from and
// thus the import must not be expected to have occurred.
int MaskExpectedImportState(int expected_import_state) {
  std::unique_ptr<ImporterList> importer_list(new ImporterList());
  base::RunLoop run_loop;
  importer_list->DetectSourceProfiles(
      g_browser_process->GetApplicationLocale(),
      false,  // include_interactive_profiles?
      run_loop.QuitClosure());
  run_loop.Run();
  int source_profile_count = importer_list->count();
#if defined(OS_WIN)
  // On Windows, the importer's DetectIEProfiles() will always add to the count.
  // Internet Explorer always exists and always has something to import.
  EXPECT_GT(source_profile_count, 0);
#endif
  if (source_profile_count == 0)
    return expected_import_state & ~first_run::AUTO_IMPORT_PROFILE_IMPORTED;
  return expected_import_state;
}

}  // namespace

extern const char kImportDefault[] =
    "{\n"
    "}\n";
typedef FirstRunMasterPrefsBrowserTestT<kImportDefault>
    FirstRunMasterPrefsImportDefault;
// http://crbug.com/314221
#if defined(OS_MACOSX) || (defined(GOOGLE_CHROME_BUILD) && defined(OS_LINUX))
#define MAYBE_ImportDefault DISABLED_ImportDefault
#else
#define MAYBE_ImportDefault ImportDefault
#endif
// No items are imported by default.
IN_PROC_BROWSER_TEST_F(FirstRunMasterPrefsImportDefault, MAYBE_ImportDefault) {
  int auto_import_state = first_run::auto_import_state();
  EXPECT_EQ(MaskExpectedImportState(first_run::AUTO_IMPORT_CALLED),
            auto_import_state);
}

extern const char kImportAll[] =
    "{\n"
    "  \"distribution\": {\n"
    "    \"import_bookmarks\": true,\n"
    "    \"import_history\": true,\n"
    "    \"import_home_page\": true,\n"
    "    \"import_search_engine\": true\n"
    "  }\n"
    "}\n";
typedef FirstRunMasterPrefsBrowserTestT<kImportAll>
    FirstRunMasterPrefsImportAll;
// http://crbug.com/314221
#if defined(OS_MACOSX) || (defined(GOOGLE_CHROME_BUILD) && defined(OS_LINUX))
#define MAYBE_ImportAll DISABLED_ImportAll
#else
#define MAYBE_ImportAll ImportAll
#endif
IN_PROC_BROWSER_TEST_F(FirstRunMasterPrefsImportAll, MAYBE_ImportAll) {
  int auto_import_state = first_run::auto_import_state();
  EXPECT_EQ(MaskExpectedImportState(first_run::AUTO_IMPORT_CALLED |
                                    first_run::AUTO_IMPORT_PROFILE_IMPORTED),
            auto_import_state);
}

// The bookmarks file doesn't actually need to exist for this integration test
// to trigger the interaction being tested.
extern const char kImportBookmarksFile[] =
    "{\n"
    "  \"distribution\": {\n"
    "     \"import_bookmarks_from_file\": \"/foo/doesntexists.wtv\"\n"
    "  }\n"
    "}\n";
typedef FirstRunMasterPrefsBrowserTestT<kImportBookmarksFile>
    FirstRunMasterPrefsImportBookmarksFile;
// http://crbug.com/314221
#if (defined(GOOGLE_CHROME_BUILD) && defined(OS_LINUX)) || defined(OS_MACOSX)
#define MAYBE_ImportBookmarksFile DISABLED_ImportBookmarksFile
#else
#define MAYBE_ImportBookmarksFile ImportBookmarksFile
#endif
IN_PROC_BROWSER_TEST_F(FirstRunMasterPrefsImportBookmarksFile,
                       MAYBE_ImportBookmarksFile) {
  int auto_import_state = first_run::auto_import_state();
  EXPECT_EQ(
      MaskExpectedImportState(first_run::AUTO_IMPORT_CALLED |
                              first_run::AUTO_IMPORT_BOOKMARKS_FILE_IMPORTED),
      auto_import_state);
}

// Test an import with all import options disabled. This is a regression test
// for http://crbug.com/169984 where this would cause the import process to
// stay running, and the NTP to be loaded with no apps.
extern const char kImportNothing[] =
    "{\n"
    "  \"distribution\": {\n"
    "    \"import_bookmarks\": false,\n"
    "    \"import_history\": false,\n"
    "    \"import_home_page\": false,\n"
    "    \"import_search_engine\": false\n"
    "  }\n"
    "}\n";
typedef FirstRunMasterPrefsBrowserTestT<kImportNothing>
    FirstRunMasterPrefsImportNothing;
// http://crbug.com/314221
#if defined(GOOGLE_CHROME_BUILD) && (defined(OS_MACOSX) || defined(OS_LINUX))
#define MAYBE_ImportNothingAndShowNewTabPage \
    DISABLED_ImportNothingAndShowNewTabPage
#else
#define MAYBE_ImportNothingAndShowNewTabPage ImportNothingAndShowNewTabPage
#endif
IN_PROC_BROWSER_TEST_F(FirstRunMasterPrefsImportNothing,
                       MAYBE_ImportNothingAndShowNewTabPage) {
  EXPECT_EQ(first_run::AUTO_IMPORT_CALLED, first_run::auto_import_state());
  ui_test_utils::NavigateToURL(browser(), GURL(chrome::kChromeUINewTabURL));
  content::WebContents* tab = browser()->tab_strip_model()->GetWebContentsAt(0);
  EXPECT_TRUE(WaitForLoadStop(tab));
}

// Test first run with some tracked preferences.
extern const char kWithTrackedPrefs[] =
    "{\n"
    "  \"homepage\": \"example.com\",\n"
    "  \"homepage_is_newtabpage\": false\n"
    "}\n";
// A test fixture that will run in a first run scenario with master_preferences
// set to kWithTrackedPrefs. Parameterizable on the SettingsEnforcement
// experiment to be forced.
class FirstRunMasterPrefsWithTrackedPreferences
    : public FirstRunMasterPrefsBrowserTestT<kWithTrackedPrefs>,
      public testing::WithParamInterface<std::string> {
 public:
  FirstRunMasterPrefsWithTrackedPreferences() {}

 protected:
  void SetUpCommandLine(base::CommandLine* command_line) override {
    FirstRunMasterPrefsBrowserTestT::SetUpCommandLine(command_line);
    command_line->AppendSwitchASCII(
        switches::kForceFieldTrials,
        std::string(chrome_prefs::internals::kSettingsEnforcementTrialName) +
            "/" + GetParam() + "/");
  }

  void SetUpInProcessBrowserTestFixture() override {
    FirstRunMasterPrefsBrowserTestT::SetUpInProcessBrowserTestFixture();

    // Bots are on a domain, turn off the domain check for settings hardening in
    // order to be able to test all SettingsEnforcement groups.
    chrome_prefs::DisableDomainCheckForTesting();
  }

 private:
  DISALLOW_COPY_AND_ASSIGN(FirstRunMasterPrefsWithTrackedPreferences);
};

// http://crbug.com/314221
#if defined(GOOGLE_CHROME_BUILD) && (defined(OS_MACOSX) || defined(OS_LINUX))
#define MAYBE_TrackedPreferencesSurviveFirstRun \
    DISABLED_TrackedPreferencesSurviveFirstRun
#else
#define MAYBE_TrackedPreferencesSurviveFirstRun \
    TrackedPreferencesSurviveFirstRun
#endif
IN_PROC_BROWSER_TEST_P(FirstRunMasterPrefsWithTrackedPreferences,
                       MAYBE_TrackedPreferencesSurviveFirstRun) {
  const PrefService* user_prefs = browser()->profile()->GetPrefs();
  EXPECT_EQ("example.com", user_prefs->GetString(prefs::kHomePage));
  EXPECT_FALSE(user_prefs->GetBoolean(prefs::kHomePageIsNewTabPage));

  // The test for kHomePageIsNewTabPage above relies on the fact that true is
  // the default (hence false must be the user's pref); ensure this fact remains
  // true.
  const base::Value* default_homepage_is_ntp_value =
      user_prefs->GetDefaultPrefValue(prefs::kHomePageIsNewTabPage);
  bool default_homepage_is_ntp = false;
  EXPECT_TRUE(
      default_homepage_is_ntp_value->GetAsBoolean(&default_homepage_is_ntp));
  EXPECT_TRUE(default_homepage_is_ntp);
}

INSTANTIATE_TEST_CASE_P(
    FirstRunMasterPrefsWithTrackedPreferencesInstance,
    FirstRunMasterPrefsWithTrackedPreferences,
    testing::Values(
        chrome_prefs::internals::kSettingsEnforcementGroupNoEnforcement,
        chrome_prefs::internals::kSettingsEnforcementGroupEnforceAlways,
        chrome_prefs::internals::
            kSettingsEnforcementGroupEnforceAlwaysWithDSE,
        chrome_prefs::internals::
            kSettingsEnforcementGroupEnforceAlwaysWithExtensionsAndDSE));

#endif  // !defined(OS_CHROMEOS)
