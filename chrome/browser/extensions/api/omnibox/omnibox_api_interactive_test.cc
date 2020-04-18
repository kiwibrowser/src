// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/strings/utf_string_conversions.h"
#include "build/build_config.h"
#include "chrome/browser/autocomplete/chrome_autocomplete_scheme_classifier.h"
#include "chrome/browser/extensions/api/omnibox/omnibox_api_testbase.h"
#include "chrome/browser/search_engines/template_url_service_factory.h"
#include "chrome/browser/ui/browser_commands.h"
#include "chrome/browser/ui/view_ids.h"
#include "chrome/test/base/interactive_test_utils.h"
#include "chrome/test/base/search_test_utils.h"
#include "extensions/test/extension_test_message_listener.h"
#include "extensions/test/result_catcher.h"
#include "third_party/metrics_proto/omnibox_event.pb.h"

namespace {

void InputKeys(Browser* browser, const std::vector<ui::KeyboardCode>& keys) {
  for (auto key : keys) {
    // Note that sending key presses can be flaky at times.
    ASSERT_TRUE(ui_test_utils::SendKeyPressSync(browser, key, false, false,
                                                false, false));
  }
}

}  // namespace

// Tests that the autocomplete popup doesn't reopen after accepting input for
// a given query.
// http://crbug.com/88552
IN_PROC_BROWSER_TEST_F(OmniboxApiTest, PopupStaysClosed) {
  ASSERT_TRUE(RunExtensionTest("omnibox")) << message_;

  // The results depend on the TemplateURLService being loaded. Make sure it is
  // loaded so that the autocomplete results are consistent.
  Profile* profile = browser()->profile();
  search_test_utils::WaitForTemplateURLServiceToLoad(
      TemplateURLServiceFactory::GetForProfile(profile));

  LocationBar* location_bar = GetLocationBar(browser());
  OmniboxView* omnibox_view = location_bar->GetOmniboxView();
  AutocompleteController* autocomplete_controller =
      GetAutocompleteController(browser());
  OmniboxPopupModel* popup_model = omnibox_view->model()->popup_model();

  // Input a keyword query and wait for suggestions from the extension.
  omnibox_view->OnBeforePossibleChange();
  omnibox_view->SetUserText(base::ASCIIToUTF16("kw comman"));
  omnibox_view->OnAfterPossibleChange(true);
  WaitForAutocompleteDone(autocomplete_controller);
  EXPECT_TRUE(autocomplete_controller->done());
  EXPECT_TRUE(popup_model->IsOpen());

  // Quickly type another query and accept it before getting suggestions back
  // for the query. The popup will close after accepting input - ensure that it
  // does not reopen when the extension returns its suggestions.
  extensions::ResultCatcher catcher;

  // TODO: Rather than send this second request by talking to the controller
  // directly, figure out how to send it via the proper calls to
  // location_bar or location_bar->().
  AutocompleteInput input(base::ASCIIToUTF16("kw command"),
                          metrics::OmniboxEventProto::NTP,
                          ChromeAutocompleteSchemeClassifier(profile));
  autocomplete_controller->Start(input);
  location_bar->AcceptInput();
  WaitForAutocompleteDone(autocomplete_controller);
  EXPECT_TRUE(autocomplete_controller->done());
  // This checks that the keyword provider (via javascript)
  // gets told to navigate to the string "command".
  EXPECT_TRUE(catcher.GetNextResult()) << catcher.message();
  EXPECT_FALSE(popup_model->IsOpen());
}

// Tests deleting a deletable omnibox extension suggestion result.
// Flaky on Windows. https://crbug.com/801316
#if defined(OS_WIN)
#define MAYBE_DeleteOmniboxSuggestionResult \
  DISABLED_DeleteOmniboxSuggestionResult
