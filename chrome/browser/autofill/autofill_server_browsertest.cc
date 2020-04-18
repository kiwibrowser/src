// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/base64url.h"
#include "base/base_switches.h"
#include "base/command_line.h"
#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "base/strings/utf_string_conversions.h"
#include "chrome/browser/autofill/personal_data_manager_factory.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/ui_test_utils.h"
#include "components/autofill/core/browser/autofill_profile.h"
#include "components/autofill/core/browser/autofill_test_utils.h"
#include "components/autofill/core/browser/personal_data_manager.h"
#include "components/autofill/core/browser/personal_data_manager_observer.h"
#include "content/public/test/browser_test_utils.h"
#include "content/public/test/test_utils.h"
#include "net/url_request/test_url_fetcher_factory.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/third_party/mozilla/url_parse.h"

namespace autofill {
namespace {

// TODO(bondd): PdmChangeWaiter in autofill_uitest_util.cc is a replacement for
// this class. Remove this class and use helper functions in that file instead.
class WindowedPersonalDataManagerObserver : public PersonalDataManagerObserver {
 public:
  explicit WindowedPersonalDataManagerObserver(Profile* profile)
      : profile_(profile),
        message_loop_runner_(new content::MessageLoopRunner){
    PersonalDataManagerFactory::GetForProfile(profile_)->AddObserver(this);
  }
  ~WindowedPersonalDataManagerObserver() override {}

  // Waits for the PersonalDataManager's list of profiles to be updated.
  void Wait() {
    message_loop_runner_->Run();
    PersonalDataManagerFactory::GetForProfile(profile_)->RemoveObserver(this);
  }

  // PersonalDataManagerObserver:
  void OnPersonalDataChanged() override { message_loop_runner_->Quit(); }

 private:
  Profile* profile_;
  scoped_refptr<content::MessageLoopRunner> message_loop_runner_;
};

class WindowedNetworkObserver : public net::TestURLFetcher::DelegateForTests {
 public:
  explicit WindowedNetworkObserver(const std::string& expected_upload_data)
      : factory_(new net::TestURLFetcherFactory),
        expected_upload_data_(expected_upload_data),
        message_loop_runner_(new content::MessageLoopRunner) {
    factory_->SetDelegateForTests(this);
  }
  ~WindowedNetworkObserver() {}

  // Waits for a network request with the |expected_upload_data_|.
  void Wait() {
    message_loop_runner_->Run();
    factory_.reset();
  }

  // Helper to extract the value of a query param. Returns "*** not found ***"
  // if the requested query param is not in the query string.
  std::string GetQueryParam(const net::TestURLFetcher& fetcher,
                            const std::string& param_name) {
    std::string query_str = fetcher.GetOriginalURL().query();
    url::Component query(0, query_str.length());
    url::Component key, value;
    while (url::ExtractQueryKeyValue(query_str.c_str(), &query, &key, &value)) {
      std::string key_string(query_str.substr(key.begin, key.len));
      std::string param_text(query_str.substr(value.begin, value.len));
      std::string param_value;
      if (key_string == param_name &&
          base::Base64UrlDecode(param_text,
                                base::Base64UrlDecodePolicy::REQUIRE_PADDING,
                                &param_value)) {
        return param_value;
      }
    }
    return "*** not found ***";
  }

  // net::TestURLFetcher::DelegateForTests:
  void OnRequestStart(int fetcher_id) override {
    net::TestURLFetcher* fetcher = factory_->GetFetcherByID(fetcher_id);
    if (fetcher->upload_data() == expected_upload_data_ ||
        GetQueryParam(*fetcher, "q") == expected_upload_data_) {
      message_loop_runner_->Quit();
    }
    // Not interested in any further status updates from this fetcher.
    fetcher->SetDelegateForTests(nullptr);
  }
  void OnChunkUpload(int fetcher_id) override {}
  void OnRequestEnd(int fetcher_id) override {}

 private:
  // Mocks out network requests.
  std::unique_ptr<net::TestURLFetcherFactory> factory_;

  const std::string expected_upload_data_;
  scoped_refptr<content::MessageLoopRunner> message_loop_runner_;

