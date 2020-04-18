// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/logging.h"
#include "base/strings/utf_string_conversions.h"
#include "base/test/scoped_command_line.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/javascript_dialogs/javascript_dialog_tab_helper.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/common/chrome_switches.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/web_contents.h"
#include "content/public/test/browser_test_utils.h"
#include "content/public/test/test_utils.h"

using JavaScriptDialogTest = InProcessBrowserTest;

IN_PROC_BROWSER_TEST_F(JavaScriptDialogTest, ReloadDoesntHang) {
  content::WebContents* tab =
      browser()->tab_strip_model()->GetActiveWebContents();
  JavaScriptDialogTabHelper* js_helper =
      JavaScriptDialogTabHelper::FromWebContents(tab);

  // Show a dialog.
  scoped_refptr<content::MessageLoopRunner> runner =
      new content::MessageLoopRunner;
  js_helper->SetDialogShownCallbackForTesting(runner->QuitClosure());
  tab->GetMainFrame()->ExecuteJavaScriptForTests(base::UTF8ToUTF16("alert()"));
  runner->Run();

  // Try reloading.
  tab->GetController().Reload(content::ReloadType::NORMAL, false);
  content::WaitForLoadStop(tab);

  // If the WaitForLoadStop doesn't hang forever, we've passed.
}

IN_PROC_BROWSER_TEST_F(JavaScriptDialogTest,
                       ClosingPageSharingRendererDoesntHang) {
  // Turn off popup blocking.
  base::test::ScopedCommandLine scoped_command_line;
  scoped_command_line.GetProcessCommandLine()->AppendSwitch(
      switches::kDisablePopupBlocking);

  // Two tabs, one render process.
  content::WebContents* tab1 =
      browser()->tab_strip_model()->GetActiveWebContents();
  content::WebContentsAddedObserver new_wc_observer;
  tab1->GetMainFrame()->ExecuteJavaScriptForTests(
      base::UTF8ToUTF16("window.open('about:blank');"));
  content::WebContents* tab2 = new_wc_observer.GetWebContents();
  ASSERT_NE(tab1, tab2);
  ASSERT_EQ(tab1->GetMainFrame()->GetProcess(),
            tab2->GetMainFrame()->GetProcess());

  // Tab two shows a dialog.
  scoped_refptr<content::MessageLoopRunner> runner =
      new content::MessageLoopRunner;
  JavaScriptDialogTabHelper* js_helper2 =
      JavaScriptDialogTabHelper::FromWebContents(tab2);
  js_helper2->SetDialogShownCallbackForTesting(runner->QuitClosure());
  tab2->GetMainFrame()->ExecuteJavaScriptForTests(base::UTF8ToUTF16("alert()"));
  runner->Run();

  // Tab two is closed while the dialog is up.
  int tab2_index = browser()->tab_strip_model()->GetIndexOfWebContents(tab2);
  browser()->tab_strip_model()->CloseWebContentsAt(tab2_index,
                                                   TabStripModel::CLOSE_NONE);

  // Try reloading tab one.
  tab1->GetController().Reload(content::ReloadType::NORMAL, false);
  content::WaitForLoadStop(tab1);

  // If the WaitForLoadStop doesn't hang forever, we've passed.
}

IN_PROC_BROWSER_TEST_F(JavaScriptDialogTest,
                       ClosingPageWithSubframeAlertingDoesntCrash) {
  content::WebContents* tab =
      browser()->tab_strip_model()->GetActiveWebContents();
  JavaScriptDialogTabHelper* js_helper =
      JavaScriptDialogTabHelper::FromWebContents(tab);

  // A subframe shows a dialog.
  std::string dialog_url = "data:text/html,<script>alert(\"hi\");</script>";
  std::string script = "var iframe = document.createElement('iframe');"
                       "iframe.src = '" + dialog_url + "';"
                       "document.body.appendChild(iframe);";
  scoped_refptr<content::MessageLoopRunner> runner =
      new content::MessageLoopRunner;
  js_helper->SetDialogShownCallbackForTesting(runner->QuitClosure());
  tab->GetMainFrame()->ExecuteJavaScriptForTests(base::UTF8ToUTF16(script));
  runner->Run();

  // The tab is closed while the dialog is up.
  int tab_index = browser()->tab_strip_model()->GetIndexOfWebContents(tab);
  browser()->tab_strip_model()->CloseWebContentsAt(tab_index,
                                                   TabStripModel::CLOSE_NONE);

  // No crash is good news.
}

