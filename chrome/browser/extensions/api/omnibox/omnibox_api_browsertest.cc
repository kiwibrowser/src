// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/strings/string16.h"
#include "base/strings/utf_string_conversions.h"
#include "chrome/browser/autocomplete/chrome_autocomplete_scheme_classifier.h"
#include "chrome/browser/extensions/api/omnibox/omnibox_api_testbase.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/search_engines/template_url_service_factory.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/location_bar/location_bar.h"
#include "chrome/test/base/search_test_utils.h"
#include "components/omnibox/browser/autocomplete_controller.h"
#include "components/omnibox/browser/autocomplete_input.h"
#include "components/omnibox/browser/autocomplete_match.h"
#include "components/omnibox/browser/autocomplete_result.h"
#include "components/omnibox/browser/omnibox_view.h"
#include "extensions/test/result_catcher.h"
#include "third_party/metrics_proto/omnibox_event.pb.h"
#include "ui/base/window_open_disposition.h"

using base::ASCIIToUTF16;
using extensions::ResultCatcher;
using metrics::OmniboxEventProto;

// http://crbug.com/167158
IN_PROC_BROWSER_TEST_F(OmniboxApiTest, DISABLED_Basic) {
  ASSERT_TRUE(RunExtensionTest("omnibox")) << message_;

  // The results depend on the TemplateURLService being loaded. Make sure it is
  // loaded so that the autocomplete results are consistent.
  Profile* profile = browser()->profile();
  search_test_utils::WaitForTemplateURLServiceToLoad(
      TemplateURLServiceFactory::GetForProfile(profile));

  AutocompleteController* autocomplete_controller =
      GetAutocompleteController(browser());

  // Test that our extension's keyword is suggested to us when we partially type
  // it.
  {
    AutocompleteInput input(ASCIIToUTF16("keywor"),
                            metrics::OmniboxEventProto::NTP,
                            ChromeAutocompleteSchemeClassifier(profile));
    autocomplete_controller->Start(input);
    WaitForAutocompleteDone(autocomplete_controller);
    EXPECT_TRUE(autocomplete_controller->done());

    // Now, peek into the controller to see if it has the results we expect.
    // First result should be to search for what was typed, second should be to
    // enter "extension keyword" mode.
    const AutocompleteResult& result = autocomplete_controller->result();
    ASSERT_EQ(2U, result.size()) << AutocompleteResultAsString(result);
    AutocompleteMatch match = result.match_at(0);
    EXPECT_EQ(AutocompleteMatchType::SEARCH_WHAT_YOU_TYPED, match.type);
    EXPECT_FALSE(match.deletable);

    match = result.match_at(1);
    EXPECT_EQ(ASCIIToUTF16("kw"), match.keyword);
  }

  // Test that our extension can send suggestions back to us.
  {
    AutocompleteInput input(ASCIIToUTF16("kw suggestio"),
                            metrics::OmniboxEventProto::NTP,
                            ChromeAutocompleteSchemeClassifier(profile));
    autocomplete_controller->Start(input);
    WaitForAutocompleteDone(autocomplete_controller);
    EXPECT_TRUE(autocomplete_controller->done());

    // Now, peek into the controller to see if it has the results we expect.
    // First result should be to invoke the keyword with what we typed, 2-4
    // should be to invoke with suggestions from the extension, and the last
    // should be to search for what we typed.
    const AutocompleteResult& result = autocomplete_controller->result();
    ASSERT_EQ(5U, result.size()) << AutocompleteResultAsString(result);

    EXPECT_EQ(ASCIIToUTF16("kw"), result.match_at(0).keyword);
    EXPECT_EQ(ASCIIToUTF16("kw suggestio"), result.match_at(0).fill_into_edit);
    EXPECT_EQ(AutocompleteMatchType::SEARCH_OTHER_ENGINE,
              result.match_at(0).type);
    EXPECT_EQ(AutocompleteProvider::TYPE_KEYWORD,
              result.match_at(0).provider->type());
    EXPECT_EQ(ASCIIToUTF16("kw"), result.match_at(1).keyword);
    EXPECT_EQ(ASCIIToUTF16("kw suggestion1"),
              result.match_at(1).fill_into_edit);
    EXPECT_EQ(AutocompleteProvider::TYPE_KEYWORD,
              result.match_at(1).provider->type());
    EXPECT_EQ(ASCIIToUTF16("kw"), result.match_at(2).keyword);
    EXPECT_EQ(ASCIIToUTF16("kw suggestion2"),
              result.match_at(2).fill_into_edit);
    EXPECT_EQ(AutocompleteProvider::TYPE_KEYWORD,
              result.match_at(2).provider->type());
    EXPECT_EQ(ASCIIToUTF16("kw"), result.match_at(3).keyword);
    EXPECT_EQ(ASCIIToUTF16("kw suggestion3"),
              result.match_at(3).fill_into_edit);
    EXPECT_EQ(AutocompleteProvider::TYPE_KEYWORD,
              result.match_at(3).provider->type());

    base::string16 description =
        ASCIIToUTF16("Description with style: <match>, [dim], (url till end)");
    EXPECT_EQ(description, result.match_at(1).contents);
    ASSERT_EQ(6u, result.match_at(1).contents_class.size());

    EXPECT_EQ(0u,
              result.match_at(1).contents_class[0].offset);
    EXPECT_EQ(ACMatchClassification::NONE,
              result.match_at(1).contents_class[0].style);

    EXPECT_EQ(description.find('<'),
              result.match_at(1).contents_class[1].offset);
    EXPECT_EQ(ACMatchClassification::MATCH,
              result.match_at(1).contents_class[1].style);

    EXPECT_EQ(description.find('>') + 1u,
              result.match_at(1).contents_class[2].offset);
    EXPECT_EQ(ACMatchClassification::NONE,
              result.match_at(1).contents_class[2].style);

    EXPECT_EQ(description.find('['),
              result.match_at(1).contents_class[3].offset);
    EXPECT_EQ(ACMatchClassification::DIM,
              result.match_at(1).contents_class[3].style);

    EXPECT_EQ(description.find(']') + 1u,
              result.match_at(1).contents_class[4].offset);
    EXPECT_EQ(ACMatchClassification::NONE,
              result.match_at(1).contents_class[4].style);

    EXPECT_EQ(description.find('('),
              result.match_at(1).contents_class[5].offset);
    EXPECT_EQ(ACMatchClassification::URL,
              result.match_at(1).contents_class[5].style);

    AutocompleteMatch match = result.match_at(4);
    EXPECT_EQ(AutocompleteMatchType::SEARCH_WHAT_YOU_TYPED, match.type);
    EXPECT_EQ(AutocompleteProvider::TYPE_SEARCH,
              result.match_at(4).provider->type());
    EXPECT_FALSE(match.deletable);
  }

  // Flaky, see http://crbug.com/167158
  /*
  {
    LocationBar* location_bar = GetLocationBar(browser());
    ResultCatcher catcher;
    OmniboxView* omnibox_view = location_bar->GetOmniboxView();
    omnibox_view->OnBeforePossibleChange();
    omnibox_view->SetUserText(ASCIIToUTF16("kw command"));
    omnibox_view->OnAfterPossibleChange(true);
    location_bar->AcceptInput();
    // This checks that the keyword provider (via javascript)
    // gets told to navigate to the string "command".
    EXPECT_TRUE(catcher.GetNextResult()) << catcher.message();
  }
  */
}

