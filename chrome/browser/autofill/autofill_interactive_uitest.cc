// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <string>
#include <tuple>
#include <utility>

#include "base/base_switches.h"
#include "base/command_line.h"
#include "base/files/file_util.h"
#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "base/metrics/field_trial.h"
#include "base/rand_util.h"
#include "base/run_loop.h"
#include "base/strings/string16.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_split.h"
#include "base/strings/stringprintf.h"
#include "base/strings/utf_string_conversions.h"
#include "base/test/histogram_tester.h"
#include "base/test/mock_entropy_provider.h"
#include "base/test/scoped_feature_list.h"
#include "base/time/time.h"
#include "build/build_config.h"
#include "chrome/browser/autofill/autofill_uitest_util.h"
#include "chrome/browser/chrome_notification_types.h"
#include "chrome/browser/metrics/subprocess_metrics_provider.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/translate/chrome_translate_client.h"
#include "chrome/browser/translate/translate_service.h"
#include "chrome/browser/ui/autofill/chrome_autofill_client.h"
#include "chrome/browser/ui/browser_window.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/browser/ui/translate/translate_bubble_test_utils.h"
#include "chrome/common/chrome_features.h"
#include "chrome/common/chrome_switches.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/interactive_test_utils.h"
#include "chrome/test/base/test_switches.h"
#include "chrome/test/base/ui_test_utils.h"
#include "components/autofill/content/browser/content_autofill_driver.h"
#include "components/autofill/content/browser/content_autofill_driver_factory.h"
#include "components/autofill/core/browser/autofill_experiments.h"
#include "components/autofill/core/browser/autofill_manager.h"
#include "components/autofill/core/browser/autofill_manager_test_delegate.h"
#include "components/autofill/core/browser/autofill_profile.h"
#include "components/autofill/core/browser/autofill_test_utils.h"
#include "components/autofill/core/browser/validation.h"
#include "components/autofill/core/common/autofill_features.h"
#include "components/network_session_configurator/common/network_switches.h"
#include "components/translate/core/browser/translate_manager.h"
#include "content/public/browser/navigation_controller.h"
#include "content/public/browser/notification_observer.h"
#include "content/public/browser/notification_registrar.h"
#include "content/public/browser/notification_service.h"
#include "content/public/browser/render_view_host.h"
#include "content/public/browser/render_widget_host.h"
#include "content/public/browser/render_widget_host_view.h"
#include "content/public/browser/web_contents.h"
#include "content/public/test/browser_test_utils.h"
#include "content/public/test/test_renderer_host.h"
#include "content/public/test/test_utils.h"
#include "net/base/net_errors.h"
#include "net/dns/mock_host_resolver.h"
#include "net/test/embedded_test_server/embedded_test_server.h"
#include "net/url_request/test_url_fetcher_factory.h"
#include "net/url_request/url_request_status.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/events/base_event_utils.h"
#include "ui/events/keycodes/keyboard_code_conversion.h"
#include "ui/events/keycodes/keyboard_codes.h"

using base::ASCIIToUTF16;

namespace autofill {

namespace {

static const char kDataURIPrefix[] = "data:text/html;charset=utf-8,";
static const char kTestFormString[] =
    "<form action=\"http://www.example.com/\" method=\"POST\">"
    "<label for=\"firstname\">First name:</label>"
    " <input type=\"text\" id=\"firstname\"><br>"
    "<label for=\"lastname\">Last name:</label>"
    " <input type=\"text\" id=\"lastname\"><br>"
    "<label for=\"address1\">Address line 1:</label>"
    " <input type=\"text\" id=\"address1\"><br>"
    "<label for=\"address2\">Address line 2:</label>"
    " <input type=\"text\" id=\"address2\"><br>"
    "<label for=\"city\">City:</label>"
    " <input type=\"text\" id=\"city\"><br>"
    "<label for=\"state\">State:</label>"
    " <select id=\"state\">"
    " <option value=\"\" selected=\"yes\">--</option>"
    " <option value=\"CA\">California</option>"
    " <option value=\"TX\">Texas</option>"
    " </select><br>"
    "<label for=\"zip\">ZIP code:</label>"
    " <input type=\"text\" id=\"zip\"><br>"
    "<label for=\"country\">Country:</label>"
    " <select id=\"country\">"
    " <option value=\"\" selected=\"yes\">--</option>"
    " <option value=\"CA\">Canada</option>"
    " <option value=\"US\">United States</option>"
    " </select><br>"
    "<label for=\"phone\">Phone number:</label>"
    " <input type=\"text\" id=\"phone\"><br>"
    "</form>";

// TODO(crbug.com/609861): Remove the autocomplete attribute from the textarea
// field when the bug is fixed.
static const char kTestEventFormString[] =
    "<script type=\"text/javascript\">"
    "var inputfocus = false;"
    "var inputkeydown = false;"
    "var inputinput = false;"
    "var inputchange = false;"
    "var inputkeyup = false;"
    "var inputblur = false;"
    "var textfocus = false;"
    "var textkeydown = false;"
    "var textinput= false;"
    "var textchange = false;"
    "var textkeyup = false;"
    "var textblur = false;"
    "var selectfocus = false;"
    "var selectinput = false;"
    "var selectchange = false;"
    "var selectblur = false;"
    "</script>"
    "<form action=\"http://www.example.com/\" method=\"POST\">"
    "<label for=\"firstname\">First name:</label>"
    " <input type=\"text\" id=\"firstname\"><br>"
    "<label for=\"lastname\">Last name:</label>"
    " <input type=\"text\" id=\"lastname\""
    " onfocus=\"inputfocus = true\" onkeydown=\"inputkeydown = true\""
    " oninput=\"inputinput = true\" onchange=\"inputchange = true\""
    " onkeyup=\"inputkeyup = true\" onblur=\"inputblur = true\" ><br>"
    "<label for=\"address1\">Address line 1:</label>"
    " <input type=\"text\" id=\"address1\"><br>"
    "<label for=\"address2\">Address line 2:</label>"
    " <input type=\"text\" id=\"address2\"><br>"
    "<label for=\"city\">City:</label>"
    " <textarea rows=\"4\" cols=\"50\" id=\"city\" name=\"city\""
    " autocomplete=\"address-level2\" onfocus=\"textfocus = true\""
    " onkeydown=\"textkeydown = true\" oninput=\"textinput = true\""
    " onchange=\"textchange = true\" onkeyup=\"textkeyup = true\""
    " onblur=\"textblur = true\"></textarea><br>"
    "<label for=\"state\">State:</label>"
    " <select id=\"state\""
    " onfocus=\"selectfocus = true\" oninput=\"selectinput = true\""
    " onchange=\"selectchange = true\" onblur=\"selectblur = true\" >"
    " <option value=\"\" selected=\"yes\">--</option>"
    " <option value=\"CA\">California</option>"
    " <option value=\"TX\">Texas</option>"
    " </select><br>"
    "<label for=\"zip\">ZIP code:</label>"
    " <input type=\"text\" id=\"zip\"><br>"
    "<label for=\"country\">Country:</label>"
    " <select id=\"country\">"
    " <option value=\"\" selected=\"yes\">--</option>"
    " <option value=\"CA\">Canada</option>"
    " <option value=\"US\">United States</option>"
    " </select><br>"
    "<label for=\"phone\">Phone number:</label>"
    " <input type=\"text\" id=\"phone\"><br>"
    "</form>";

// AutofillManagerTestDelegateImpl --------------------------------------------

class AutofillManagerTestDelegateImpl
    : public autofill::AutofillManagerTestDelegate {
 public:
  AutofillManagerTestDelegateImpl() {}
  ~AutofillManagerTestDelegateImpl() override {}

  // autofill::AutofillManagerTestDelegate:
  void DidPreviewFormData() override {
    ASSERT_TRUE(loop_runner_);
    loop_runner_->Quit();
  }

  void DidFillFormData() override {
    ASSERT_TRUE(loop_runner_);
    if (!is_expecting_dynamic_refill_)
      ASSERT_TRUE(loop_runner_->running());
    loop_runner_->Quit();
  }

  void DidShowSuggestions() override {
    ASSERT_TRUE(loop_runner_);
    loop_runner_->Quit();
  }

  void OnTextFieldChanged() override {
    if (!waiting_for_text_change_)
      return;
    waiting_for_text_change_ = false;
    ASSERT_TRUE(loop_runner_);
    loop_runner_->Quit();
  }

  void Reset() { loop_runner_ = std::make_unique<base::RunLoop>(); }

  void Wait() {
    loop_runner_->Run();
  }

  void WaitForTextChange() {
    waiting_for_text_change_ = true;
    loop_runner_->Run();
  }

  void SetIsExpectingDynamicRefill(bool expect_refill) {
    is_expecting_dynamic_refill_ = expect_refill;
  }

 private:
  std::unique_ptr<base::RunLoop> loop_runner_;
  bool waiting_for_text_change_ = false;
  bool is_expecting_dynamic_refill_ = false;

  DISALLOW_COPY_AND_ASSIGN(AutofillManagerTestDelegateImpl);
};

// Searches all frames of |web_contents| and returns one called |name|. If
// there are none, returns null, if there are more, returns an arbitrary one.
content::RenderFrameHost* RenderFrameHostForName(
    content::WebContents* web_contents,
    const std::string& name) {
  return content::FrameMatchingPredicate(
      web_contents, base::BindRepeating(&content::FrameMatchesName, name));
}

}  // namespace

// AutofillInteractiveTestBase ------------------------------------------------

// Test fixtures derive from this class and indicate via constructor parameter
// if feature kAutofillExpandedPopupViews is enabled. This class hierarchy
// allows test fixtures to have distinct list of test parameters.
//
// TODO(crbug.com/832707): Parametrize this class to ensure that all tests in
//                         this run with all possible valid combinations of
//                         features and field trials.
class AutofillInteractiveTestBase : public InProcessBrowserTest {
 protected:
  explicit AutofillInteractiveTestBase(bool popup_views_enabled)
      : key_press_event_sink_(base::BindRepeating(
            &AutofillInteractiveTestBase::HandleKeyPressEvent,
            base::Unretained(this))),
        popup_views_enabled_(popup_views_enabled) {
    scoped_feature_list_.InitWithFeatureState(kAutofillExpandedPopupViews,
                                              popup_views_enabled_);
  }

  ~AutofillInteractiveTestBase() override {}

  // InProcessBrowserTest:
  void SetUpOnMainThread() override {
    InProcessBrowserTest::SetUpOnMainThread();
    // Don't want Keychain coming up on Mac.
    test::DisableSystemServices(browser()->profile()->GetPrefs());

    // Inject the test delegate into the AutofillManager.
    GetAutofillManager()->SetTestDelegate(&test_delegate_);

    // If the mouse happened to be over where the suggestions are shown, then
    // the preview will show up and will fail the tests. We need to give it a
    // point that's within the browser frame, or else the method hangs.
    gfx::Point reset_mouse(GetWebContents()->GetContainerBounds().origin());
    reset_mouse = gfx::Point(reset_mouse.x() + 5, reset_mouse.y() + 5);
    ASSERT_TRUE(ui_test_utils::SendMouseMoveSync(reset_mouse));

    // Ensure that |embedded_test_server()| serves both domains used below.
    host_resolver()->AddRule("*", "127.0.0.1");
    ASSERT_TRUE(embedded_test_server()->Start());
  }

  void TearDownOnMainThread() override {
    // Make sure to close any showing popups prior to tearing down the UI.
    GetAutofillManager()->client()->HideAutofillPopup();
    test::ReenableSystemServices();
  }

  content::WebContents* GetWebContents() {
    return browser()->tab_strip_model()->GetActiveWebContents();
  }

  content::RenderViewHost* GetRenderViewHost() {
    return GetWebContents()->GetRenderViewHost();
  }

  AutofillManager* GetAutofillManager() {
    content::WebContents* web_contents = GetWebContents();
    return ContentAutofillDriverFactory::FromWebContents(web_contents)
        ->DriverForFrame(web_contents->GetMainFrame())
        ->autofill_manager();
  }

  void CreateTestProfile() {
    AutofillProfile profile;
    test::SetProfileInfo(&profile, "Milton", "C.", "Waddams",
                         "red.swingline@initech.com", "Initech",
                         "4120 Freidrich Lane", "Basement", "Austin", "Texas",
                         "78744", "US", "15125551234");

    AddTestProfile(browser(), profile);
  }

  // Populates a webpage form using autofill data and keypress events.
  // This function focuses the specified input field in the form, and then
  // sends keypress events to the tab to cause the form to be populated.
  void PopulateForm(const std::string& field_id) {
    std::string js("document.getElementById('" + field_id + "').focus();");
    ASSERT_TRUE(content::ExecuteScript(GetRenderViewHost(), js));

    SendKeyToPageAndWait(ui::DomKey::ARROW_DOWN);
    SendKeyToPopupAndWait(ui::DomKey::ARROW_DOWN);
    SendKeyToPopupAndWait(ui::DomKey::ENTER);
  }

  void ExpectFieldValue(const std::string& field_name,
                        const std::string& expected_value) {
    std::string value;
    ASSERT_TRUE(content::ExecuteScriptAndExtractString(
        GetWebContents(),
        "window.domAutomationController.send("
        "    document.getElementById('" + field_name + "').value);",
        &value));
    EXPECT_EQ(expected_value, value) << "for field " << field_name;
  }

  void GetFieldBackgroundColor(const std::string& field_name,
                               std::string* color) {
    ASSERT_TRUE(content::ExecuteScriptAndExtractString(
        GetWebContents(),
        "window.domAutomationController.send("
        "    document.defaultView.getComputedStyle(document.getElementById('" +
        field_name + "')).backgroundColor);",
        color));
  }