class JavaScriptCallbackHelper {
 public:
  JavaScriptDialogTabHelper::DialogClosedCallback GetCallback() {
    return base::BindOnce(&JavaScriptCallbackHelper::DialogClosed,
                          base::Unretained(this));
  }

  bool last_success() { return last_success_; }
  base::string16 last_input() { return last_input_; }

 private:
  void DialogClosed(bool success, const base::string16& user_input) {
    last_success_ = success;
    last_input_ = user_input;
  }

  bool last_success_;
  base::string16 last_input_;
};

// Tests to make sure HandleJavaScriptDialog works correctly.
IN_PROC_BROWSER_TEST_F(JavaScriptDialogTest, HandleJavaScriptDialog) {
  content::WebContents* tab =
      browser()->tab_strip_model()->GetActiveWebContents();
  content::RenderFrameHost* frame = tab->GetMainFrame();
  JavaScriptDialogTabHelper* js_helper =
      JavaScriptDialogTabHelper::FromWebContents(tab);

  JavaScriptCallbackHelper callback_helper;

  // alert
  bool did_suppress = false;
  js_helper->RunJavaScriptDialog(
      tab, frame, content::JAVASCRIPT_DIALOG_TYPE_ALERT, base::string16(),
      base::string16(), callback_helper.GetCallback(), &did_suppress);
  ASSERT_TRUE(js_helper->IsShowingDialogForTesting());
  js_helper->HandleJavaScriptDialog(tab, true, nullptr);
  ASSERT_FALSE(js_helper->IsShowingDialogForTesting());
  ASSERT_TRUE(callback_helper.last_success());
  ASSERT_EQ(base::string16(), callback_helper.last_input());

  // confirm
  for (auto response : {true, false}) {
    js_helper->RunJavaScriptDialog(
        tab, frame, content::JAVASCRIPT_DIALOG_TYPE_CONFIRM, base::string16(),
        base::string16(), callback_helper.GetCallback(), &did_suppress);
    ASSERT_TRUE(js_helper->IsShowingDialogForTesting());
    js_helper->HandleJavaScriptDialog(tab, response, nullptr);
    ASSERT_FALSE(js_helper->IsShowingDialogForTesting());
    ASSERT_EQ(response, callback_helper.last_success());
    ASSERT_EQ(base::string16(), callback_helper.last_input());
  }

  // prompt, cancel
  js_helper->RunJavaScriptDialog(tab, frame,
                                 content::JAVASCRIPT_DIALOG_TYPE_PROMPT,
                                 base::ASCIIToUTF16("Label"), base::string16(),
                                 callback_helper.GetCallback(), &did_suppress);
  ASSERT_TRUE(js_helper->IsShowingDialogForTesting());
  js_helper->HandleJavaScriptDialog(tab, false, nullptr);
  ASSERT_FALSE(js_helper->IsShowingDialogForTesting());
  ASSERT_FALSE(callback_helper.last_success());
  ASSERT_EQ(base::string16(), callback_helper.last_input());

  base::string16 value1 = base::ASCIIToUTF16("abc");
  base::string16 value2 = base::ASCIIToUTF16("123");

  // prompt, ok + override
  js_helper->RunJavaScriptDialog(tab, frame,
                                 content::JAVASCRIPT_DIALOG_TYPE_PROMPT,
                                 base::ASCIIToUTF16("Label"), value1,
                                 callback_helper.GetCallback(), &did_suppress);
  ASSERT_TRUE(js_helper->IsShowingDialogForTesting());
  js_helper->HandleJavaScriptDialog(tab, true, &value2);
  ASSERT_FALSE(js_helper->IsShowingDialogForTesting());
  ASSERT_TRUE(callback_helper.last_success());
  ASSERT_EQ(value2, callback_helper.last_input());

  // prompt, ok + no override
  js_helper->RunJavaScriptDialog(tab, frame,
                                 content::JAVASCRIPT_DIALOG_TYPE_PROMPT,
                                 base::ASCIIToUTF16("Label"), value1,
                                 callback_helper.GetCallback(), &did_suppress);
  ASSERT_TRUE(js_helper->IsShowingDialogForTesting());
  js_helper->HandleJavaScriptDialog(tab, true, nullptr);
  ASSERT_FALSE(js_helper->IsShowingDialogForTesting());
  ASSERT_TRUE(callback_helper.last_success());
  ASSERT_EQ(value1, callback_helper.last_input());
}
