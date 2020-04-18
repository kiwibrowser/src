// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <stddef.h>

#include <string>
#include <utility>

#include "base/command_line.h"
#include "base/memory/ref_counted.h"
#include "base/path_service.h"
#include "base/strings/stringprintf.h"
#include "chrome/browser/extensions/api/debugger/debugger_api.h"
#include "chrome/browser/extensions/extension_apitest.h"
#include "chrome/browser/extensions/extension_function_test_utils.h"
#include "chrome/browser/infobars/infobar_service.h"
#include "chrome/browser/sessions/session_tab_helper.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/common/chrome_paths.h"
#include "chrome/common/chrome_switches.h"
#include "chrome/test/base/ui_test_utils.h"
#include "components/infobars/core/infobar.h"
#include "components/infobars/core/infobar_delegate.h"
#include "extensions/browser/extension_function.h"
#include "extensions/common/extension.h"
#include "extensions/common/extension_builder.h"
#include "extensions/common/manifest_constants.h"
#include "extensions/common/switches.h"
#include "extensions/common/value_builder.h"

namespace extensions {

class DebuggerApiTest : public ExtensionApiTest {
 protected:
  ~DebuggerApiTest() override {}

  void SetUpCommandLine(base::CommandLine* command_line) override;
  void SetUpOnMainThread() override;

  // Run the attach function. If |expected_error| is not empty, then the
  // function should fail with the error. Otherwise, the function is expected
  // to succeed.
  testing::AssertionResult RunAttachFunction(const GURL& url,
                                             const std::string& expected_error);

  const Extension* extension() const { return extension_.get(); }
  base::CommandLine* command_line() const { return command_line_; }

 private:
  testing::AssertionResult RunAttachFunctionOnTarget(
      const std::string& debuggee_target, const std::string& expected_error);

  // The command-line for the test process, preserved in order to modify
  // mid-test.
  base::CommandLine* command_line_;