  void SimulateURLFetch(bool success) {
    net::TestURLFetcher* fetcher = url_fetcher_factory_.GetFetcherByID(0);
    ASSERT_TRUE(fetcher);
    net::Error error = success ? net::OK : net::ERR_FAILED;

    std::string script = " var google = {};"
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
        "          return \"ja\";"
        "        },"
        "        translatePage : function(originalLang, targetLang,"
        "                                 onTranslateProgress) {"
        "          document.getElementsByTagName(\"body\")[0].innerHTML = '" +
        std::string(kTestFormString) +
        "              ';"
        "          onTranslateProgress(100, true, false);"
        "        }"
        "      };"
        "    }"
        "  };"
        "})();"
        "cr.googleTranslate.onTranslateElementLoad();";

    fetcher->set_url(fetcher->GetOriginalURL());
    fetcher->set_status(net::URLRequestStatus::FromError(error));
    fetcher->set_response_code(success ? 200 : 500);
    fetcher->SetResponseString(script);
    fetcher->delegate()->OnURLFetchComplete(fetcher);
  }

  void FocusFieldByName(const std::string& name) {
    bool result = false;
    std::string script = base::StringPrintf(
        R"( function onFocusHandler(e) {
              e.target.removeEventListener(e.type, arguments.callee);
              domAutomationController.send(true);
            }
            if (document.readyState === 'complete') {
              var target = document.getElementById('%s');
              target.addEventListener('focus', onFocusHandler);
              target.focus();
            } else {
              domAutomationController.send(false);
            })",
        name.c_str());
    ASSERT_TRUE(content::ExecuteScriptAndExtractBool(GetRenderViewHost(),
                                                     script, &result));
    ASSERT_TRUE(result);
  }

  void FocusFirstNameField() { FocusFieldByName("firstname"); }

  // Simulates a click on the middle of the DOM element with the given |id|.
  void ClickElementWithId(const std::string& id) {
    int x;
    ASSERT_TRUE(content::ExecuteScriptAndExtractInt(
        GetRenderViewHost(),
        "var bounds = document.getElementById('" +
            id +
            "').getBoundingClientRect();"
            "domAutomationController.send("
            "    Math.floor(bounds.left + bounds.width / 2));",
        &x));
    int y;
    ASSERT_TRUE(content::ExecuteScriptAndExtractInt(
        GetRenderViewHost(),
        "var bounds = document.getElementById('" +
            id +
            "').getBoundingClientRect();"
            "domAutomationController.send("
            "    Math.floor(bounds.top + bounds.height / 2));",
        &y));
    content::SimulateMouseClickAt(GetWebContents(), 0,
                                  blink::WebMouseEvent::Button::kLeft,
                                  gfx::Point(x, y));
  }

  void ClickFirstNameField() {
    ASSERT_NO_FATAL_FAILURE(ClickElementWithId("firstname"));
  }

  // Make a pointless round trip to the renderer, giving the popup a chance to
  // show if it's going to. If it does show, an assert in
  // AutofillManagerTestDelegateImpl will trigger.
  void MakeSurePopupDoesntAppear() {
    int unused;
    ASSERT_TRUE(content::ExecuteScriptAndExtractInt(
        GetRenderViewHost(), "domAutomationController.send(42)", &unused));
  }

  void ExpectFilledTestForm() {
    ExpectFieldValue("firstname", "Milton");
    ExpectFieldValue("lastname", "Waddams");
    ExpectFieldValue("address1", "4120 Freidrich Lane");
    ExpectFieldValue("address2", "Basement");
    ExpectFieldValue("city", "Austin");
    ExpectFieldValue("state", "TX");
    ExpectFieldValue("zip", "78744");
    ExpectFieldValue("country", "US");
    ExpectFieldValue("phone", "15125551234");
  }

  void SendKeyToPageAndWait(ui::DomKey key) {
    ui::KeyboardCode key_code = ui::NonPrintableDomKeyToKeyboardCode(key);
    ui::DomCode code = ui::UsLayoutKeyboardCodeToDomCode(key_code);
    SendKeyToPageAndWait(key, code, key_code);
  }

  void SendKeyToPageAndWait(ui::DomKey key,
                            ui::DomCode code,
                            ui::KeyboardCode key_code) {
    test_delegate_.Reset();
    content::SimulateKeyPress(GetWebContents(), key, code, key_code, false,
                              false, false, false);
    test_delegate_.Wait();
  }

  bool HandleKeyPressEvent(const content::NativeWebKeyboardEvent& event) {
    return true;
  }

  void SendKeyToPopupAndWait(ui::DomKey key) {
    ui::KeyboardCode key_code = ui::NonPrintableDomKeyToKeyboardCode(key);
    ui::DomCode code = ui::UsLayoutKeyboardCodeToDomCode(key_code);
    SendKeyToPopupAndWait(key, code, key_code,
                          GetRenderViewHost()->GetWidget());
  }

  void SendKeyToPopupAndWait(ui::DomKey key,
                             ui::DomCode code,
                             ui::KeyboardCode key_code,
                             content::RenderWidgetHost* widget) {
    // Route popup-targeted key presses via the render view host.
    content::NativeWebKeyboardEvent event(blink::WebKeyboardEvent::kRawKeyDown,
                                          blink::WebInputEvent::kNoModifiers,
                                          ui::EventTimeForNow());
    event.windows_key_code = key_code;
    event.dom_code = static_cast<int>(code);
    event.dom_key = key;
    test_delegate_.Reset();
    // Install the key press event sink to ensure that any events that are not
    // handled by the installed callbacks do not end up crashing the test.
    widget->AddKeyPressEventCallback(key_press_event_sink_);
    widget->ForwardKeyboardEvent(event);
    test_delegate_.Wait();
    widget->RemoveKeyPressEventCallback(key_press_event_sink_);
  }

  void SendKeyToDataListPopup(ui::DomKey key) {
    ui::KeyboardCode key_code = ui::NonPrintableDomKeyToKeyboardCode(key);
    ui::DomCode code = ui::UsLayoutKeyboardCodeToDomCode(key_code);
    SendKeyToDataListPopup(key, code, key_code);
  }

  // Datalist does not support autofill preview. There is no need to start
  // message loop for Datalist.
  void SendKeyToDataListPopup(ui::DomKey key,
                              ui::DomCode code,
                              ui::KeyboardCode key_code) {
    // Route popup-targeted key presses via the render view host.
    content::NativeWebKeyboardEvent event(blink::WebKeyboardEvent::kRawKeyDown,
                                          blink::WebInputEvent::kNoModifiers,
                                          ui::EventTimeForNow());
    event.windows_key_code = key_code;
    event.dom_code = static_cast<int>(code);
    event.dom_key = key;
    // Install the key press event sink to ensure that any events that are not
    // handled by the installed callbacks do not end up crashing the test.
    GetRenderViewHost()->GetWidget()->AddKeyPressEventCallback(
        key_press_event_sink_);
    GetRenderViewHost()->GetWidget()->ForwardKeyboardEvent(event);
    GetRenderViewHost()->GetWidget()->RemoveKeyPressEventCallback(
        key_press_event_sink_);
  }

  void TryBasicFormFill() {
    FocusFirstNameField();

    // Start filling the first name field with "M" and wait for the popup to be
    // shown.
    SendKeyToPageAndWait(ui::DomKey::FromCharacter('M'), ui::DomCode::US_M,
                         ui::VKEY_M);

    // Press the down arrow to select the suggestion and preview the autofilled
    // form.
    SendKeyToPopupAndWait(ui::DomKey::ARROW_DOWN);

    // The previewed values should not be accessible to JavaScript.
    ExpectFieldValue("firstname", "M");
    ExpectFieldValue("lastname", std::string());
    ExpectFieldValue("address1", std::string());
    ExpectFieldValue("address2", std::string());
    ExpectFieldValue("city", std::string());
    ExpectFieldValue("state", std::string());
    ExpectFieldValue("zip", std::string());
    ExpectFieldValue("country", std::string());
    ExpectFieldValue("phone", std::string());
    // TODO(isherman): It would be nice to test that the previewed values are
    // displayed: http://crbug.com/57220

    // Press Enter to accept the autofill suggestions.
    SendKeyToPopupAndWait(ui::DomKey::ENTER);

    // The form should be filled.
    ExpectFilledTestForm();
  }

  void TriggerFormFill(const std::string& field_name) {
    FocusFieldByName(field_name);

    // Start filling the first name field with "M" and wait for the popup to be
    // shown.
    SendKeyToPageAndWait(ui::DomKey::FromCharacter('M'), ui::DomCode::US_M,
                         ui::VKEY_M);

    // Press the down arrow to select the suggestion and preview the autofilled
    // form.
    SendKeyToPopupAndWait(ui::DomKey::ARROW_DOWN);

    // Press Enter to accept the autofill suggestions.
    SendKeyToPopupAndWait(ui::DomKey::ENTER);
  }

  AutofillManagerTestDelegateImpl* test_delegate() { return &test_delegate_; }

 private:
  AutofillManagerTestDelegateImpl test_delegate_;

  net::TestURLFetcherFactory url_fetcher_factory_;

  // KeyPressEventCallback that serves as a sink to ensure that every key press
  // event the tests create and have the WebContents forward is handled by some
  // key press event callback. It is necessary to have this sinkbecause if no
  // key press event callback handles the event (at least on Mac), a DCHECK
  // ends up going off that the |event| doesn't have an |os_event| associated
  // with it.
  content::RenderWidgetHost::KeyPressEventCallback key_press_event_sink_;

  // Indicates if AutofillExpandedPopupViews is enabled.
  const bool popup_views_enabled_;

  base::test::ScopedFeatureList scoped_feature_list_;

  DISALLOW_COPY_AND_ASSIGN(AutofillInteractiveTestBase);
};

// AutofillInteractiveTest ----------------------------------------------------

// Test params:
//  - bool popup_views_enabled: whether feature AutofillExpandedPopupViews
//        is enabled for testing.
class AutofillInteractiveTest : public AutofillInteractiveTestBase,
                                public testing::WithParamInterface<bool> {
 protected:
  AutofillInteractiveTest() : AutofillInteractiveTestBase(GetParam()) {}
  ~AutofillInteractiveTest() override = default;
};

// Test that basic form fill is working.
IN_PROC_BROWSER_TEST_P(AutofillInteractiveTest, BasicFormFill) {
  CreateTestProfile();

  // Load the test page.
  ASSERT_NO_FATAL_FAILURE(ui_test_utils::NavigateToURL(browser(),
      GURL(std::string(kDataURIPrefix) + kTestFormString)));

  // Invoke Autofill.
  TryBasicFormFill();
}

// Test that form filling can be initiated by pressing the down arrow.
IN_PROC_BROWSER_TEST_P(AutofillInteractiveTest, AutofillViaDownArrow) {
  CreateTestProfile();

  // Load the test page.
  ASSERT_NO_FATAL_FAILURE(ui_test_utils::NavigateToURL(browser(),
      GURL(std::string(kDataURIPrefix) + kTestFormString)));

  // Focus a fillable field.
  FocusFirstNameField();

  // Press the down arrow to initiate Autofill and wait for the popup to be
  // shown.
  SendKeyToPageAndWait(ui::DomKey::ARROW_DOWN);

  // Press the down arrow to select the suggestion and preview the autofilled
  // form.
  SendKeyToPopupAndWait(ui::DomKey::ARROW_DOWN);

  // Press Enter to accept the autofill suggestions.
  SendKeyToPopupAndWait(ui::DomKey::ENTER);

  // The form should be filled.
  ExpectFilledTestForm();
}

IN_PROC_BROWSER_TEST_P(AutofillInteractiveTest, AutofillSelectViaTab) {
  CreateTestProfile();

  // Load the test page.
  ASSERT_NO_FATAL_FAILURE(ui_test_utils::NavigateToURL(browser(),
      GURL(std::string(kDataURIPrefix) + kTestFormString)));

  // Focus a fillable field.
  FocusFirstNameField();

  // Press the down arrow to initiate Autofill and wait for the popup to be
  // shown.
  SendKeyToPageAndWait(ui::DomKey::ARROW_DOWN);

  // Press the down arrow to select the suggestion and preview the autofilled
  // form.
  SendKeyToPopupAndWait(ui::DomKey::ARROW_DOWN);

  // Press tab to accept the autofill suggestions.
  SendKeyToPopupAndWait(ui::DomKey::TAB);

  // The form should be filled.
  ExpectFilledTestForm();
}

IN_PROC_BROWSER_TEST_P(AutofillInteractiveTest, AutofillViaClick) {
  CreateTestProfile();

  // Load the test page.
  ASSERT_NO_FATAL_FAILURE(ui_test_utils::NavigateToURL(
      browser(), GURL(std::string(kDataURIPrefix) + kTestFormString)));
  // Focus a fillable field.
  ASSERT_NO_FATAL_FAILURE(FocusFirstNameField());

  // Now click it.
  test_delegate()->Reset();
  ASSERT_NO_FATAL_FAILURE(ClickFirstNameField());
  test_delegate()->Wait();

  // Press the down arrow to select the suggestion and preview the autofilled
  // form.
  SendKeyToPopupAndWait(ui::DomKey::ARROW_DOWN);

  // Press Enter to accept the autofill suggestions.
  SendKeyToPopupAndWait(ui::DomKey::ENTER);

  // The form should be filled.
  ExpectFilledTestForm();
}