#else
#define MAYBE_DeleteOmniboxSuggestionResult DeleteOmniboxSuggestionResult
#endif
IN_PROC_BROWSER_TEST_F(OmniboxApiTest, MAYBE_DeleteOmniboxSuggestionResult) {
  ASSERT_TRUE(RunExtensionTest("omnibox")) << message_;

  // The results depend on the TemplateURLService being loaded. Make sure it is
  // loaded so that the autocomplete results are consistent.
  Profile* profile = browser()->profile();
  search_test_utils::WaitForTemplateURLServiceToLoad(
      TemplateURLServiceFactory::GetForProfile(profile));

  AutocompleteController* autocomplete_controller =
      GetAutocompleteController(browser());

  chrome::FocusLocationBar(browser());
  ASSERT_TRUE(ui_test_utils::IsViewFocused(browser(), VIEW_ID_OMNIBOX));

  // Input a keyword query and wait for suggestions from the extension.
  InputKeys(browser(), {ui::VKEY_K, ui::VKEY_W, ui::VKEY_SPACE, ui::VKEY_D});

  WaitForAutocompleteDone(autocomplete_controller);
  EXPECT_TRUE(autocomplete_controller->done());

  // Peek into the controller to see if it has the results we expect.
  const AutocompleteResult& result = autocomplete_controller->result();
  ASSERT_EQ(4U, result.size()) << AutocompleteResultAsString(result);

  EXPECT_EQ(base::ASCIIToUTF16("kw d"), result.match_at(0).fill_into_edit);
  EXPECT_EQ(AutocompleteProvider::TYPE_KEYWORD,
            result.match_at(0).provider->type());
  EXPECT_FALSE(result.match_at(0).deletable);

  EXPECT_EQ(base::ASCIIToUTF16("kw n1"), result.match_at(1).fill_into_edit);
  EXPECT_EQ(AutocompleteProvider::TYPE_KEYWORD,
            result.match_at(1).provider->type());
  // Verify that the first omnibox extension suggestion is deletable.
  EXPECT_TRUE(result.match_at(1).deletable);

  EXPECT_EQ(base::ASCIIToUTF16("kw n2"), result.match_at(2).fill_into_edit);
  EXPECT_EQ(AutocompleteProvider::TYPE_KEYWORD,
            result.match_at(2).provider->type());
  // Verify that the second omnibox extension suggestion is not deletable.
  EXPECT_FALSE(result.match_at(2).deletable);

  EXPECT_EQ(base::ASCIIToUTF16("kw d"), result.match_at(3).fill_into_edit);
  EXPECT_EQ(AutocompleteMatchType::SEARCH_WHAT_YOU_TYPED,
            result.match_at(3).type);
  EXPECT_FALSE(result.match_at(3).deletable);

// This test portion is excluded from Mac because the Mac key combination
// FN+SHIFT+DEL used to delete an omnibox suggestion cannot be reproduced.
// This is because the FN key is not supported in interactive_test_util.h.
#if !defined(OS_MACOSX)
  ExtensionTestMessageListener delete_suggestion_listener(
      "onDeleteSuggestion: des1", false);

  // Skip the first suggestion result.
  EXPECT_TRUE(ui_test_utils::SendKeyPressSync(browser(), ui::VKEY_DOWN, false,
                                              false, false, false));
  // Delete the second suggestion result. On Linux, this is done via SHIFT+DEL.
  EXPECT_TRUE(ui_test_utils::SendKeyPressSync(browser(), ui::VKEY_DELETE, false,
                                              true, false, false));
  // Verify that the onDeleteSuggestion event was fired.
  ASSERT_TRUE(delete_suggestion_listener.WaitUntilSatisfied());

  // Verify that the first suggestion result was deleted. There should be one
  // less suggestion result, 3 now instead of 4.
  ASSERT_EQ(3U, result.size());
  EXPECT_EQ(base::ASCIIToUTF16("kw d"), result.match_at(0).fill_into_edit);
  EXPECT_EQ(base::ASCIIToUTF16("kw n2"), result.match_at(1).fill_into_edit);
  EXPECT_EQ(base::ASCIIToUTF16("kw d"), result.match_at(2).fill_into_edit);
#endif
}