  DISALLOW_COPY_AND_ASSIGN(WindowedNetworkObserver);
};

}  // namespace

class AutofillServerTest : public InProcessBrowserTest  {
 public:
  void SetUpCommandLine(base::CommandLine* command_line) override {
    // Enable finch experiment for sending field metadata.
    command_line->AppendSwitchASCII(
        ::switches::kForceFieldTrials, "AutofillFieldMetadata/Enabled/");
  }
};

// Regression test for http://crbug.com/177419
IN_PROC_BROWSER_TEST_F(AutofillServerTest,
                       QueryAndUploadBothIncludeFieldsWithAutocompleteOff) {
  // Seed some test Autofill profile data, as upload requests are only made when
  // there is local data available to use as a baseline.
  WindowedPersonalDataManagerObserver personal_data_observer(
      browser()->profile());
  PersonalDataManagerFactory::GetForProfile(browser()->profile())
      ->AddProfile(test::GetFullProfile());
  personal_data_observer.Wait();

  // Load the test page. Expect a query request upon loading the page.
  const char kDataURIPrefix[] = "data:text/html;charset=utf-8,";
  const char kFormHtml[] =
      "<form id='test_form' action='about:blank'>"
      "  <input id='one'>"
      "  <input id='two' autocomplete='off'>"
      "  <input id='three'>"
      "  <input id='four' autocomplete='off'>"
      "  <input type='submit'>"
      "</form>"
      "<script>"
      "  document.onclick = function() {"
      "    document.getElementById('test_form').submit();"
      "  };"
      "</script>";

  AutofillQueryContents query;
  query.set_client_version("6.1.1715.1442/en (GGLL)");
  AutofillQueryContents::Form* query_form = query.add_form();
  query_form->set_signature(15916856893790176210U);

  test::FillQueryField(query_form->add_field(), 2594484045U, "one", "text");
  test::FillQueryField(query_form->add_field(), 2750915947U, "two", "text");
  test::FillQueryField(query_form->add_field(), 3494787134U, "three", "text");
  test::FillQueryField(query_form->add_field(), 1236501728U, "four", "text");

  std::string expected_query_string;
  ASSERT_TRUE(query.SerializeToString(&expected_query_string));

  WindowedNetworkObserver query_network_observer(expected_query_string);

  ui_test_utils::NavigateToURL(
      browser(), GURL(std::string(kDataURIPrefix) + kFormHtml));
  query_network_observer.Wait();

  // Submit the form, using a simulated mouse click because form submissions not
  // triggered by user gestures are ignored. Expect an upload request upon form
  // submission, with form fields matching those from the query request.
  AutofillUploadContents upload;
  upload.set_submission(true);
  upload.set_client_version("6.1.1715.1442/en (GGLL)");
  upload.set_form_signature(15916856893790176210U);
  upload.set_autofill_used(false);
  upload.set_data_present("1f7e0003780000080004");
  upload.set_action_signature(15724779818122431245U);
  upload.set_form_name("test_form");
  upload.set_passwords_revealed(false);

  test::FillUploadField(upload.add_field(), 2594484045U, "one", "text", nullptr,
                        2U);
  test::FillUploadField(upload.add_field(), 2750915947U, "two", "text", "off",
                        2U);
  test::FillUploadField(upload.add_field(), 3494787134U, "three", "text",
                        nullptr, 2U);
  test::FillUploadField(upload.add_field(), 1236501728U, "four", "text", "off",
                        2U);

  std::string expected_upload_string;
  ASSERT_TRUE(upload.SerializeToString(&expected_upload_string));

  WindowedNetworkObserver upload_network_observer(expected_upload_string);
  content::WebContents* web_contents =
      browser()->tab_strip_model()->GetActiveWebContents();
  content::SimulateMouseClick(web_contents, 0,
                              blink::WebMouseEvent::Button::kLeft);
  upload_network_observer.Wait();
}

// Verify that a site with password fields will query even in the presence
// of user defined autocomplete types.
IN_PROC_BROWSER_TEST_F(AutofillServerTest,
                       AlwaysQueryForPasswordFields) {
  // Load the test page. Expect a query request upon loading the page.
  const char kDataURIPrefix[] = "data:text/html;charset=utf-8,";
  const char kFormHtml[] =
      "<form id='test_form'>"
      "  <input type='text' id='one' autocomplete='username'>"
      "  <input type='text' id='two' autocomplete='off'>"
      "  <input type='password' id='three'>"
      "  <input type='submit'>"
      "</form>";

  AutofillQueryContents query;
  query.set_client_version("6.1.1715.1442/en (GGLL)");
  AutofillQueryContents::Form* query_form = query.add_form();
  query_form->set_signature(8900697631820480876U);

  test::FillQueryField(query_form->add_field(), 2594484045U, "one", "text");
  test::FillQueryField(query_form->add_field(), 2750915947U, "two", "text");
  test::FillQueryField(query_form->add_field(), 116843943U, "three",
                       "password");

  std::string expected_query_string;
  ASSERT_TRUE(query.SerializeToString(&expected_query_string));

  WindowedNetworkObserver query_network_observer(expected_query_string);
  ui_test_utils::NavigateToURL(
      browser(), GURL(std::string(kDataURIPrefix) + kFormHtml));
  query_network_observer.Wait();
}

}  // namespace autofill