// Test params:
//  - bool popup_views_enabled_: whether feature AutofillExpandedPopupViews
//        is enabled.
//  - bool single_click_enabled_: whether AutofillSingleClick is enabled.
class AutofillSingleClickTest
    : public AutofillInteractiveTestBase,
      public testing::WithParamInterface<std::tuple<bool, bool>> {
 protected:
  AutofillSingleClickTest()
      : AutofillInteractiveTestBase(std::get<0>(GetParam())),
        single_click_enabled_(std::get<1>(GetParam())) {}

  void SetUp() override {
    scoped_feature_list_.InitWithFeatureState(features::kSingleClickAutofill,
                                              single_click_enabled_);
    AutofillInteractiveTestBase::SetUp();
  }

  const bool single_click_enabled_;

 private:
  base::test::ScopedFeatureList scoped_feature_list_;
};

// Depending on whether or not AutofillSingleClick is enabled, makes sure that
// the first click does or does not activate the autofill popup on the initial
// click within a fillable field.
IN_PROC_BROWSER_TEST_P(AutofillSingleClickTest, Click) {
  // Make sure autofill data exists.
  CreateTestProfile();

  // Load the test page.
  ASSERT_NO_FATAL_FAILURE(ui_test_utils::NavigateToURL(
      browser(), GURL(std::string(kDataURIPrefix) + kTestFormString)));

  // If AutofillSingleClick is NOT enabled, then the first time we click on the
  // first name field, nothing should happen.
  if (!single_click_enabled_) {
    // Click the first name field while it's out of focus, then twiddle our
    // thumbs a bit. If the autofill popup shows, it will hit the CHECKs in
    // AutofillManagerTestDelegateImpl while we're waiting.
    ASSERT_NO_FATAL_FAILURE(ClickFirstNameField());
    ASSERT_NO_FATAL_FAILURE(MakeSurePopupDoesntAppear());
  }

  // This click should activate the autofill popup.
  test_delegate()->Reset();
  ASSERT_NO_FATAL_FAILURE(ClickFirstNameField());
  test_delegate()->Wait();

  // Press the down arrow to select the suggestion and preview the autofilled
  // form.
  SendKeyToPopupAndWait(ui::DomKey::ARROW_DOWN);

  // Press Enter to accept the autofill suggestions.
  SendKeyToPopupAndWait(ui::DomKey::ENTER);

  // The form should be filled.
  ExpectFilledTestForm();
}

// Makes sure that clicking outside the focused field doesn't activate
// the popup.
IN_PROC_BROWSER_TEST_P(AutofillInteractiveTest, DontAutofillForOutsideClick) {
  CreateTestProfile();

  // Load the test page.
  ASSERT_NO_FATAL_FAILURE(ui_test_utils::NavigateToURL(
      browser(),
      GURL(std::string(kDataURIPrefix) + kTestFormString +
           "<button disabled id='disabled-button'>Cant click this</button>")));

  ASSERT_NO_FATAL_FAILURE(FocusFirstNameField());

  // Clicking a disabled button will generate a mouse event but focus doesn't
  // change. This tests that autofill can handle a mouse event outside a focused
  // input *without* showing the popup.
  ASSERT_NO_FATAL_FAILURE(ClickElementWithId("disabled-button"));
  ASSERT_NO_FATAL_FAILURE(MakeSurePopupDoesntAppear());

  test_delegate()->Reset();
  ASSERT_NO_FATAL_FAILURE(ClickFirstNameField());
  test_delegate()->Wait();
}

// Test that a field is still autofillable after the previously autofilled
// value is deleted.
IN_PROC_BROWSER_TEST_P(AutofillInteractiveTest, OnDeleteValueAfterAutofill) {
  CreateTestProfile();

  // Load the test page.
  ASSERT_NO_FATAL_FAILURE(ui_test_utils::NavigateToURL(browser(),
      GURL(std::string(kDataURIPrefix) + kTestFormString)));

  // Invoke and accept the Autofill popup and verify the form was filled.
  FocusFirstNameField();
  SendKeyToPageAndWait(ui::DomKey::FromCharacter('M'), ui::DomCode::US_M,
                       ui::VKEY_M);
  SendKeyToPopupAndWait(ui::DomKey::ARROW_DOWN);
  SendKeyToPopupAndWait(ui::DomKey::ENTER);
  ExpectFilledTestForm();

  // Delete the value of a filled field.
  ASSERT_TRUE(content::ExecuteScript(
      GetRenderViewHost(),
      "document.getElementById('firstname').value = '';"));
  ExpectFieldValue("firstname", "");

  // Invoke and accept the Autofill popup and verify the field was filled.
  SendKeyToPageAndWait(ui::DomKey::FromCharacter('M'), ui::DomCode::US_M,
                       ui::VKEY_M);
  SendKeyToPopupAndWait(ui::DomKey::ARROW_DOWN);
  SendKeyToPopupAndWait(ui::DomKey::ENTER);
  ExpectFieldValue("firstname", "Milton");
}

// Test that an input field is not rendered with the yellow autofilled
// background color when choosing an option from the datalist suggestion list.
#if defined(OS_MACOSX) || defined(OS_CHROMEOS) || defined(OS_WIN) || \
    defined(OS_LINUX)
// Flakily triggers and assert on Mac; flakily gets empty string instead
// of "Adam" on ChromeOS.
// http://crbug.com/419868, http://crbug.com/595385.
// Flaky on Windows and Linux as well: http://crbug.com/595385
#define MAYBE_OnSelectOptionFromDatalist DISABLED_OnSelectOptionFromDatalist
#else
#define MAYBE_OnSelectOptionFromDatalist OnSelectOptionFromDatalist
#endif
IN_PROC_BROWSER_TEST_P(AutofillInteractiveTest,
                       MAYBE_OnSelectOptionFromDatalist) {
  // Load the test page.
  ASSERT_NO_FATAL_FAILURE(ui_test_utils::NavigateToURL(
      browser(),
      GURL(std::string(kDataURIPrefix) +
           "<form action=\"http://www.example.com/\" method=\"POST\">"
           "  <input list=\"dl\" type=\"search\" id=\"firstname\"><br>"
           "  <datalist id=\"dl\">"
           "  <option value=\"Adam\"></option>"
           "  <option value=\"Bob\"></option>"
           "  <option value=\"Carl\"></option>"
           "  </datalist>"
           "</form>")));
  std::string orginalcolor;
  GetFieldBackgroundColor("firstname", &orginalcolor);

  FocusFirstNameField();
  SendKeyToPageAndWait(ui::DomKey::ARROW_DOWN);
  SendKeyToDataListPopup(ui::DomKey::ARROW_DOWN);
  SendKeyToDataListPopup(ui::DomKey::ENTER);
  ExpectFieldValue("firstname", "Adam");
  std::string color;
  GetFieldBackgroundColor("firstname", &color);
  EXPECT_EQ(color, orginalcolor);
}

// Test that a JavaScript oninput event is fired after auto-filling a form.
IN_PROC_BROWSER_TEST_P(AutofillInteractiveTest, OnInputAfterAutofill) {
  CreateTestProfile();

  const char kOnInputScript[] =
      "<script>"
      "focused_fired = false;"
      "unfocused_fired = false;"
      "changed_select_fired = false;"
      "unchanged_select_fired = false;"
      "document.getElementById('firstname').oninput = function() {"
      "  focused_fired = true;"
      "};"
      "document.getElementById('lastname').oninput = function() {"
      "  unfocused_fired = true;"
      "};"
      "document.getElementById('state').oninput = function() {"
      "  changed_select_fired = true;"
      "};"
      "document.getElementById('country').oninput = function() {"
      "  unchanged_select_fired = true;"
      "};"
      "document.getElementById('country').value = 'US';"
      "</script>";

  // Load the test page.
  ASSERT_NO_FATAL_FAILURE(ui_test_utils::NavigateToURL(browser(),
      GURL(std::string(kDataURIPrefix) + kTestFormString + kOnInputScript)));

  // Invoke Autofill.
  FocusFirstNameField();

  // Start filling the first name field with "M" and wait for the popup to be
  // shown.
  SendKeyToPageAndWait(ui::DomKey::FromCharacter('M'), ui::DomCode::US_M,
                       ui::VKEY_M);

  // Press the down arrow to select the suggestion and preview the autofilled
  // form.
  SendKeyToPopupAndWait(ui::DomKey::ARROW_DOWN);

  // Press Enter to accept the autofill suggestions.
  SendKeyToPopupAndWait(ui::DomKey::ENTER);

  // The form should be filled.
  ExpectFilledTestForm();

  bool focused_fired = false;
  bool unfocused_fired = false;
  bool changed_select_fired = false;
  bool unchanged_select_fired = false;
  ASSERT_TRUE(content::ExecuteScriptAndExtractBool(
      GetRenderViewHost(),
      "domAutomationController.send(focused_fired);",
      &focused_fired));
  ASSERT_TRUE(content::ExecuteScriptAndExtractBool(
      GetRenderViewHost(),
      "domAutomationController.send(unfocused_fired);",
      &unfocused_fired));
  ASSERT_TRUE(content::ExecuteScriptAndExtractBool(
      GetRenderViewHost(),
      "domAutomationController.send(changed_select_fired);",
      &changed_select_fired));
  ASSERT_TRUE(content::ExecuteScriptAndExtractBool(
      GetRenderViewHost(),
      "domAutomationController.send(unchanged_select_fired);",
      &unchanged_select_fired));
  EXPECT_TRUE(focused_fired);
  EXPECT_TRUE(unfocused_fired);
  EXPECT_TRUE(changed_select_fired);
  EXPECT_FALSE(unchanged_select_fired);
}

// Test that a JavaScript onchange event is fired after auto-filling a form.
IN_PROC_BROWSER_TEST_P(AutofillInteractiveTest, OnChangeAfterAutofill) {
  CreateTestProfile();

  const char kOnChangeScript[] =
      "<script>"
      "focused_fired = false;"
      "unfocused_fired = false;"
      "changed_select_fired = false;"
      "unchanged_select_fired = false;"
      "document.getElementById('firstname').onchange = function() {"
      "  focused_fired = true;"
      "};"
      "document.getElementById('lastname').onchange = function() {"
      "  unfocused_fired = true;"
      "};"
      "document.getElementById('state').onchange = function() {"
      "  changed_select_fired = true;"
      "};"
      "document.getElementById('country').onchange = function() {"
      "  unchanged_select_fired = true;"
      "};"
      "document.getElementById('country').value = 'US';"
      "</script>";

  // Load the test page.
  ASSERT_NO_FATAL_FAILURE(ui_test_utils::NavigateToURL(browser(),
      GURL(std::string(kDataURIPrefix) + kTestFormString + kOnChangeScript)));

  // Invoke Autofill.
  FocusFirstNameField();

  // Start filling the first name field with "M" and wait for the popup to be
  // shown.
  SendKeyToPageAndWait(ui::DomKey::FromCharacter('M'), ui::DomCode::US_M,
                       ui::VKEY_M);

  // Press the down arrow to select the suggestion and preview the autofilled
  // form.
  SendKeyToPopupAndWait(ui::DomKey::ARROW_DOWN);

  // Press Enter to accept the autofill suggestions.
  SendKeyToPopupAndWait(ui::DomKey::ENTER);

  // The form should be filled.
  ExpectFilledTestForm();

  bool focused_fired = false;
  bool unfocused_fired = false;
  bool changed_select_fired = false;
  bool unchanged_select_fired = false;
  ASSERT_TRUE(content::ExecuteScriptAndExtractBool(
      GetRenderViewHost(),
      "domAutomationController.send(focused_fired);",
      &focused_fired));
  ASSERT_TRUE(content::ExecuteScriptAndExtractBool(
      GetRenderViewHost(),
      "domAutomationController.send(unfocused_fired);",
      &unfocused_fired));
  ASSERT_TRUE(content::ExecuteScriptAndExtractBool(
      GetRenderViewHost(),
      "domAutomationController.send(changed_select_fired);",
      &changed_select_fired));
  ASSERT_TRUE(content::ExecuteScriptAndExtractBool(
      GetRenderViewHost(),
      "domAutomationController.send(unchanged_select_fired);",
      &unchanged_select_fired));
  EXPECT_TRUE(focused_fired);
  EXPECT_TRUE(unfocused_fired);
  EXPECT_TRUE(changed_select_fired);
  EXPECT_FALSE(unchanged_select_fired);
}