IN_PROC_BROWSER_TEST_F(OmniboxApiTest, OnInputEntered) {
  ASSERT_TRUE(RunExtensionTest("omnibox")) << message_;
  Profile* profile = browser()->profile();
  search_test_utils::WaitForTemplateURLServiceToLoad(
      TemplateURLServiceFactory::GetForProfile(profile));

  LocationBar* location_bar = GetLocationBar(browser());
  OmniboxView* omnibox_view = location_bar->GetOmniboxView();
  ResultCatcher catcher;
  AutocompleteController* autocomplete_controller =
      GetAutocompleteController(browser());
  omnibox_view->OnBeforePossibleChange();
  omnibox_view->SetUserText(ASCIIToUTF16("kw command"));
  omnibox_view->OnAfterPossibleChange(true);

  {
    AutocompleteInput input(ASCIIToUTF16("kw command"),
                            metrics::OmniboxEventProto::NTP,
                            ChromeAutocompleteSchemeClassifier(profile));
    autocomplete_controller->Start(input);
  }
  omnibox_view->model()->AcceptInput(WindowOpenDisposition::CURRENT_TAB, false);
  WaitForAutocompleteDone(autocomplete_controller);
  EXPECT_TRUE(autocomplete_controller->done());
  EXPECT_TRUE(catcher.GetNextResult()) << catcher.message();

  omnibox_view->OnBeforePossibleChange();
  omnibox_view->SetUserText(ASCIIToUTF16("kw newtab"));
  omnibox_view->OnAfterPossibleChange(true);
  WaitForAutocompleteDone(autocomplete_controller);
  EXPECT_TRUE(autocomplete_controller->done());

  {
    AutocompleteInput input(ASCIIToUTF16("kw newtab"),
                            metrics::OmniboxEventProto::NTP,
                            ChromeAutocompleteSchemeClassifier(profile));
    autocomplete_controller->Start(input);
  }
  omnibox_view->model()->AcceptInput(WindowOpenDisposition::NEW_FOREGROUND_TAB,
                                     false);
  WaitForAutocompleteDone(autocomplete_controller);
  EXPECT_TRUE(autocomplete_controller->done());
  EXPECT_TRUE(catcher.GetNextResult()) << catcher.message();
}