  // A basic extension with the debugger permission.
  scoped_refptr<const Extension> extension_;
};

void DebuggerApiTest::SetUpCommandLine(base::CommandLine* command_line) {
  ExtensionApiTest::SetUpCommandLine(command_line);
  // We need to hold onto |command_line| in order to modify it during the test.
  command_line_ = command_line;
}

void DebuggerApiTest::SetUpOnMainThread() {
  ExtensionApiTest::SetUpOnMainThread();
  extension_ =
      ExtensionBuilder()
          .SetManifest(
              DictionaryBuilder()
                  .Set("name", "debugger")
                  .Set("version", "0.1")
                  .Set("manifest_version", 2)
                  .Set("permissions", ListBuilder().Append("debugger").Build())
                  .Build())
          .Build();
}

testing::AssertionResult DebuggerApiTest::RunAttachFunction(
    const GURL& url, const std::string& expected_error) {
  ui_test_utils::NavigateToURL(browser(), url);
  content::WebContents* web_contents =
      browser()->tab_strip_model()->GetActiveWebContents();

  // Attach by tabId.
  int tab_id = SessionTabHelper::IdForTab(web_contents).id();
  std::string debugee_by_tab = base::StringPrintf("{\"tabId\": %d}", tab_id);
  testing::AssertionResult result =
      RunAttachFunctionOnTarget(debugee_by_tab, expected_error);
  if (!result)
    return result;

  // Attach by targetId.
  scoped_refptr<DebuggerGetTargetsFunction> get_targets_function =
      new DebuggerGetTargetsFunction();
  std::unique_ptr<base::Value> value(
      extension_function_test_utils::RunFunctionAndReturnSingleResult(
          get_targets_function.get(), "[]", browser()));
  base::ListValue* targets = nullptr;
  EXPECT_TRUE(value->GetAsList(&targets));

  std::string debugger_target_id;
  for (size_t i = 0; i < targets->GetSize(); ++i) {
    base::DictionaryValue* target_dict = nullptr;
    EXPECT_TRUE(targets->GetDictionary(i, &target_dict));
    int id = -1;
    if (target_dict->GetInteger("tabId", &id) && id == tab_id) {
      EXPECT_TRUE(target_dict->GetString("id", &debugger_target_id));
      break;
    }
  }
  EXPECT_TRUE(!debugger_target_id.empty());

  std::string debugee_by_target_id =
      base::StringPrintf("{\"targetId\": \"%s\"}", debugger_target_id.c_str());
  return RunAttachFunctionOnTarget(debugee_by_target_id, expected_error);
}

testing::AssertionResult DebuggerApiTest::RunAttachFunctionOnTarget(
    const std::string& debuggee_target, const std::string& expected_error) {
  scoped_refptr<DebuggerAttachFunction> attach_function =
      new DebuggerAttachFunction();
  attach_function->set_extension(extension_.get());

  std::string actual_error;
  if (!extension_function_test_utils::RunFunction(
          attach_function.get(),
          base::StringPrintf("[%s, \"1.1\"]", debuggee_target.c_str()),
          browser(), api_test_utils::NONE)) {
    actual_error = attach_function->GetError();
  } else {
    // Clean up and detach.
    scoped_refptr<DebuggerDetachFunction> detach_function =
        new DebuggerDetachFunction();
    detach_function->set_extension(extension_.get());
    if (!extension_function_test_utils::RunFunction(
            detach_function.get(),
            base::StringPrintf("[%s]", debuggee_target.c_str()), browser(),
            api_test_utils::NONE)) {
      return testing::AssertionFailure() << "Could not detach from "
          << debuggee_target << " : " << detach_function->GetError();
    }
  }

  if (expected_error.empty() && !actual_error.empty()) {
    return testing::AssertionFailure() << "Could not attach to "
        << debuggee_target << " : " << actual_error;
  } else if (actual_error != expected_error) {
    return testing::AssertionFailure() << "Did not get correct error upon "
        << "attach to " << debuggee_target << " : "
        << "expected: " << expected_error << ", found: " << actual_error;
  }
  return testing::AssertionSuccess();
}

IN_PROC_BROWSER_TEST_F(ExtensionApiTest, Debugger) {
  ASSERT_TRUE(RunExtensionTest("debugger")) << message_;
}

IN_PROC_BROWSER_TEST_F(DebuggerApiTest,
                       DebuggerNotAllowedOnOtherExtensionPages) {
  // Load another arbitrary extension with an associated resource (popup.html).
  base::FilePath path;
  ASSERT_TRUE(base::PathService::Get(chrome::DIR_TEST_DATA, &path));
  path = path.AppendASCII("extensions").AppendASCII("simple_with_popup");
  const Extension* another_extension = LoadExtension(path);
  ASSERT_TRUE(another_extension);

  GURL other_ext_url =
      GURL(base::StringPrintf("chrome-extension://%s/popup.html",
                              another_extension->id().c_str()));

  // This extension should not be able to access another extension.
  EXPECT_TRUE(RunAttachFunction(
      other_ext_url, manifest_errors::kCannotAccessExtensionUrl));

  // This extension *should* be able to debug itself.
  EXPECT_TRUE(RunAttachFunction(
                  GURL(base::StringPrintf("chrome-extension://%s/foo.html",
                                          extension()->id().c_str())),
                  std::string()));

  // Append extensions on chrome urls switch. The extension should now be able
  // to debug any extension.
  command_line()->AppendSwitch(switches::kExtensionsOnChromeURLs);
  EXPECT_TRUE(RunAttachFunction(other_ext_url, std::string()));
}

IN_PROC_BROWSER_TEST_F(DebuggerApiTest, InfoBar) {
  int tab_id = SessionTabHelper::IdForTab(
                   browser()->tab_strip_model()->GetActiveWebContents())
                   .id();
  scoped_refptr<DebuggerAttachFunction> attach_function;
  scoped_refptr<DebuggerDetachFunction> detach_function;

  Browser* another_browser =
      new Browser(Browser::CreateParams(profile(), true));
  AddBlankTabAndShow(another_browser);
  AddBlankTabAndShow(another_browser);
  int tab_id2 = SessionTabHelper::IdForTab(
                    another_browser->tab_strip_model()->GetActiveWebContents())
                    .id();

  InfoBarService* service1 = InfoBarService::FromWebContents(
      browser()->tab_strip_model()->GetActiveWebContents());
  InfoBarService* service2 = InfoBarService::FromWebContents(
      another_browser->tab_strip_model()->GetWebContentsAt(0));
  InfoBarService* service3 = InfoBarService::FromWebContents(
      another_browser->tab_strip_model()->GetWebContentsAt(1));

  // Attach should create infobars in both browsers.
  attach_function = new DebuggerAttachFunction();
  attach_function->set_extension(extension());
  ASSERT_TRUE(extension_function_test_utils::RunFunction(
      attach_function.get(),
      base::StringPrintf("[{\"tabId\": %d}, \"1.1\"]", tab_id), browser(),
      api_test_utils::NONE));
  EXPECT_EQ(1u, service1->infobar_count());
  EXPECT_EQ(1u, service2->infobar_count());
  EXPECT_EQ(1u, service3->infobar_count());

  // Second attach should not create infobars.
  attach_function = new DebuggerAttachFunction();
  attach_function->set_extension(extension());
  ASSERT_TRUE(extension_function_test_utils::RunFunction(
      attach_function.get(),
      base::StringPrintf("[{\"tabId\": %d}, \"1.1\"]", tab_id2), browser(),
      api_test_utils::NONE));
  EXPECT_EQ(1u, service1->infobar_count());
  EXPECT_EQ(1u, service2->infobar_count());
  EXPECT_EQ(1u, service3->infobar_count());

  // Detach from one of the tabs should not remove infobars.
  detach_function = new DebuggerDetachFunction();
  detach_function->set_extension(extension());
  ASSERT_TRUE(extension_function_test_utils::RunFunction(
      detach_function.get(), base::StringPrintf("[{\"tabId\": %d}]", tab_id2),
      browser(), api_test_utils::NONE));
  EXPECT_EQ(1u, service1->infobar_count());
  EXPECT_EQ(1u, service2->infobar_count());
  EXPECT_EQ(1u, service3->infobar_count());

  // Detach should remove all infobars.
  detach_function = new DebuggerDetachFunction();
  detach_function->set_extension(extension());
  ASSERT_TRUE(extension_function_test_utils::RunFunction(
      detach_function.get(), base::StringPrintf("[{\"tabId\": %d}]", tab_id),
      browser(), api_test_utils::NONE));
  EXPECT_EQ(0u, service1->infobar_count());
  EXPECT_EQ(0u, service2->infobar_count());
  EXPECT_EQ(0u, service3->infobar_count());

  // Attach again.
  attach_function = new DebuggerAttachFunction();
  attach_function->set_extension(extension());
  ASSERT_TRUE(extension_function_test_utils::RunFunction(
      attach_function.get(),
      base::StringPrintf("[{\"tabId\": %d}, \"1.1\"]", tab_id), browser(),
      api_test_utils::NONE));
  EXPECT_EQ(1u, service1->infobar_count());
  EXPECT_EQ(1u, service2->infobar_count());
  EXPECT_EQ(1u, service3->infobar_count());

  // Calling delegate()->InfoBarDismissed() on a global infobar should
  // cause detach and removal of all infobars, except the one used to
  // fetch the delegate (i.e., service2->infobar_at(0) itself).
  // Afterwards, service2->infobar_at(0) must be explicitly removed.
  // See InfoBarView::ButtonPressed for an example.
  service2->infobar_at(0)->delegate()->InfoBarDismissed();
  service2->infobar_at(0)->RemoveSelf();
  EXPECT_EQ(0u, service1->infobar_count());
  EXPECT_EQ(0u, service2->infobar_count());
  EXPECT_EQ(0u, service3->infobar_count());
  detach_function = new DebuggerDetachFunction();
  detach_function->set_extension(extension());
  // Cannot detach again.
  ASSERT_FALSE(extension_function_test_utils::RunFunction(
      detach_function.get(), base::StringPrintf("[{\"tabId\": %d}]", tab_id),
      browser(), api_test_utils::NONE));

  // And again...
  attach_function = new DebuggerAttachFunction();
  attach_function->set_extension(extension());
  ASSERT_TRUE(extension_function_test_utils::RunFunction(
      attach_function.get(),
      base::StringPrintf("[{\"tabId\": %d}, \"1.1\"]", tab_id), browser(),
      api_test_utils::NONE));
  EXPECT_EQ(1u, service1->infobar_count());
  EXPECT_EQ(1u, service2->infobar_count());
  EXPECT_EQ(1u, service3->infobar_count());

  // Closing tab should not affect anything.
  ASSERT_TRUE(another_browser->tab_strip_model()->CloseWebContentsAt(1, 0));
  service3 = nullptr;
  EXPECT_EQ(1u, service1->infobar_count());
  EXPECT_EQ(1u, service2->infobar_count());

  // Closing browser should not affect anything.
  CloseBrowserSynchronously(another_browser);
  service2 = nullptr;
  another_browser = nullptr;
  EXPECT_EQ(1u, service1->infobar_count());

  // Detach should remove the remaining infobar.
  detach_function = new DebuggerDetachFunction();
  detach_function->set_extension(extension());
  ASSERT_TRUE(extension_function_test_utils::RunFunction(
      detach_function.get(), base::StringPrintf("[{\"tabId\": %d}]", tab_id),
      browser(), api_test_utils::NONE));
  EXPECT_EQ(0u, service1->infobar_count());
}

}  // namespace extensions