IN_PROC_BROWSER_TEST_P(AutofillInteractiveTest, InputFiresBeforeChange) {
  CreateTestProfile();

  const char kInputFiresBeforeChangeScript[] =
      "<script>"
      "inputElementEvents = [];"
      "function recordInputElementEvent(e) {"
      "  if (e.target.tagName != 'INPUT') throw 'only <input> tags allowed';"
      "  inputElementEvents.push(e.type);"
      "}"
      "selectElementEvents = [];"
      "function recordSelectElementEvent(e) {"
      "  if (e.target.tagName != 'SELECT') throw 'only <select> tags allowed';"
      "  selectElementEvents.push(e.type);"
      "}"
      "document.getElementById('lastname').oninput = recordInputElementEvent;"
      "document.getElementById('lastname').onchange = recordInputElementEvent;"
      "document.getElementById('country').oninput = recordSelectElementEvent;"
      "document.getElementById('country').onchange = recordSelectElementEvent;"
      "</script>";

  // Load the test page.
  ASSERT_NO_FATAL_FAILURE(ui_test_utils::NavigateToURL(browser(),
      GURL(std::string(kDataURIPrefix) + kTestFormString +
           kInputFiresBeforeChangeScript)));

  // Invoke and accept the Autofill popup and verify the form was filled.
  FocusFirstNameField();
  SendKeyToPageAndWait(ui::DomKey::FromCharacter('M'), ui::DomCode::US_M,
                       ui::VKEY_M);
  SendKeyToPopupAndWait(ui::DomKey::ARROW_DOWN);
  SendKeyToPopupAndWait(ui::DomKey::ENTER);
  ExpectFilledTestForm();

  int num_input_element_events = -1;
  ASSERT_TRUE(content::ExecuteScriptAndExtractInt(
      GetRenderViewHost(),
      "domAutomationController.send(inputElementEvents.length);",
      &num_input_element_events));
  EXPECT_EQ(2, num_input_element_events);

  std::vector<std::string> input_element_events;
  input_element_events.resize(2);

  ASSERT_TRUE(content::ExecuteScriptAndExtractString(
      GetRenderViewHost(),
      "domAutomationController.send(inputElementEvents[0]);",
      &input_element_events[0]));
  ASSERT_TRUE(content::ExecuteScriptAndExtractString(
      GetRenderViewHost(),
      "domAutomationController.send(inputElementEvents[1]);",
      &input_element_events[1]));

  EXPECT_EQ("input", input_element_events[0]);
  EXPECT_EQ("change", input_element_events[1]);

  int num_select_element_events = -1;
  ASSERT_TRUE(content::ExecuteScriptAndExtractInt(
      GetRenderViewHost(),
      "domAutomationController.send(selectElementEvents.length);",
      &num_select_element_events));
  EXPECT_EQ(2, num_select_element_events);

  std::vector<std::string> select_element_events;
  select_element_events.resize(2);

  ASSERT_TRUE(content::ExecuteScriptAndExtractString(
      GetRenderViewHost(),
      "domAutomationController.send(selectElementEvents[0]);",
      &select_element_events[0]));
  ASSERT_TRUE(content::ExecuteScriptAndExtractString(
      GetRenderViewHost(),
      "domAutomationController.send(selectElementEvents[1]);",
      &select_element_events[1]));

  EXPECT_EQ("input", select_element_events[0]);
  EXPECT_EQ("change", select_element_events[1]);
}

// Test that we can autofill forms distinguished only by their |id| attribute.
IN_PROC_BROWSER_TEST_P(AutofillInteractiveTest,
                       AutofillFormsDistinguishedById) {
  CreateTestProfile();

  // Load the test page.
  const std::string kURL =
      std::string(kDataURIPrefix) + kTestFormString +
      "<script>"
      "var mainForm = document.forms[0];"
      "mainForm.id = 'mainForm';"
      "var newForm = document.createElement('form');"
      "newForm.action = mainForm.action;"
      "newForm.method = mainForm.method;"
      "newForm.id = 'newForm';"
      "mainForm.parentNode.insertBefore(newForm, mainForm);"
      "</script>";
  ASSERT_NO_FATAL_FAILURE(ui_test_utils::NavigateToURL(browser(), GURL(kURL)));

  // Invoke Autofill.
  TryBasicFormFill();
}

// Test that we properly autofill forms with repeated fields.
// In the wild, the repeated fields are typically either email fields
// (duplicated for "confirmation"); or variants that are hot-swapped via
// JavaScript, with only one actually visible at any given time.
IN_PROC_BROWSER_TEST_P(AutofillInteractiveTest, AutofillFormWithRepeatedField) {
  CreateTestProfile();

  // Load the test page.
  ASSERT_NO_FATAL_FAILURE(ui_test_utils::NavigateToURL(browser(),
      GURL(std::string(kDataURIPrefix) +
           "<form action=\"http://www.example.com/\" method=\"POST\">"
           "<label for=\"firstname\">First name:</label>"
           " <input type=\"text\" id=\"firstname\""
           "        onfocus=\"domAutomationController.send(true)\"><br>"
           "<label for=\"lastname\">Last name:</label>"
           " <input type=\"text\" id=\"lastname\"><br>"
           "<label for=\"address1\">Address line 1:</label>"
           " <input type=\"text\" id=\"address1\"><br>"
           "<label for=\"address2\">Address line 2:</label>"
           " <input type=\"text\" id=\"address2\"><br>"
           "<label for=\"city\">City:</label>"
           " <input type=\"text\" id=\"city\"><br>"
           "<label for=\"state\">State:</label>"
           " <select id=\"state\">"
           " <option value=\"\" selected=\"yes\">--</option>"
           " <option value=\"CA\">California</option>"
           " <option value=\"TX\">Texas</option>"
           " </select><br>"
           "<label for=\"state_freeform\" style=\"display:none\">State:</label>"
           " <input type=\"text\" id=\"state_freeform\""
           "        style=\"display:none\"><br>"
           "<label for=\"zip\">ZIP code:</label>"
           " <input type=\"text\" id=\"zip\"><br>"
           "<label for=\"country\">Country:</label>"
           " <select id=\"country\">"
           " <option value=\"\" selected=\"yes\">--</option>"
           " <option value=\"CA\">Canada</option>"
           " <option value=\"US\">United States</option>"
           " </select><br>"
           "<label for=\"phone\">Phone number:</label>"
           " <input type=\"text\" id=\"phone\"><br>"
           "</form>")));

  // Invoke Autofill.
  TryBasicFormFill();
  ExpectFieldValue("state_freeform", std::string());
}

// Test that we properly autofill forms with non-autofillable fields.
IN_PROC_BROWSER_TEST_P(AutofillInteractiveTest,
                       AutofillFormWithNonAutofillableField) {
  CreateTestProfile();

  // Load the test page.
  ASSERT_NO_FATAL_FAILURE(ui_test_utils::NavigateToURL(browser(),
      GURL(std::string(kDataURIPrefix) +
           "<form action=\"http://www.example.com/\" method=\"POST\">"
           "<label for=\"firstname\">First name:</label>"
           " <input type=\"text\" id=\"firstname\""
           "        onfocus=\"domAutomationController.send(true)\"><br>"
           "<label for=\"middlename\">Middle name:</label>"
           " <input type=\"text\" id=\"middlename\" autocomplete=\"off\" /><br>"
           "<label for=\"lastname\">Last name:</label>"
           " <input type=\"text\" id=\"lastname\"><br>"
           "<label for=\"address1\">Address line 1:</label>"
           " <input type=\"text\" id=\"address1\"><br>"
           "<label for=\"address2\">Address line 2:</label>"
           " <input type=\"text\" id=\"address2\"><br>"
           "<label for=\"city\">City:</label>"
           " <input type=\"text\" id=\"city\"><br>"
           "<label for=\"state\">State:</label>"
           " <select id=\"state\">"
           " <option value=\"\" selected=\"yes\">--</option>"
           " <option value=\"CA\">California</option>"
           " <option value=\"TX\">Texas</option>"
           " </select><br>"
           "<label for=\"zip\">ZIP code:</label>"
           " <input type=\"text\" id=\"zip\"><br>"
           "<label for=\"country\">Country:</label>"
           " <select id=\"country\">"
           " <option value=\"\" selected=\"yes\">--</option>"
           " <option value=\"CA\">Canada</option>"
           " <option value=\"US\">United States</option>"
           " </select><br>"
           "<label for=\"phone\">Phone number:</label>"
           " <input type=\"text\" id=\"phone\"><br>"
           "</form>")));

  // Invoke Autofill.
  TryBasicFormFill();
}

// Test that we can Autofill dynamically generated forms.
IN_PROC_BROWSER_TEST_P(AutofillInteractiveTest, DynamicFormFill) {
  CreateTestProfile();

  // Load the test page.
  ASSERT_NO_FATAL_FAILURE(ui_test_utils::NavigateToURL(browser(),
      GURL(std::string(kDataURIPrefix) +
           "<form id=\"form\" action=\"http://www.example.com/\""
           "      method=\"POST\"></form>"
           "<script>"
           "function AddElement(name, label) {"
           "  var form = document.getElementById('form');"
           ""
           "  var label_text = document.createTextNode(label);"
           "  var label_element = document.createElement('label');"
           "  label_element.setAttribute('for', name);"
           "  label_element.appendChild(label_text);"
           "  form.appendChild(label_element);"
           ""
           "  if (name === 'state' || name === 'country') {"
           "    var select_element = document.createElement('select');"
           "    select_element.setAttribute('id', name);"
           "    select_element.setAttribute('name', name);"
           ""
           "    /* Add an empty selected option. */"
           "    var default_option = new Option('--', '', true);"
           "    select_element.appendChild(default_option);"
           ""
           "    /* Add the other options. */"
           "    if (name == 'state') {"
           "      var option1 = new Option('California', 'CA');"
           "      select_element.appendChild(option1);"
           "      var option2 = new Option('Texas', 'TX');"
           "      select_element.appendChild(option2);"
           "    } else {"
           "      var option1 = new Option('Canada', 'CA');"
           "      select_element.appendChild(option1);"
           "      var option2 = new Option('United States', 'US');"
           "      select_element.appendChild(option2);"
           "    }"
           ""
           "    form.appendChild(select_element);"
           "  } else {"
           "    var input_element = document.createElement('input');"
           "    input_element.setAttribute('id', name);"
           "    input_element.setAttribute('name', name);"
           ""
           "    /* Add the onfocus listener to the 'firstname' field. */"
           "    if (name === 'firstname') {"
           "      input_element.onfocus = function() {"
           "        domAutomationController.send(true);"
           "      };"
           "    }"
           ""
           "    form.appendChild(input_element);"
           "  }"
           ""
           "  form.appendChild(document.createElement('br'));"
           "};"
           ""
           "function BuildForm() {"
           "  var elements = ["
           "    ['firstname', 'First name:'],"
           "    ['lastname', 'Last name:'],"
           "    ['address1', 'Address line 1:'],"
           "    ['address2', 'Address line 2:'],"
           "    ['city', 'City:'],"
           "    ['state', 'State:'],"
           "    ['zip', 'ZIP code:'],"
           "    ['country', 'Country:'],"
           "    ['phone', 'Phone number:'],"
           "  ];"
           ""
           "  for (var i = 0; i < elements.length; i++) {"
           "    var name = elements[i][0];"
           "    var label = elements[i][1];"
           "    AddElement(name, label);"
           "  }"
           "};"
           "</script>")));

  // Dynamically construct the form.
  ASSERT_TRUE(content::ExecuteScript(GetRenderViewHost(), "BuildForm();"));

  // Invoke Autofill.
  TryBasicFormFill();
}

// Test that form filling works after reloading the current page.
IN_PROC_BROWSER_TEST_P(AutofillInteractiveTest, AutofillAfterReload) {
  CreateTestProfile();

  // Load the test page.
  ASSERT_NO_FATAL_FAILURE(ui_test_utils::NavigateToURL(browser(),
      GURL(std::string(kDataURIPrefix) + kTestFormString)));

  // Reload the page.
  content::WebContents* web_contents = GetWebContents();
  web_contents->GetController().Reload(content::ReloadType::NORMAL, false);
  content::WaitForLoadStop(web_contents);

  // Invoke Autofill.
  TryBasicFormFill();
}

// Test that filling a form sends all the expected events to the different
// fields being filled.
IN_PROC_BROWSER_TEST_P(AutofillInteractiveTest, AutofillEvents) {
  CreateTestProfile();

  // Load the test page.
  ASSERT_NO_FATAL_FAILURE(ui_test_utils::NavigateToURL(
      browser(), GURL(std::string(kDataURIPrefix) + kTestEventFormString)));

  // Invoke Autofill.
  TryBasicFormFill();

  // Checks that all the events were fired for the input field.
  bool input_focus_triggered;
  EXPECT_TRUE(content::ExecuteScriptAndExtractBool(
      GetRenderViewHost(), "domAutomationController.send(inputfocus);",
      &input_focus_triggered));
  EXPECT_TRUE(input_focus_triggered);
  bool input_keydown_triggered;
  EXPECT_TRUE(content::ExecuteScriptAndExtractBool(
      GetRenderViewHost(), "domAutomationController.send(inputkeydown);",
      &input_keydown_triggered));
  EXPECT_TRUE(input_keydown_triggered);
  bool input_input_triggered;
  EXPECT_TRUE(content::ExecuteScriptAndExtractBool(
      GetRenderViewHost(), "domAutomationController.send(inputinput);",
      &input_input_triggered));
  EXPECT_TRUE(input_input_triggered);
  bool input_change_triggered;
  EXPECT_TRUE(content::ExecuteScriptAndExtractBool(
      GetRenderViewHost(), "domAutomationController.send(inputchange);",
      &input_change_triggered));
  EXPECT_TRUE(input_change_triggered);
  bool input_keyup_triggered;
  EXPECT_TRUE(content::ExecuteScriptAndExtractBool(
      GetRenderViewHost(), "domAutomationController.send(inputkeyup);",
      &input_keyup_triggered));
  EXPECT_TRUE(input_keyup_triggered);
  bool input_blur_triggered;
  EXPECT_TRUE(content::ExecuteScriptAndExtractBool(
      GetRenderViewHost(), "domAutomationController.send(inputblur);",
      &input_blur_triggered));
  EXPECT_TRUE(input_blur_triggered);

  // Checks that all the events were fired for the textarea field.
  bool text_focus_triggered;
  EXPECT_TRUE(content::ExecuteScriptAndExtractBool(
      GetRenderViewHost(), "domAutomationController.send(textfocus);",
      &text_focus_triggered));
  EXPECT_TRUE(text_focus_triggered);
  bool text_keydown_triggered;
  EXPECT_TRUE(content::ExecuteScriptAndExtractBool(
      GetRenderViewHost(), "domAutomationController.send(textkeydown);",
      &text_keydown_triggered));
  EXPECT_TRUE(text_keydown_triggered);
  bool text_input_triggered;
  EXPECT_TRUE(content::ExecuteScriptAndExtractBool(
      GetRenderViewHost(), "domAutomationController.send(textinput);",
      &text_input_triggered));
  EXPECT_TRUE(text_input_triggered);
  bool text_change_triggered;
  EXPECT_TRUE(content::ExecuteScriptAndExtractBool(
      GetRenderViewHost(), "domAutomationController.send(textchange);",
      &text_change_triggered));
  EXPECT_TRUE(text_change_triggered);
  bool text_keyup_triggered;
  EXPECT_TRUE(content::ExecuteScriptAndExtractBool(
      GetRenderViewHost(), "domAutomationController.send(textkeyup);",
      &text_keyup_triggered));
  EXPECT_TRUE(text_keyup_triggered);
  bool text_blur_triggered;
  EXPECT_TRUE(content::ExecuteScriptAndExtractBool(
      GetRenderViewHost(), "domAutomationController.send(textblur);",
      &text_blur_triggered));
  EXPECT_TRUE(text_blur_triggered);

  // Checks that all the events were fired for the select field.
  bool select_focus_triggered;
  EXPECT_TRUE(content::ExecuteScriptAndExtractBool(
      GetRenderViewHost(), "domAutomationController.send(selectfocus);",
      &select_focus_triggered));
  EXPECT_TRUE(select_focus_triggered);
  bool select_input_triggered;
  EXPECT_TRUE(content::ExecuteScriptAndExtractBool(
      GetRenderViewHost(), "domAutomationController.send(selectinput);",
      &select_input_triggered));
  EXPECT_TRUE(select_input_triggered);
  bool select_change_triggered;
  EXPECT_TRUE(content::ExecuteScriptAndExtractBool(
      GetRenderViewHost(), "domAutomationController.send(selectchange);",
      &select_change_triggered));
  EXPECT_TRUE(select_change_triggered);
  bool select_blur_triggered;
  EXPECT_TRUE(content::ExecuteScriptAndExtractBool(
      GetRenderViewHost(), "domAutomationController.send(selectblur);",
      &select_blur_triggered));
  EXPECT_TRUE(select_blur_triggered);
}