// Tests typing something but not staying in keyword mode.
IN_PROC_BROWSER_TEST_F(OmniboxApiTest, ExtensionSuggestionsOnlyInKeywordMode) {
  ASSERT_TRUE(RunExtensionTest("omnibox")) << message_;

  // The results depend on the TemplateURLService being loaded. Make sure it is
  // loaded so that the autocomplete results are consistent.
  Profile* profile = browser()->profile();
  search_test_utils::WaitForTemplateURLServiceToLoad(
      TemplateURLServiceFactory::GetForProfile(profile));

  AutocompleteController* autocomplete_controller =
      GetAutocompleteController(browser());

  chrome::FocusLocationBar(browser());
  ASSERT_TRUE(ui_test_utils::IsViewFocused(browser(), VIEW_ID_OMNIBOX));

  // Input a keyword query and wait for suggestions from the extension.
  InputKeys(browser(), {ui::VKEY_K, ui::VKEY_W, ui::VKEY_SPACE, ui::VKEY_D});
  WaitForAutocompleteDone(autocomplete_controller);
  EXPECT_TRUE(autocomplete_controller->done());

  // Peek into the controller to see if it has the results we expect.
  {
    const AutocompleteResult& result = autocomplete_controller->result();
    ASSERT_EQ(4U, result.size()) << AutocompleteResultAsString(result);

    EXPECT_EQ(base::ASCIIToUTF16("kw d"), result.match_at(0).fill_into_edit);
    EXPECT_EQ(AutocompleteProvider::TYPE_KEYWORD,
              result.match_at(0).provider->type());

    EXPECT_EQ(base::ASCIIToUTF16("kw n1"), result.match_at(1).fill_into_edit);
    EXPECT_EQ(AutocompleteProvider::TYPE_KEYWORD,
              result.match_at(1).provider->type());

    EXPECT_EQ(base::ASCIIToUTF16("kw n2"), result.match_at(2).fill_into_edit);
    EXPECT_EQ(AutocompleteProvider::TYPE_KEYWORD,
              result.match_at(2).provider->type());

    EXPECT_EQ(base::ASCIIToUTF16("kw d"), result.match_at(3).fill_into_edit);
    EXPECT_EQ(AutocompleteMatchType::SEARCH_WHAT_YOU_TYPED,
              result.match_at(3).type);
  }

  // Now clear the omnibox by pressing escape multiple times, focus the
  // omnibox, then type the same text again except with a backspace after
  // the "kw ".  This will cause leaving keyword mode so the full text will be
  // "kw d" without a keyword chip displayed.  This middle step of focussing
  // the omnibox is necessary because on Mac pressing escape can make the
  // omnibox lose focus.
  InputKeys(browser(), {ui::VKEY_ESCAPE, ui::VKEY_ESCAPE});
  chrome::FocusLocationBar(browser());
  ASSERT_TRUE(ui_test_utils::IsViewFocused(browser(), VIEW_ID_OMNIBOX));
  InputKeys(browser(), {ui::VKEY_K, ui::VKEY_W, ui::VKEY_SPACE, ui::VKEY_BACK,
                        ui::VKEY_D});

  WaitForAutocompleteDone(autocomplete_controller);
  EXPECT_TRUE(autocomplete_controller->done());

  // Peek into the controller to see if it has the results we expect.  Since
  // the user left keyword mode, the extension should not be the top-ranked
  // match nor should it have provided suggestions.  (It can and should provide
  // a match to query the extension for exactly what the user typed however.)
  {
    const AutocompleteResult& result = autocomplete_controller->result();
    ASSERT_EQ(2U, result.size()) << AutocompleteResultAsString(result);

    EXPECT_EQ(base::ASCIIToUTF16("kw d"), result.match_at(0).fill_into_edit);
    EXPECT_EQ(AutocompleteMatchType::SEARCH_WHAT_YOU_TYPED,
              result.match_at(0).type);

    EXPECT_EQ(base::ASCIIToUTF16("kw d"), result.match_at(1).fill_into_edit);
    EXPECT_EQ(AutocompleteProvider::TYPE_KEYWORD,
              result.match_at(1).provider->type());
  }
}