// Tests that we get suggestions from and send input to the incognito context
// of an incognito split mode extension.
// http://crbug.com/100927
// Test is flaky: http://crbug.com/101219
IN_PROC_BROWSER_TEST_F(OmniboxApiTest, DISABLED_IncognitoSplitMode) {
  Profile* profile = browser()->profile();
  ResultCatcher catcher_incognito;
  catcher_incognito.RestrictToBrowserContext(profile->GetOffTheRecordProfile());

  ASSERT_TRUE(RunExtensionTestIncognito("omnibox")) << message_;

  // Open an incognito window and wait for the incognito extension process to
  // respond.
  Browser* incognito_browser = CreateIncognitoBrowser();
  ASSERT_TRUE(catcher_incognito.GetNextResult()) << catcher_incognito.message();

  // The results depend on the TemplateURLService being loaded. Make sure it is
  // loaded so that the autocomplete results are consistent.
  search_test_utils::WaitForTemplateURLServiceToLoad(
      TemplateURLServiceFactory::GetForProfile(browser()->profile()));

  LocationBar* location_bar = GetLocationBar(incognito_browser);
  AutocompleteController* autocomplete_controller =
      GetAutocompleteController(incognito_browser);

  // Test that we get the incognito-specific suggestions.
  {
    AutocompleteInput input(ASCIIToUTF16("kw suggestio"),
                            metrics::OmniboxEventProto::NTP,
                            ChromeAutocompleteSchemeClassifier(profile));
    autocomplete_controller->Start(input);
    WaitForAutocompleteDone(autocomplete_controller);
    EXPECT_TRUE(autocomplete_controller->done());

    // First result should be to invoke the keyword with what we typed, 2-4
    // should be to invoke with suggestions from the extension, and the last
    // should be to search for what we typed.
    const AutocompleteResult& result = autocomplete_controller->result();
    ASSERT_EQ(5U, result.size()) << AutocompleteResultAsString(result);
    ASSERT_FALSE(result.match_at(0).keyword.empty());
    EXPECT_EQ(ASCIIToUTF16("kw suggestion3 incognito"),
              result.match_at(3).fill_into_edit);
  }

  // Test that our input is sent to the incognito context. The test will do a
  // text comparison and succeed only if "command incognito" is sent to the
  // incognito context.
  {
    ResultCatcher catcher;
    AutocompleteInput input(ASCIIToUTF16("kw command incognito"),
                            metrics::OmniboxEventProto::NTP,
                            ChromeAutocompleteSchemeClassifier(profile));
    autocomplete_controller->Start(input);
    location_bar->AcceptInput();
    EXPECT_TRUE(catcher.GetNextResult()) << catcher.message();
  }
}