// Test fails on Linux ASAN, see http://crbug.com/532737
#if defined(ADDRESS_SANITIZER)
#define MAYBE_AutofillAfterTranslate DISABLED_AutofillAfterTranslate
#else
#define MAYBE_AutofillAfterTranslate AutofillAfterTranslate
#endif  // ADDRESS_SANITIZER
IN_PROC_BROWSER_TEST_P(AutofillInteractiveTest, MAYBE_AutofillAfterTranslate) {
  ASSERT_TRUE(TranslateService::IsTranslateBubbleEnabled());

  translate::TranslateManager::SetIgnoreMissingKeyForTesting(true);

  CreateTestProfile();

  GURL url(std::string(kDataURIPrefix) +
               "<form action=\"http://www.example.com/\" method=\"POST\">"
               "<label for=\"fn\">なまえ</label>"
               " <input type=\"text\" id=\"fn\""
               "        onfocus=\"domAutomationController.send(true)\""
               "><br>"
               "<label for=\"ln\">みょうじ</label>"
               " <input type=\"text\" id=\"ln\"><br>"
               "<label for=\"a1\">Address line 1:</label>"
               " <input type=\"text\" id=\"a1\"><br>"
               "<label for=\"a2\">Address line 2:</label>"
               " <input type=\"text\" id=\"a2\"><br>"
               "<label for=\"ci\">City:</label>"
               " <input type=\"text\" id=\"ci\"><br>"
               "<label for=\"st\">State:</label>"
               " <select id=\"st\">"
               " <option value=\"\" selected=\"yes\">--</option>"
               " <option value=\"CA\">California</option>"
               " <option value=\"TX\">Texas</option>"
               " </select><br>"
               "<label for=\"z\">ZIP code:</label>"
               " <input type=\"text\" id=\"z\"><br>"
               "<label for=\"co\">Country:</label>"
               " <select id=\"co\">"
               " <option value=\"\" selected=\"yes\">--</option>"
               " <option value=\"CA\">Canada</option>"
               " <option value=\"US\">United States</option>"
               " </select><br>"
               "<label for=\"ph\">Phone number:</label>"
               " <input type=\"text\" id=\"ph\"><br>"
               "</form>"
               // Add additional Japanese characters to ensure the translate bar
               // will appear.
               "我々は重要な、興味深いものになるが、時折状況が発生するため苦労や痛みは"
               "彼にいくつかの素晴らしいを調達することができます。それから、いくつかの利");

  // Set up an observer to be able to wait for the bubble to be shown.
  content::Source<content::WebContents> source(GetWebContents());
  content::WindowedNotificationObserver language_detected_signal(
      chrome::NOTIFICATION_TAB_LANGUAGE_DETERMINED, source);

  ASSERT_NO_FATAL_FAILURE(
      ui_test_utils::NavigateToURL(browser(), url));

  // Wait for the translate bubble to appear.
  language_detected_signal.Wait();

  // Verify current translate step.
  const TranslateBubbleModel* model =
      translate::test_utils::GetCurrentModel(browser());
  ASSERT_NE(nullptr, model);
  EXPECT_EQ(TranslateBubbleModel::VIEW_STATE_BEFORE_TRANSLATE,
            model->GetViewState());

  translate::test_utils::PressTranslate(browser());

  // Wait for translation.
  content::WindowedNotificationObserver translation_observer(
      chrome::NOTIFICATION_PAGE_TRANSLATED,
      content::NotificationService::AllSources());

  // Simulate the translate script being retrieved.
  // Pass fake google.translate lib as the translate script.
  SimulateURLFetch(true);

  // Simulate the render notifying the translation has been done.
  translation_observer.Wait();

  TryBasicFormFill();
}

// Test phone fields parse correctly from a given profile.
// The high level key presses execute the following: Select the first text
// field, invoke the autofill popup list, select the first profile within the
// list, and commit to the profile to populate the form.
IN_PROC_BROWSER_TEST_P(AutofillInteractiveTest, ComparePhoneNumbers) {
  AutofillProfile profile;
  profile.SetRawInfo(NAME_FIRST, ASCIIToUTF16("Bob"));
  profile.SetRawInfo(NAME_LAST, ASCIIToUTF16("Smith"));
  profile.SetRawInfo(ADDRESS_HOME_LINE1, ASCIIToUTF16("1234 H St."));
  profile.SetRawInfo(ADDRESS_HOME_CITY, ASCIIToUTF16("San Jose"));
  profile.SetRawInfo(ADDRESS_HOME_STATE, ASCIIToUTF16("CA"));
  profile.SetRawInfo(ADDRESS_HOME_ZIP, ASCIIToUTF16("95110"));
  profile.SetRawInfo(PHONE_HOME_WHOLE_NUMBER, ASCIIToUTF16("1-408-555-4567"));
  SetTestProfile(browser(), profile);

  GURL url = embedded_test_server()->GetURL("/autofill/form_phones.html");
  ui_test_utils::NavigateToURL(browser(), url);
  PopulateForm("NAME_FIRST1");

  ExpectFieldValue("NAME_FIRST1", "Bob");
  ExpectFieldValue("NAME_LAST1", "Smith");
  ExpectFieldValue("ADDRESS_HOME_LINE1", "1234 H St.");
  ExpectFieldValue("ADDRESS_HOME_CITY", "San Jose");
  ExpectFieldValue("ADDRESS_HOME_STATE", "CA");
  ExpectFieldValue("ADDRESS_HOME_ZIP", "95110");
  ExpectFieldValue("PHONE_HOME_WHOLE_NUMBER", "14085554567");

  PopulateForm("NAME_FIRST2");
  ExpectFieldValue("NAME_FIRST2", "Bob");
  ExpectFieldValue("NAME_LAST2", "Smith");
  ExpectFieldValue("PHONE_HOME_CITY_CODE-1", "408");
  ExpectFieldValue("PHONE_HOME_NUMBER", "5554567");

  PopulateForm("NAME_FIRST3");
  ExpectFieldValue("NAME_FIRST3", "Bob");
  ExpectFieldValue("NAME_LAST3", "Smith");
  ExpectFieldValue("PHONE_HOME_CITY_CODE-2", "408");
  ExpectFieldValue("PHONE_HOME_NUMBER_3-1", "555");
  ExpectFieldValue("PHONE_HOME_NUMBER_4-1", "4567");
  ExpectFieldValue("PHONE_HOME_EXT-1", std::string());

  PopulateForm("NAME_FIRST4");
  ExpectFieldValue("NAME_FIRST4", "Bob");
  ExpectFieldValue("NAME_LAST4", "Smith");
  ExpectFieldValue("PHONE_HOME_COUNTRY_CODE-1", "1");
  ExpectFieldValue("PHONE_HOME_CITY_CODE-3", "408");
  ExpectFieldValue("PHONE_HOME_NUMBER_3-2", "555");
  ExpectFieldValue("PHONE_HOME_NUMBER_4-2", "4567");
  ExpectFieldValue("PHONE_HOME_EXT-2", std::string());
}

// Test that Autofill does not fill in read-only fields.
IN_PROC_BROWSER_TEST_P(AutofillInteractiveTest, NoAutofillForReadOnlyFields) {
  std::string addr_line1("1234 H St.");

  AutofillProfile profile;
  profile.SetRawInfo(NAME_FIRST, ASCIIToUTF16("Bob"));
  profile.SetRawInfo(NAME_LAST, ASCIIToUTF16("Smith"));
  profile.SetRawInfo(EMAIL_ADDRESS, ASCIIToUTF16("bsmith@gmail.com"));
  profile.SetRawInfo(ADDRESS_HOME_LINE1, ASCIIToUTF16(addr_line1));
  profile.SetRawInfo(ADDRESS_HOME_CITY, ASCIIToUTF16("San Jose"));
  profile.SetRawInfo(ADDRESS_HOME_STATE, ASCIIToUTF16("CA"));
  profile.SetRawInfo(ADDRESS_HOME_ZIP, ASCIIToUTF16("95110"));
  profile.SetRawInfo(COMPANY_NAME, ASCIIToUTF16("Company X"));
  profile.SetRawInfo(PHONE_HOME_WHOLE_NUMBER, ASCIIToUTF16("408-871-4567"));
  SetTestProfile(browser(), profile);

  GURL url =
      embedded_test_server()->GetURL("/autofill/read_only_field_test.html");
  ui_test_utils::NavigateToURL(browser(), url);
  PopulateForm("firstname");

  ExpectFieldValue("email", std::string());
  ExpectFieldValue("address", addr_line1);
}

// Test form is fillable from a profile after form was reset.
// Steps:
//   1. Fill form using a saved profile.
//   2. Reset the form.
//   3. Fill form using a saved profile.
IN_PROC_BROWSER_TEST_P(AutofillInteractiveTest, FormFillableOnReset) {
  CreateTestProfile();

  GURL url =
      embedded_test_server()->GetURL("/autofill/autofill_test_form.html");
  ui_test_utils::NavigateToURL(browser(), url);
  PopulateForm("NAME_FIRST");

  ASSERT_TRUE(content::ExecuteScript(
       GetWebContents(), "document.getElementById('testform').reset()"));

  PopulateForm("NAME_FIRST");

  ExpectFieldValue("NAME_FIRST", "Milton");
  ExpectFieldValue("NAME_LAST", "Waddams");
  ExpectFieldValue("EMAIL_ADDRESS", "red.swingline@initech.com");
  ExpectFieldValue("ADDRESS_HOME_LINE1", "4120 Freidrich Lane");
  ExpectFieldValue("ADDRESS_HOME_CITY", "Austin");
  ExpectFieldValue("ADDRESS_HOME_STATE", "Texas");
  ExpectFieldValue("ADDRESS_HOME_ZIP", "78744");
  ExpectFieldValue("ADDRESS_HOME_COUNTRY", "United States");
  ExpectFieldValue("PHONE_HOME_WHOLE_NUMBER", "15125551234");
}

// Test Autofill distinguishes a middle initial in a name.
IN_PROC_BROWSER_TEST_P(AutofillInteractiveTest,
                       DistinguishMiddleInitialWithinName) {
  CreateTestProfile();

  GURL url =
      embedded_test_server()->GetURL("/autofill/autofill_middleinit_form.html");
  ui_test_utils::NavigateToURL(browser(), url);
  PopulateForm("NAME_FIRST");

  ExpectFieldValue("NAME_MIDDLE", "C");
}

// Test forms with multiple email addresses are filled properly.
// Entire form should be filled with one user gesture.
IN_PROC_BROWSER_TEST_P(AutofillInteractiveTest,
                       MultipleEmailFilledByOneUserGesture) {
  std::string email("bsmith@gmail.com");

  AutofillProfile profile;
  profile.SetRawInfo(NAME_FIRST, ASCIIToUTF16("Bob"));
  profile.SetRawInfo(NAME_LAST, ASCIIToUTF16("Smith"));
  profile.SetRawInfo(EMAIL_ADDRESS, ASCIIToUTF16(email));
  profile.SetRawInfo(PHONE_HOME_WHOLE_NUMBER, ASCIIToUTF16("4088714567"));
  SetTestProfile(browser(), profile);

  GURL url = embedded_test_server()->GetURL(
      "/autofill/autofill_confirmemail_form.html");
  ui_test_utils::NavigateToURL(browser(), url);
  PopulateForm("NAME_FIRST");

  ExpectFieldValue("EMAIL_CONFIRM", email);
  // TODO(isherman): verify entire form.
}

// Test latency time on form submit with lots of stored Autofill profiles.
// This test verifies when a profile is selected from the Autofill dictionary
// that consists of thousands of profiles, the form does not hang after being
// submitted.
// Flakily times out creating 1500 profiles: http://crbug.com/281527
IN_PROC_BROWSER_TEST_P(AutofillInteractiveTest,
                       DISABLED_FormFillLatencyAfterSubmit) {
  std::vector<std::string> cities;
  cities.push_back("San Jose");
  cities.push_back("San Francisco");
  cities.push_back("Sacramento");
  cities.push_back("Los Angeles");

  std::vector<std::string> streets;
  streets.push_back("St");
  streets.push_back("Ave");
  streets.push_back("Ln");
  streets.push_back("Ct");

  const int kNumProfiles = 1500;
  std::vector<AutofillProfile> profiles;
  for (int i = 0; i < kNumProfiles; i++) {
    AutofillProfile profile;
    base::string16 name(base::IntToString16(i));
    base::string16 email(name + ASCIIToUTF16("@example.com"));
    base::string16 street = ASCIIToUTF16(
        base::IntToString(base::RandInt(0, 10000)) + " " +
        streets[base::RandInt(0, streets.size() - 1)]);
    base::string16 city =
        ASCIIToUTF16(cities[base::RandInt(0, cities.size() - 1)]);
    base::string16 zip(base::IntToString16(base::RandInt(0, 10000)));
    profile.SetRawInfo(NAME_FIRST, name);
    profile.SetRawInfo(EMAIL_ADDRESS, email);
    profile.SetRawInfo(ADDRESS_HOME_LINE1, street);
    profile.SetRawInfo(ADDRESS_HOME_CITY, city);
    profile.SetRawInfo(ADDRESS_HOME_STATE, ASCIIToUTF16("CA"));
    profile.SetRawInfo(ADDRESS_HOME_ZIP, zip);
    profile.SetRawInfo(ADDRESS_HOME_COUNTRY, ASCIIToUTF16("US"));
    profiles.push_back(profile);
  }
  SetTestProfiles(browser(), &profiles);

  GURL url = embedded_test_server()->GetURL(
      "/autofill/latency_after_submit_test.html");
  ui_test_utils::NavigateToURL(browser(), url);
  PopulateForm("NAME_FIRST");

  content::WindowedNotificationObserver load_stop_observer(
      content::NOTIFICATION_LOAD_STOP,
      content::Source<content::NavigationController>(
          &GetWebContents()->GetController()));

  ASSERT_TRUE(content::ExecuteScript(
      GetRenderViewHost(),
      "document.getElementById('testform').submit();"));
  // This will ensure the test didn't hang.
  load_stop_observer.Wait();
}

// Test that Chrome doesn't crash when autocomplete is disabled while the user
// is interacting with the form.  This is a regression test for
// http://crbug.com/160476
IN_PROC_BROWSER_TEST_P(AutofillInteractiveTest,
                       DisableAutocompleteWhileFilling) {
  CreateTestProfile();

  // Load the test page.
  ASSERT_NO_FATAL_FAILURE(ui_test_utils::NavigateToURL(browser(),
      GURL(std::string(kDataURIPrefix) + kTestFormString)));

  // Invoke Autofill: Start filling the first name field with "M" and wait for
  // the popup to be shown.
  FocusFirstNameField();
  SendKeyToPageAndWait(ui::DomKey::FromCharacter('M'), ui::DomCode::US_M,
                       ui::VKEY_M);

  // Now that the popup with suggestions is showing, disable autocomplete for
  // the active field.
  ASSERT_TRUE(content::ExecuteScript(
      GetRenderViewHost(),
      "document.querySelector('input').autocomplete = 'off';"));

  // Press the down arrow to select the suggestion and attempt to preview the
  // autofilled form.
  SendKeyToPopupAndWait(ui::DomKey::ARROW_DOWN);
}

// Test that dynamic forms don't get filled when the feature is disabled.
IN_PROC_BROWSER_TEST_P(AutofillInteractiveTest, DynamicChangingFormFill) {
  // Explicitly disable the filling of dynamic forms.
  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitAndDisableFeature(
      features::kAutofillRequireSecureCreditCardContext);

  CreateTestProfile();

  GURL url =
      embedded_test_server()->GetURL("/autofill/dynamic_form_disabled.html");
  ASSERT_NO_FATAL_FAILURE(ui_test_utils::NavigateToURL(browser(), url));

  TriggerFormFill("firstname");

  // Wait for the re-fill to happen.
  bool has_refilled = false;
  ASSERT_TRUE(content::ExecuteScriptAndExtractBool(
      GetRenderViewHost(), "hasRefilled()", &has_refilled));
  ASSERT_FALSE(has_refilled);

  // Make sure that the new form was not filled.
  ExpectFieldValue("firstname_form1", "");
  ExpectFieldValue("address_form1", "");
  ExpectFieldValue("state_form1", "CA");  // Default value.
  ExpectFieldValue("city_form1", "");
  ExpectFieldValue("company_form1", "");
  ExpectFieldValue("email_form1", "");
  ExpectFieldValue("phone_form1", "");
}

// Test that we can Autofill forms where some fields name change during the
// fill.
IN_PROC_BROWSER_TEST_P(AutofillInteractiveTest, FieldsChangeName) {
  CreateTestProfile();

  GURL url = embedded_test_server()->GetURL(
      "/autofill/field_changing_name_during_fill.html");
  ASSERT_NO_FATAL_FAILURE(ui_test_utils::NavigateToURL(browser(), url));

  TriggerFormFill("firstname");

  // Wait for the fill to happen.
  bool has_filled = false;
  ASSERT_TRUE(content::ExecuteScriptAndExtractBool(GetRenderViewHost(),
                                                   "hasFilled()", &has_filled));
  ASSERT_TRUE(has_filled);

  // Make sure the form was filled correctly.
  ExpectFieldValue("firstname", "Milton");
  ExpectFieldValue("address", "4120 Freidrich Lane");
  ExpectFieldValue("state", "TX");
  ExpectFieldValue("city", "Austin");
  ExpectFieldValue("company", "Initech");
  ExpectFieldValue("email", "red.swingline@initech.com");
  ExpectFieldValue("phone", "15125551234");
}

// Test params:
//  - bool popup_views_enabled_: whether feature AutofillExpandedPopupViews
//        is enabled.
//  - bool restrict_unowned_fields_: whether autofill of unowned fields is
//        restricted to checkout related pages.
class AutofillRestrictUnownedFieldsTest
    : public AutofillInteractiveTestBase,
      public testing::WithParamInterface<std::tuple<bool, bool>> {
 protected:
  AutofillRestrictUnownedFieldsTest()
      : AutofillInteractiveTestBase(std::get<0>(GetParam())),
        restrict_unowned_fields_(std::get<1>(GetParam())) {
    std::vector<base::Feature> enabled;
    std::vector<base::Feature> disabled = {
        features::kAutofillEnforceMinRequiredFieldsForHeuristics,
        features::kAutofillEnforceMinRequiredFieldsForQuery,
        features::kAutofillEnforceMinRequiredFieldsForUpload};
    (restrict_unowned_fields_ ? enabled : disabled)
        .push_back(features::kAutofillRestrictUnownedFieldsToFormlessCheckout);
    scoped_feature_list_.InitWithFeatures(enabled, disabled);
  }

  const bool restrict_unowned_fields_;

 private:
  base::test::ScopedFeatureList scoped_feature_list_;
};

// Test that we do not fill formless non-checkout forms when we enable the
// formless form restrictions.
IN_PROC_BROWSER_TEST_P(AutofillRestrictUnownedFieldsTest, NoAutocomplete) {
  SCOPED_TRACE(base::StringPrintf("restrict_unowned_fields_ = %d",
                                  restrict_unowned_fields_));
  base::HistogramTester histogram;

  CreateTestProfile();

  GURL url =
      embedded_test_server()->GetURL("/autofill/formless_no_autocomplete.html");
  ASSERT_NO_FATAL_FAILURE(ui_test_utils::NavigateToURL(browser(), url));

  // Of unowned forms are restricted, then there are no forms detected.
  if (restrict_unowned_fields_) {
    SubprocessMetricsProvider::MergeHistogramDeltasForTesting();
    // We should only have samples saying that some elements were filtered.
    auto buckets =
        histogram.GetAllSamples("Autofill.UnownedFieldsWereFiltered");
    ASSERT_EQ(1u, buckets.size());
    EXPECT_EQ(1, buckets[0].min);  // The "true" bucket.

    ASSERT_EQ(0U, GetAutofillManager()->NumFormsDetected());
    return;
  }

  // If we reach this point, then unowned forms are not restricted. There
  // should a form we can trigger fill on (using the firstname field)
  ASSERT_FALSE(restrict_unowned_fields_);
  ASSERT_EQ(1U, GetAutofillManager()->NumFormsDetected());
  TriggerFormFill("firstname");

  // Wait for the fill to happen.
  bool has_filled = false;
  ASSERT_TRUE(content::ExecuteScriptAndExtractBool(GetRenderViewHost(),
                                                   "hasFilled()", &has_filled));
  EXPECT_EQ(has_filled, !restrict_unowned_fields_);

  SubprocessMetricsProvider::MergeHistogramDeltasForTesting();

  // If only some form fields are tagged with autocomplete types, then the
  // number of input elements will not match the number of fields when autofill
  // triees to preview or fill.
  histogram.ExpectUniqueSample("Autofill.NumElementsMatchesNumFields",
                               !restrict_unowned_fields_, 2);

  ExpectFieldValue("firstname", "Milton");
  ExpectFieldValue("address", "4120 Freidrich Lane");
  ExpectFieldValue("state", "TX");
  ExpectFieldValue("city", "Austin");
  ExpectFieldValue("company", "Initech");
  ExpectFieldValue("email", "red.swingline@initech.com");
  ExpectFieldValue("phone", "15125551234");
}

// Test that we do not fill formless non-checkout forms when we enable the
// formless form restrictions. This test differes from the NoAutocomplete
// version of the the test in that at least one of the fields has an
// autocomplete attribute, so autofill will always be aware of the existence
// of the form.
IN_PROC_BROWSER_TEST_P(AutofillRestrictUnownedFieldsTest, SomeAutocomplete) {
  SCOPED_TRACE(base::StringPrintf("restrict_unowned_fields_ = %d",
                                  restrict_unowned_fields_));
  CreateTestProfile();

  base::HistogramTester histogram;

  GURL url = embedded_test_server()->GetURL(
      "/autofill/formless_some_autocomplete.html");
  ASSERT_NO_FATAL_FAILURE(ui_test_utils::NavigateToURL(browser(), url));

  ASSERT_EQ(1U, GetAutofillManager()->NumFormsDetected());
  TriggerFormFill("firstname");

  // Wait for the fill to happen.
  bool has_filled = false;
  ASSERT_TRUE(content::ExecuteScriptAndExtractBool(GetRenderViewHost(),
                                                   "hasFilled()", &has_filled));
  EXPECT_EQ(has_filled, !restrict_unowned_fields_);

  SubprocessMetricsProvider::MergeHistogramDeltasForTesting();

  // If only some form fields are tagged with autocomplete types, then the
  // number of input elements will not match the number of fields when autofill
  // triees to preview or fill.
  histogram.ExpectUniqueSample("Autofill.NumElementsMatchesNumFields",
                               !restrict_unowned_fields_, 2);

  // http://crbug.com/841784
  // Formless fields with autocomplete attributes don't work because the
  // extracted form and the form to be previewed/filled end up with a mismatched
  // number of fields and early abort.
  // This is fixed when !restrict_unowned_fields_
  if (restrict_unowned_fields_) {
    // We should only have samples saying that some elements were filtered.
    auto buckets =
        histogram.GetAllSamples("Autofill.UnownedFieldsWereFiltered");
    ASSERT_EQ(1u, buckets.size());
    EXPECT_EQ(1, buckets[0].min);  // The "true" bucket.

    ExpectFieldValue("firstname", "M");
    ExpectFieldValue("address", "");
    ExpectFieldValue("state", "--");
    ExpectFieldValue("city", "");
    ExpectFieldValue("company", "");
    ExpectFieldValue("email", "");
    ExpectFieldValue("phone", "");
  } else {
    ExpectFieldValue("firstname", "Milton");
    ExpectFieldValue("address", "4120 Freidrich Lane");
    ExpectFieldValue("state", "TX");
    ExpectFieldValue("city", "Austin");
    ExpectFieldValue("company", "Initech");
    ExpectFieldValue("email", "red.swingline@initech.com");
    ExpectFieldValue("phone", "15125551234");
  }
}

// Test that we do not fill formless non-checkout forms when we enable the
// formless form restrictions.
IN_PROC_BROWSER_TEST_P(AutofillRestrictUnownedFieldsTest, AllAutocomplete) {
  SCOPED_TRACE(base::StringPrintf("restrict_unowned_fields_ = %d",
                                  restrict_unowned_fields_));
  CreateTestProfile();

  base::HistogramTester histogram;

  GURL url = embedded_test_server()->GetURL(
      "/autofill/formless_all_autocomplete.html");
  ASSERT_NO_FATAL_FAILURE(ui_test_utils::NavigateToURL(browser(), url));

  ASSERT_EQ(1U, GetAutofillManager()->NumFormsDetected());
  TriggerFormFill("firstname");

  // Wait for the fill to happen.
  bool has_filled = false;
  ASSERT_TRUE(content::ExecuteScriptAndExtractBool(GetRenderViewHost(),
                                                   "hasFilled()", &has_filled));
  EXPECT_TRUE(has_filled);

  SubprocessMetricsProvider::MergeHistogramDeltasForTesting();

  // If all form fields are tagged with autocomplete types, we make them all
  // available to be filled.
  histogram.ExpectUniqueSample("Autofill.NumElementsMatchesNumFields", true, 2);

  if (restrict_unowned_fields_) {
    // We should only have samples saying that no elements were filtered.
    auto buckets =
        histogram.GetAllSamples("Autofill.UnownedFieldsWereFiltered");
    ASSERT_EQ(1u, buckets.size());
    EXPECT_EQ(0, buckets[0].min);  // The "false" bucket.
  }

  ExpectFieldValue("firstname", "Milton");
  ExpectFieldValue("address", "4120 Freidrich Lane");
  ExpectFieldValue("state", "TX");
  ExpectFieldValue("city", "Austin");
  ExpectFieldValue("company", "Initech");
  ExpectFieldValue("email", "red.swingline@initech.com");
  ExpectFieldValue("phone", "15125551234");
}

// An extension of the test fixture for tests with site isolation.
//
// Test params:
//  - bool popup_views_enabled: whether feature AutofillExpandedPopupViews
//        is enabled for testing.
class AutofillInteractiveIsolationTest
    : public AutofillInteractiveTestBase,
      public testing::WithParamInterface<bool> {
 protected:
  AutofillInteractiveIsolationTest()
      : AutofillInteractiveTestBase(GetParam()) {}
  ~AutofillInteractiveIsolationTest() override = default;

  void SendKeyToPopupAndWait(ui::DomKey key,
                             content::RenderWidgetHost* widget) {
    ui::KeyboardCode key_code = ui::NonPrintableDomKeyToKeyboardCode(key);
    ui::DomCode code = ui::UsLayoutKeyboardCodeToDomCode(key_code);
    AutofillInteractiveTestBase::SendKeyToPopupAndWait(key, code, key_code,
                                                       widget);
  }

  bool IsPopupShown() {
    return !!static_cast<ChromeAutofillClient*>(
                 ContentAutofillDriverFactory::FromWebContents(GetWebContents())
                     ->DriverForFrame(GetWebContents()->GetMainFrame())
                     ->autofill_manager()
                     ->client())
                 ->popup_controller_for_testing();
  }

 private:
  void SetUpCommandLine(base::CommandLine* command_line) override {
    AutofillInteractiveTestBase::SetUpCommandLine(command_line);
    // Append --site-per-process flag.
    content::IsolateAllSitesForTesting(command_line);
  }
};

IN_PROC_BROWSER_TEST_P(AutofillInteractiveIsolationTest, SimpleCrossSiteFill) {
  CreateTestProfile();

  // Main frame is on a.com, iframe is on b.com.
  GURL url = embedded_test_server()->GetURL(
      "a.com", "/autofill/cross_origin_iframe.html");
  ui_test_utils::NavigateToURL(browser(), url);
  GURL iframe_url = embedded_test_server()->GetURL(
      "b.com", "/autofill/autofill_test_form.html");
  EXPECT_TRUE(
      content::NavigateIframeToURL(GetWebContents(), "crossFrame", iframe_url));

  // Let |test_delegate()| also observe autofill events in the iframe.
  content::RenderFrameHost* cross_frame =
      RenderFrameHostForName(GetWebContents(), "crossFrame");
  ASSERT_TRUE(cross_frame);
  ContentAutofillDriver* cross_driver =
      ContentAutofillDriverFactory::FromWebContents(GetWebContents())
          ->DriverForFrame(cross_frame);
  ASSERT_TRUE(cross_driver);
  cross_driver->autofill_manager()->SetTestDelegate(test_delegate());

  // Focus the form in the iframe and simulate choosing a suggestion via
  // keyboard.
  std::string script_focus("document.getElementById('NAME_FIRST').focus();");
  ASSERT_TRUE(content::ExecuteScript(cross_frame, script_focus));
  SendKeyToPageAndWait(ui::DomKey::ARROW_DOWN);
  content::RenderWidgetHost* widget =
      cross_frame->GetView()->GetRenderWidgetHost();
  SendKeyToPopupAndWait(ui::DomKey::ARROW_DOWN, widget);
  SendKeyToPopupAndWait(ui::DomKey::ENTER, widget);

  // Check that the suggestion was filled.
  std::string value;
  ASSERT_TRUE(content::ExecuteScriptAndExtractString(
      cross_frame,
      "window.domAutomationController.send("
      "    document.getElementById('NAME_FIRST').value);",
      &value));
  EXPECT_EQ("Milton", value);
}

// This test verifies that credit card (payment card list) popup works when the
// form is inside an OOPIF.
// Flaky on Windows http://crbug.com/728488
#if defined(OS_WIN)
#define MAYBE_CrossSitePaymentForms DISABLED_CrossSitePaymentForms
#else
#define MAYBE_CrossSitePaymentForms CrossSitePaymentForms
#endif
IN_PROC_BROWSER_TEST_P(AutofillInteractiveTest, MAYBE_CrossSitePaymentForms) {
  CreditCard card;
  test::SetCreditCardInfo(&card, "Milton Waddams", "4111111111111111", "09",
                          "2999", "");
  AddTestCreditCard(browser(), card);

  // Main frame is on a.com, iframe is on b.com.
  GURL url = embedded_test_server()->GetURL(
      "a.com", "/autofill/cross_origin_iframe.html");
  ui_test_utils::NavigateToURL(browser(), url);
  GURL iframe_url = embedded_test_server()->GetURL(
      "b.com", "/autofill/autofill_creditcard_form.html");
  EXPECT_TRUE(
      content::NavigateIframeToURL(GetWebContents(), "crossFrame", iframe_url));

  // Let |test_delegate()| also observe autofill events in the iframe.
  content::RenderFrameHost* cross_frame =
      RenderFrameHostForName(GetWebContents(), "crossFrame");
  ASSERT_TRUE(cross_frame);
  ContentAutofillDriver* cross_driver =
      ContentAutofillDriverFactory::FromWebContents(GetWebContents())
          ->DriverForFrame(cross_frame);
  ASSERT_TRUE(cross_driver);
  cross_driver->autofill_manager()->SetTestDelegate(test_delegate());

  // Focus the form in the iframe and simulate choosing a suggestion via
  // keyboard.
  std::string script_focus(
      "window.focus();"
      "document.getElementById('CREDIT_CARD_NUMBER').focus();");
  ASSERT_TRUE(content::ExecuteScript(cross_frame, script_focus));

  // Send an arrow dow keypress in order to trigger the autofill popup.
  SendKeyToPageAndWait(ui::DomKey::ARROW_DOWN);
}

IN_PROC_BROWSER_TEST_P(AutofillInteractiveIsolationTest,
                       DeletingFrameUnderSuggestion) {
  CreateTestProfile();

  // Main frame is on a.com, iframe is on b.com.
  GURL url = embedded_test_server()->GetURL(
      "a.com", "/autofill/cross_origin_iframe.html");
  ui_test_utils::NavigateToURL(browser(), url);
  GURL iframe_url = embedded_test_server()->GetURL(
      "b.com", "/autofill/autofill_test_form.html");
  EXPECT_TRUE(
      content::NavigateIframeToURL(GetWebContents(), "crossFrame", iframe_url));

  // Let |test_delegate()| also observe autofill events in the iframe.
  content::RenderFrameHost* cross_frame =
      RenderFrameHostForName(GetWebContents(), "crossFrame");
  ASSERT_TRUE(cross_frame);
  ContentAutofillDriver* cross_driver =
      ContentAutofillDriverFactory::FromWebContents(GetWebContents())
          ->DriverForFrame(cross_frame);
  ASSERT_TRUE(cross_driver);
  cross_driver->autofill_manager()->SetTestDelegate(test_delegate());

  // Focus the form in the iframe and simulate choosing a suggestion via
  // keyboard.
  std::string script_focus("document.getElementById('NAME_FIRST').focus();");
  ASSERT_TRUE(content::ExecuteScript(cross_frame, script_focus));
  SendKeyToPageAndWait(ui::DomKey::ARROW_DOWN);
  content::RenderWidgetHost* widget =
      cross_frame->GetView()->GetRenderWidgetHost();
  SendKeyToPopupAndWait(ui::DomKey::ARROW_DOWN, widget);
  // Do not accept the suggestion yet, to keep the pop-up shown.
  EXPECT_TRUE(IsPopupShown());

  // Delete the iframe.
  std::string script_delete =
      "document.body.removeChild(document.getElementById('crossFrame'));";
  ASSERT_TRUE(content::ExecuteScript(GetRenderViewHost(), script_delete));

  // The popup should have disappeared with the iframe.
  EXPECT_FALSE(IsPopupShown());
}

// Test params:
//  - bool popup_views_enabled: whether feature AutofillExpandedPopupViews
//        is enabled for testing.
class DynamicFormInteractiveTest : public AutofillInteractiveTestBase,
                                   public testing::WithParamInterface<bool> {
 protected:
  DynamicFormInteractiveTest()
      : AutofillInteractiveTestBase(GetParam()),
        https_server_(net::EmbeddedTestServer::TYPE_HTTPS) {
    // Setup that the test expects a re-fill to happen.
    test_delegate()->SetIsExpectingDynamicRefill(true);
  }
  ~DynamicFormInteractiveTest() override = default;

  // AutofillInteractiveTestBase:
  void SetUp() override {
    // Explicitly enable the filling of dynamic forms and disabled the
    // requirement for a secure context to fill credit cards.
    scoped_feature_list_.InitWithFeatures(
        {features::kAutofillDynamicForms},
        {features::kAutofillRequireSecureCreditCardContext,
         features::kAutofillRestrictUnownedFieldsToFormlessCheckout});
    https_server_.SetSSLConfig(net::EmbeddedTestServer::CERT_OK);
    https_server_.ServeFilesFromSourceDirectory("chrome/test/data");
    ASSERT_TRUE(https_server_.InitializeAndListen());
    https_server_.StartAcceptingConnections();
    AutofillInteractiveTestBase::SetUp();
  }

  void SetUpCommandLine(base::CommandLine* command_line) override {
    AutofillInteractiveTestBase::SetUpCommandLine(command_line);
    // HTTPS server only serves a valid cert for localhost, so this is needed to
    // load pages from "a.com" without an interstitial.
    command_line->AppendSwitch(switches::kIgnoreCertificateErrors);
  }

  net::EmbeddedTestServer* https_server() { return &https_server_; }

 private:
  base::test::ScopedFeatureList scoped_feature_list_;
  net::EmbeddedTestServer https_server_;

  DISALLOW_COPY_AND_ASSIGN(DynamicFormInteractiveTest);
};

// Test that we can Autofill dynamically generated forms.
IN_PROC_BROWSER_TEST_P(DynamicFormInteractiveTest, DynamicChangingFormFill) {
  CreateTestProfile();

  GURL url =
      embedded_test_server()->GetURL("a.com", "/autofill/dynamic_form.html");
  ASSERT_NO_FATAL_FAILURE(ui_test_utils::NavigateToURL(browser(), url));

  TriggerFormFill("firstname");

  // Wait for the re-fill to happen.
  bool has_refilled = false;
  ASSERT_TRUE(content::ExecuteScriptAndExtractBool(
      GetRenderViewHost(), "hasRefilled()", &has_refilled));
  ASSERT_TRUE(has_refilled);

  // Make sure the new form was filled correctly.
  ExpectFieldValue("firstname_form1", "Milton");
  ExpectFieldValue("address_form1", "4120 Freidrich Lane");
  ExpectFieldValue("state_form1", "TX");
  ExpectFieldValue("city_form1", "Austin");
  ExpectFieldValue("company_form1", "Initech");
  ExpectFieldValue("email_form1", "red.swingline@initech.com");
  ExpectFieldValue("phone_form1", "15125551234");
}

IN_PROC_BROWSER_TEST_P(DynamicFormInteractiveTest,
                       TwoDynamicChangingFormsFill) {
  // Setup that the test expects a re-fill to happen.
  test_delegate()->SetIsExpectingDynamicRefill(true);

  CreateTestProfile();

  GURL url = embedded_test_server()->GetURL("a.com",
                                            "/autofill/two_dynamic_forms.html");
  ASSERT_NO_FATAL_FAILURE(ui_test_utils::NavigateToURL(browser(), url));

  TriggerFormFill("firstname_form1");

  // Wait for the re-fill to happen.
  bool has_refilled = false;
  ASSERT_TRUE(content::ExecuteScriptAndExtractBool(
      GetRenderViewHost(), "hasRefilled('firstname_form1')", &has_refilled));
  ASSERT_TRUE(has_refilled);

  // Make sure the new form was filled correctly.
  ExpectFieldValue("firstname_form1", "Milton");
  ExpectFieldValue("address_form1", "4120 Freidrich Lane");
  ExpectFieldValue("state_form1", "TX");
  ExpectFieldValue("city_form1", "Austin");
  ExpectFieldValue("company_form1", "Initech");
  ExpectFieldValue("email_form1", "red.swingline@initech.com");
  ExpectFieldValue("phone_form1", "15125551234");

  TriggerFormFill("firstname_form2");

  // Wait for the re-fill to happen.
  has_refilled = false;
  ASSERT_TRUE(content::ExecuteScriptAndExtractBool(
      GetRenderViewHost(), "hasRefilled('firstname_form2')", &has_refilled));
  ASSERT_TRUE(has_refilled);

  // Make sure the new form was filled correctly.
  ExpectFieldValue("firstname_form2", "Milton");
  ExpectFieldValue("address_form2", "4120 Freidrich Lane");
  ExpectFieldValue("state_form2", "TX");
  ExpectFieldValue("city_form2", "Austin");
  ExpectFieldValue("company_form2", "Initech");
  ExpectFieldValue("email_form2", "red.swingline@initech.com");
  ExpectFieldValue("phone_form2", "15125551234");
}

// Test that forms that dynamically change a second time do not get filled.
IN_PROC_BROWSER_TEST_P(DynamicFormInteractiveTest,
                       DynamicChangingFormFill_SecondChange) {
  CreateTestProfile();

  GURL url = embedded_test_server()->GetURL(
      "a.com", "/autofill/double_dynamic_form.html");
  ASSERT_NO_FATAL_FAILURE(ui_test_utils::NavigateToURL(browser(), url));

  TriggerFormFill("firstname");

  // Wait for two dynamic changes to happen.
  bool has_refilled = false;
  ASSERT_TRUE(content::ExecuteScriptAndExtractBool(
      GetRenderViewHost(), "hasRefilled()", &has_refilled));
  ASSERT_FALSE(has_refilled);

  // Make sure the new form was not filled.
  ExpectFieldValue("firstname_form2", "");
  ExpectFieldValue("address_form2", "");
  ExpectFieldValue("state_form2", "CA");  // Default value.
  ExpectFieldValue("city_form2", "");
  ExpectFieldValue("company_form2", "");
  ExpectFieldValue("email_form2", "");
  ExpectFieldValue("phone_form2", "");
}

// Test that forms that dynamically change after a second do not get filled.
IN_PROC_BROWSER_TEST_P(DynamicFormInteractiveTest,
                       DynamicChangingFormFill_AfterDelay) {
  CreateTestProfile();

  GURL url = embedded_test_server()->GetURL(
      "a.com", "/autofill/dynamic_form_after_delay.html");
  ASSERT_NO_FATAL_FAILURE(ui_test_utils::NavigateToURL(browser(), url));

  TriggerFormFill("firstname");

  // Wait for the dynamic change to happen.
  bool has_refilled = false;
  ASSERT_TRUE(content::ExecuteScriptAndExtractBool(
      GetRenderViewHost(), "hasRefilled()", &has_refilled));
  ASSERT_FALSE(has_refilled);

  // Make sure that the new form was not filled.
  ExpectFieldValue("firstname_form1", "");
  ExpectFieldValue("address_form1", "");
  ExpectFieldValue("state_form1", "CA");  // Default value.
  ExpectFieldValue("city_form1", "");
  ExpectFieldValue("company_form1", "");
  ExpectFieldValue("email_form1", "");
  ExpectFieldValue("phone_form1", "");
}

// Test that only field of a type group that was filled initially get refilled.
IN_PROC_BROWSER_TEST_P(DynamicFormInteractiveTest,
                       DynamicChangingFormFill_AddsNewFieldTypeGroups) {
  CreateTestProfile();

  GURL url = embedded_test_server()->GetURL(
      "a.com", "/autofill/dynamic_form_new_field_types.html");
  ASSERT_NO_FATAL_FAILURE(ui_test_utils::NavigateToURL(browser(), url));

  TriggerFormFill("firstname");

  // Wait for the dynamic change to happen.
  bool has_refilled = false;
  ASSERT_TRUE(content::ExecuteScriptAndExtractBool(
      GetRenderViewHost(), "hasRefilled()", &has_refilled));
  ASSERT_TRUE(has_refilled);

  // The fields present in the initial fill should be filled.
  ExpectFieldValue("firstname_form1", "Milton");
  ExpectFieldValue("address_form1", "4120 Freidrich Lane");
  ExpectFieldValue("state_form1", "TX");
  ExpectFieldValue("city_form1", "Austin");
  // Fields from group that were not present in the initial fill should not be
  // filled
  ExpectFieldValue("company_form1", "");
  // Fields that were present but hidden in the initial fill should not be
  // filled.
  ExpectFieldValue("email_form1", "");
  // The phone should be filled even if it's a different format than the initial
  // fill.
  ExpectFieldValue("phone_form1", "5125551234");
}

// Test that credit card fields are never re-filled.
IN_PROC_BROWSER_TEST_P(DynamicFormInteractiveTest,
                       DynamicChangingFormFill_NotForCreditCard) {
  // Add a credit card.
  CreditCard card;
  test::SetCreditCardInfo(&card, "Milton Waddams", "4111111111111111", "09",
                          "2999", "");
  AddTestCreditCard(browser(), card);

  // Navigate to the page.
  GURL url = https_server()->GetURL("a.com",
                                    "/autofill/dynamic_form_credit_card.html");
  ASSERT_NO_FATAL_FAILURE(ui_test_utils::NavigateToURL(browser(), url));

  // Trigger the initial fill.
  FocusFieldByName("cc-name");
  SendKeyToPageAndWait(ui::DomKey::FromCharacter('M'), ui::DomCode::US_M,
                       ui::VKEY_M);
  SendKeyToPopupAndWait(ui::DomKey::ARROW_DOWN);
  SendKeyToPopupAndWait(ui::DomKey::ENTER);

  // Wait for the dynamic change to happen.
  bool has_refilled = false;
  ASSERT_TRUE(content::ExecuteScriptAndExtractBool(
      GetRenderViewHost(), "hasRefilled()", &has_refilled));
  ASSERT_FALSE(has_refilled);

  // There should be no values in the fields.
  ExpectFieldValue("cc-name", "");
  ExpectFieldValue("cc-num", "");
  ExpectFieldValue("cc-exp-month", "01");   // Default value.
  ExpectFieldValue("cc-exp-year", "2010");  // Default value.
  ExpectFieldValue("cc-csc", "");
}

// Test that we can Autofill dynamically changing selects that have options
// added and removed.
IN_PROC_BROWSER_TEST_P(DynamicFormInteractiveTest,
                       DynamicChangingFormFill_SelectUpdated) {
  CreateTestProfile();

  GURL url = embedded_test_server()->GetURL(
      "a.com", "/autofill/dynamic_form_select_options_change.html");
  ASSERT_NO_FATAL_FAILURE(ui_test_utils::NavigateToURL(browser(), url));

  TriggerFormFill("firstname");

  // Wait for the re-fill to happen.
  bool has_refilled = false;
  ASSERT_TRUE(content::ExecuteScriptAndExtractBool(
      GetRenderViewHost(), "hasRefilled()", &has_refilled));
  ASSERT_TRUE(has_refilled);

  // Make sure the new form was filled correctly.
  ExpectFieldValue("firstname", "Milton");
  ExpectFieldValue("address1", "4120 Freidrich Lane");
  ExpectFieldValue("state", "TX");
  ExpectFieldValue("city", "Austin");
  ExpectFieldValue("company", "Initech");
  ExpectFieldValue("email", "red.swingline@initech.com");
  ExpectFieldValue("phone", "15125551234");
}

// Test that we can Autofill dynamically changing selects that have options
// added and removed only once.
IN_PROC_BROWSER_TEST_P(DynamicFormInteractiveTest,
                       DynamicChangingFormFill_DoubleSelectUpdated) {
  CreateTestProfile();

  GURL url = embedded_test_server()->GetURL(
      "a.com", "/autofill/dynamic_form_double_select_options_change.html");
  ASSERT_NO_FATAL_FAILURE(ui_test_utils::NavigateToURL(browser(), url));

  TriggerFormFill("firstname");

  // Wait for the re-fill to happen.
  bool has_refilled = false;
  ASSERT_TRUE(content::ExecuteScriptAndExtractBool(
      GetRenderViewHost(), "hasRefilled()", &has_refilled));
  ASSERT_FALSE(has_refilled);

  // The fields that were initially filled and not reset should still be filled.
  ExpectFieldValue("firstname", "");  // That field value was reset dynamically.
  ExpectFieldValue("address1", "4120 Freidrich Lane");
  ExpectFieldValue("state", "CA");   // Default value.
  ExpectFieldValue("city", "Austin");
  ExpectFieldValue("company", "Initech");
  ExpectFieldValue("email", "red.swingline@initech.com");
  ExpectFieldValue("phone", "15125551234");
}

// Test that we can Autofill dynamically generated forms with no name if the
// NameForAutofill of the first field matches.
IN_PROC_BROWSER_TEST_P(DynamicFormInteractiveTest,
                       DynamicChangingFormFill_FormWithoutName) {
  CreateTestProfile();

  GURL url = embedded_test_server()->GetURL(
      "a.com", "/autofill/dynamic_form_no_name.html");
  ASSERT_NO_FATAL_FAILURE(ui_test_utils::NavigateToURL(browser(), url));

  TriggerFormFill("firstname");

  // Wait for the re-fill to happen.
  bool has_refilled = false;
  ASSERT_TRUE(content::ExecuteScriptAndExtractBool(
      GetRenderViewHost(), "hasRefilled()", &has_refilled));
  ASSERT_TRUE(has_refilled);

  // Make sure the new form was filled correctly.
  ExpectFieldValue("firstname_form1", "Milton");
  ExpectFieldValue("address_form1", "4120 Freidrich Lane");
  ExpectFieldValue("state_form1", "TX");
  ExpectFieldValue("city_form1", "Austin");
  ExpectFieldValue("company_form1", "Initech");
  ExpectFieldValue("email_form1", "red.swingline@initech.com");
  ExpectFieldValue("phone_form1", "15125551234");
}

// Test that we can Autofill dynamically changing selects that have options
// added and removed for forms with no names if the NameForAutofill of the first
// field matches.
IN_PROC_BROWSER_TEST_P(DynamicFormInteractiveTest,
                       DynamicChangingFormFill_SelectUpdated_FormWithoutName) {
  CreateTestProfile();

  GURL url = embedded_test_server()->GetURL(
      "a.com",
      "/autofill/dynamic_form_with_no_name_select_options_change.html");
  ASSERT_NO_FATAL_FAILURE(ui_test_utils::NavigateToURL(browser(), url));

  TriggerFormFill("firstname");

  // Wait for the re-fill to happen.
  bool has_refilled = false;
  ASSERT_TRUE(content::ExecuteScriptAndExtractBool(
      GetRenderViewHost(), "hasRefilled()", &has_refilled));
  ASSERT_TRUE(has_refilled);

  // Make sure the new form was filled correctly.
  ExpectFieldValue("firstname", "Milton");
  ExpectFieldValue("address1", "4120 Freidrich Lane");
  ExpectFieldValue("state", "TX");
  ExpectFieldValue("city", "Austin");
  ExpectFieldValue("company", "Initech");
  ExpectFieldValue("email", "red.swingline@initech.com");
  ExpectFieldValue("phone", "15125551234");
}

// Test that we can Autofill dynamically generated synthetic forms if the
// NameForAutofill of the first field matches.
IN_PROC_BROWSER_TEST_P(DynamicFormInteractiveTest,
                       DynamicChangingFormFill_SyntheticForm) {
  CreateTestProfile();

  GURL url = embedded_test_server()->GetURL(
      "a.com", "/autofill/dynamic_synthetic_form.html");
  ASSERT_NO_FATAL_FAILURE(ui_test_utils::NavigateToURL(browser(), url));

  TriggerFormFill("firstname");

  // Wait for the re-fill to happen.
  bool has_refilled = false;
  ASSERT_TRUE(content::ExecuteScriptAndExtractBool(
      GetRenderViewHost(), "hasRefilled()", &has_refilled));
  ASSERT_TRUE(has_refilled);

  // Make sure the new form was filled correctly.
  ExpectFieldValue("firstname_syntheticform1", "Milton");
  ExpectFieldValue("address_syntheticform1", "4120 Freidrich Lane");
  ExpectFieldValue("state_syntheticform1", "TX");
  ExpectFieldValue("city_syntheticform1", "Austin");
  ExpectFieldValue("company_syntheticform1", "Initech");
  ExpectFieldValue("email_syntheticform1", "red.swingline@initech.com");
  ExpectFieldValue("phone_syntheticform1", "15125551234");
}

// Test that we can Autofill dynamically synthetic forms when the select options
// change if the NameForAutofill of the first field matches
IN_PROC_BROWSER_TEST_P(DynamicFormInteractiveTest,
                       DynamicChangingFormFill_SelectUpdated_SyntheticForm) {
  CreateTestProfile();

  GURL url = embedded_test_server()->GetURL(
      "a.com", "/autofill/dynamic_synthetic_form_select_options_change.html");
  ASSERT_NO_FATAL_FAILURE(ui_test_utils::NavigateToURL(browser(), url));

  TriggerFormFill("firstname");

  // Wait for the re-fill to happen.
  bool has_refilled = false;
  ASSERT_TRUE(content::ExecuteScriptAndExtractBool(
      GetRenderViewHost(), "hasRefilled()", &has_refilled));
  ASSERT_TRUE(has_refilled);

  // Make sure the new form was filled correctly.
  ExpectFieldValue("firstname", "Milton");
  ExpectFieldValue("address1", "4120 Freidrich Lane");
  ExpectFieldValue("state", "TX");
  ExpectFieldValue("city", "Austin");
  ExpectFieldValue("company", "Initech");
  ExpectFieldValue("email", "red.swingline@initech.com");
  ExpectFieldValue("phone", "15125551234");
}

INSTANTIATE_TEST_CASE_P(All, AutofillInteractiveTest, testing::Bool());

INSTANTIATE_TEST_CASE_P(All,
                        AutofillSingleClickTest,
                        testing::Combine(testing::Bool(), testing::Bool()));

INSTANTIATE_TEST_CASE_P(All, AutofillInteractiveIsolationTest, testing::Bool());

INSTANTIATE_TEST_CASE_P(All, DynamicFormInteractiveTest, testing::Bool());

INSTANTIATE_TEST_CASE_P(All,
                        AutofillRestrictUnownedFieldsTest,
                        testing::Combine(testing::Bool(), testing::Bool()));
}  // namespace autofill
