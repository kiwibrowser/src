// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/bind.h"
#include "base/bind_helpers.h"
#include "base/macros.h"
#include "base/strings/stringprintf.h"
#include "base/strings/utf_string_conversions.h"
#include "build/build_config.h"
#include "chrome/browser/bookmarks/bookmark_model_factory.h"
#include "chrome/browser/extensions/chrome_test_extension_loader.h"
#include "chrome/browser/extensions/extension_action.h"
#include "chrome/browser/extensions/extension_action_manager.h"
#include "chrome/browser/extensions/extension_action_test_util.h"
#include "chrome/browser/extensions/extension_apitest.h"
#include "chrome/browser/extensions/extension_service.h"
#include "chrome/browser/extensions/extension_tab_util.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_window.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "components/bookmarks/browser/bookmark_model.h"
#include "content/public/test/browser_test_utils.h"
#include "extensions/browser/extension_registry.h"
#include "extensions/browser/extension_system.h"
#include "extensions/browser/test_extension_registry_observer.h"
#include "extensions/test/extension_test_message_listener.h"
#include "extensions/test/test_extension_dir.h"
#include "testing/gmock/include/gmock/gmock.h"

namespace extensions {
namespace {

const char kDeclarativeContentManifest[] =
    "{\n"
    "  \"name\": \"Declarative Content apitest\",\n"
    "  \"version\": \"0.1\",\n"
    "  \"manifest_version\": 2,\n"
    "  \"description\": \n"
    "      \"end-to-end browser test for the declarative Content API\",\n"
    "  \"background\": {\n"
    "    \"scripts\": [\"background.js\"]\n"
    "  },\n"
    "  \"page_action\": {},\n"
    "  \"permissions\": [\n"
    "    \"declarativeContent\", \"bookmarks\"\n"
    "  ],\n"
    "  \"incognito\": \"spanning\"\n"
    "}\n";

const char kIncognitoSpecificBackground[] =
    "var PageStateMatcher = chrome.declarativeContent.PageStateMatcher;\n"
    "var ShowPageAction = chrome.declarativeContent.ShowPageAction;\n"
    "var inIncognitoContext = chrome.extension.inIncognitoContext;\n"
    "\n"
    "var hostPrefix = chrome.extension.inIncognitoContext ?\n"
    "    'test_split' : 'test_normal';\n"
    "var rule = {\n"
    "  conditions: [new PageStateMatcher({\n"
    "      pageUrl: {hostPrefix: hostPrefix}})],\n"
    "  actions: [new ShowPageAction()]\n"
    "}\n"
    "\n"
    "var onPageChanged = chrome.declarativeContent.onPageChanged;\n"
    "onPageChanged.removeRules(undefined, function() {\n"
    "  onPageChanged.addRules([rule], function() {\n"
    "    chrome.test.sendMessage(\n"
    "        !inIncognitoContext ? \"ready\" : \"ready (split)\");\n"
    "  });\n"
    "});\n";

const char kBackgroundHelpers[] =
    "var PageStateMatcher = chrome.declarativeContent.PageStateMatcher;\n"
    "var ShowPageAction = chrome.declarativeContent.ShowPageAction;\n"
    "var onPageChanged = chrome.declarativeContent.onPageChanged;\n"
    "var reply = window.domAutomationController.send.bind(\n"
    "    window.domAutomationController);\n"
    "\n"
    "function setRules(rules, responseString) {\n"
    "  onPageChanged.removeRules(undefined, function() {\n"
    "    onPageChanged.addRules(rules, function() {\n"
    "      if (chrome.runtime.lastError) {\n"
    "        reply(chrome.runtime.lastError.message);\n"
    "        return;\n"
    "      }\n"
    "      reply(responseString);\n"
    "    });\n"
    "  });\n"
    "};\n"
    "\n"
    "function addRules(rules, responseString) {\n"
    "  onPageChanged.addRules(rules, function() {\n"
    "    if (chrome.runtime.lastError) {\n"
    "      reply(chrome.runtime.lastError.message);\n"
    "      return;\n"
    "    }\n"
    "    reply(responseString);\n"
    "  });\n"
    "};\n"
    "\n"
    "function removeRule(id, responseString) {\n"
    "  onPageChanged.removeRules([id], function() {\n"
    "    if (chrome.runtime.lastError) {\n"
    "      reply(chrome.runtime.lastError.message);\n"
    "      return;\n"
    "    }\n"
    "    reply(responseString);\n"
    "  });\n"
    "};\n";

class DeclarativeContentApiTest : public ExtensionApiTest {
 public:
  DeclarativeContentApiTest() {}

 protected:
  enum IncognitoMode { SPANNING, SPLIT };

  // Checks that the rules are correctly evaluated for an extension in incognito
  // mode |mode| when the extension's incognito enable state is
  // |is_enabled_in_incognito|.
  void CheckIncognito(IncognitoMode mode, bool is_enabled_in_incognito);

  // Checks that the rules matching a bookmarked state of |is_bookmarked| are
  // correctly evaluated on bookmark events.
  void CheckBookmarkEvents(bool is_bookmarked);

  TestExtensionDir ext_dir_;

 private:
  DISALLOW_COPY_AND_ASSIGN(DeclarativeContentApiTest);
};

void DeclarativeContentApiTest::CheckIncognito(IncognitoMode mode,
                                               bool is_enabled_in_incognito) {
  std::string manifest = kDeclarativeContentManifest;
  if (mode == SPLIT) {
    base::ReplaceSubstringsAfterOffset(
        &manifest, 0, "\"incognito\": \"spanning\"",
        "\"incognito\": \"split\"");
    ASSERT_NE(kDeclarativeContentManifest, manifest);
  }
  ext_dir_.WriteManifest(manifest);
  ext_dir_.WriteFile(FILE_PATH_LITERAL("background.js"),
                     kIncognitoSpecificBackground);

  ExtensionTestMessageListener ready("ready", false);
  ExtensionTestMessageListener ready_incognito("ready (split)", false);

  const Extension* extension =
      is_enabled_in_incognito ? LoadExtensionIncognito(ext_dir_.UnpackedPath())
                              : LoadExtension(ext_dir_.UnpackedPath());
  ASSERT_TRUE(extension);

  Browser* incognito_browser = CreateIncognitoBrowser();
  const ExtensionAction* incognito_page_action =
      ExtensionActionManager::Get(incognito_browser->profile())->
      GetPageAction(*extension);
  ASSERT_TRUE(incognito_page_action);

  ASSERT_TRUE(ready.WaitUntilSatisfied());
  if (is_enabled_in_incognito && mode == SPLIT)
    ASSERT_TRUE(ready_incognito.WaitUntilSatisfied());

  content::WebContents* const incognito_tab =
      incognito_browser->tab_strip_model()->GetWebContentsAt(0);
  const int incognito_tab_id = ExtensionTabUtil::GetTabId(incognito_tab);

  EXPECT_FALSE(incognito_page_action->GetIsVisible(incognito_tab_id));

  NavigateInRenderer(incognito_tab, GURL("http://test_split/"));
  if (mode == SPLIT) {
    EXPECT_EQ(is_enabled_in_incognito,
              incognito_page_action->GetIsVisible(incognito_tab_id));
  } else {
    EXPECT_FALSE(incognito_page_action->GetIsVisible(incognito_tab_id));
  }

  NavigateInRenderer(incognito_tab, GURL("http://test_normal/"));
  if (mode != SPLIT) {
    EXPECT_EQ(is_enabled_in_incognito,
              incognito_page_action->GetIsVisible(incognito_tab_id));
  } else {
    EXPECT_FALSE(incognito_page_action->GetIsVisible(incognito_tab_id));
  }

  // Verify that everything works as expected in the non-incognito browser.
  const ExtensionAction* page_action =
      ExtensionActionManager::Get(browser()->profile())->
      GetPageAction(*extension);
  content::WebContents* const tab =
      browser()->tab_strip_model()->GetWebContentsAt(0);
  const int tab_id = ExtensionTabUtil::GetTabId(tab);

  EXPECT_FALSE(page_action->GetIsVisible(tab_id));

  NavigateInRenderer(tab, GURL("http://test_normal/"));
  EXPECT_TRUE(page_action->GetIsVisible(tab_id));

  NavigateInRenderer(tab, GURL("http://test_split/"));
  EXPECT_FALSE(page_action->GetIsVisible(tab_id));
}

void DeclarativeContentApiTest::CheckBookmarkEvents(bool match_is_bookmarked) {
  ext_dir_.WriteManifest(kDeclarativeContentManifest);
  ext_dir_.WriteFile(FILE_PATH_LITERAL("background.js"), kBackgroundHelpers);

  content::WebContents* const tab =
      browser()->tab_strip_model()->GetWebContentsAt(0);
  const int tab_id = ExtensionTabUtil::GetTabId(tab);

  const Extension* extension = LoadExtension(ext_dir_.UnpackedPath());
  ASSERT_TRUE(extension);
  const ExtensionAction* page_action = ExtensionActionManager::Get(
      browser()->profile())->GetPageAction(*extension);
  ASSERT_TRUE(page_action);

  NavigateInRenderer(tab, GURL("http://test1/"));
  EXPECT_FALSE(page_action->GetIsVisible(tab_id));

  static const char kSetIsBookmarkedRule[] =
      "setRules([{\n"
      "  conditions: [new PageStateMatcher({isBookmarked: %s})],\n"
      "  actions: [new ShowPageAction()]\n"
      "}], 'test_rule');\n";

  EXPECT_EQ("test_rule", ExecuteScriptInBackgroundPage(
      extension->id(),
      base::StringPrintf(kSetIsBookmarkedRule,
                         match_is_bookmarked ? "true" : "false")));
  EXPECT_EQ(!match_is_bookmarked, page_action->GetIsVisible(tab_id));

  // Check rule evaluation on add/remove bookmark.
  bookmarks::BookmarkModel* bookmark_model =
      BookmarkModelFactory::GetForBrowserContext(browser()->profile());
  const bookmarks::BookmarkNode* node =
      bookmark_model->AddURL(bookmark_model->other_node(), 0,
                             base::ASCIIToUTF16("title"),
                             GURL("http://test1/"));
  EXPECT_EQ(match_is_bookmarked, page_action->GetIsVisible(tab_id));

  bookmark_model->Remove(node);
  EXPECT_EQ(!match_is_bookmarked, page_action->GetIsVisible(tab_id));

  // Check rule evaluation on navigate to bookmarked and non-bookmarked URL.
  bookmark_model->AddURL(bookmark_model->other_node(), 0,
                         base::ASCIIToUTF16("title"),
                         GURL("http://test2/"));

  NavigateInRenderer(tab, GURL("http://test2/"));
  EXPECT_EQ(match_is_bookmarked, page_action->GetIsVisible(tab_id));

  NavigateInRenderer(tab, GURL("http://test3/"));
  EXPECT_EQ(!match_is_bookmarked, page_action->GetIsVisible(tab_id));
}

// Disabled due to flake. https://crbug.com/606574.
IN_PROC_BROWSER_TEST_F(DeclarativeContentApiTest, DISABLED_Overview) {
  ext_dir_.WriteManifest(kDeclarativeContentManifest);
  ext_dir_.WriteFile(
      FILE_PATH_LITERAL("background.js"),
      "var declarative = chrome.declarative;\n"
      "\n"
      "var PageStateMatcher = chrome.declarativeContent.PageStateMatcher;\n"
      "var ShowPageAction = chrome.declarativeContent.ShowPageAction;\n"
      "\n"
      "var rule = {\n"
      "  conditions: [new PageStateMatcher({\n"
      "                   pageUrl: {hostPrefix: \"test1\"}}),\n"
      "               new PageStateMatcher({\n"
      "                   css: [\"input[type='password']\"]})],\n"
      "  actions: [new ShowPageAction()]\n"
      "}\n"
      "\n"
      "var testEvent = chrome.declarativeContent.onPageChanged;\n"
      "\n"
      "testEvent.removeRules(undefined, function() {\n"
      "  testEvent.addRules([rule], function() {\n"
      "    chrome.test.sendMessage(\"ready\")\n"
      "  });\n"
      "});\n");
  ExtensionTestMessageListener ready("ready", false);
  const Extension* extension = LoadExtension(ext_dir_.UnpackedPath());
  ASSERT_TRUE(extension);
  const ExtensionAction* page_action =
      ExtensionActionManager::Get(browser()->profile())->
      GetPageAction(*extension);
  ASSERT_TRUE(page_action);

  ASSERT_TRUE(ready.WaitUntilSatisfied());
  content::WebContents* const tab =
      browser()->tab_strip_model()->GetWebContentsAt(0);
  const int tab_id = ExtensionTabUtil::GetTabId(tab);

  NavigateInRenderer(tab, GURL("http://test1/"));

  // The declarative API should show the page action instantly, rather
  // than waiting for the extension to run.
  EXPECT_TRUE(page_action->GetIsVisible(tab_id));

  // Make sure leaving a matching page unshows the page action.
  NavigateInRenderer(tab, GURL("http://not_checked/"));
  EXPECT_FALSE(page_action->GetIsVisible(tab_id));

  // Insert a password field to make sure that's noticed.
  // Notice that we touch offsetTop to force a synchronous layout.
  ASSERT_TRUE(content::ExecuteScript(
      tab, "document.body.innerHTML = '<input type=\"password\">';"
           "document.body.offsetTop;"));

  // Give the style match a chance to run and send back the matching-selector
  // update.
  ASSERT_TRUE(content::ExecuteScript(tab, std::string()));

  EXPECT_TRUE(page_action->GetIsVisible(tab_id))
      << "Adding a matching element should show the page action.";

  // Remove it again to make sure that reverts the action.
  // Notice that we touch offsetTop to force a synchronous layout.
  ASSERT_TRUE(content::ExecuteScript(
      tab, "document.body.innerHTML = 'Hello world';"
           "document.body.offsetTop;"));

  // Give the style match a chance to run and send back the matching-selector
  // update.
  ASSERT_TRUE(content::ExecuteScript(tab, std::string()));

  EXPECT_FALSE(page_action->GetIsVisible(tab_id))
      << "Removing the matching element should hide the page action again.";
}

// Test that adds two rules pointing to single action instance.
// Regression test for http://crbug.com/574149.
IN_PROC_BROWSER_TEST_F(DeclarativeContentApiTest, ReusedActionInstance) {
  ext_dir_.WriteManifest(kDeclarativeContentManifest);
  ext_dir_.WriteFile(
      FILE_PATH_LITERAL("background.js"),
      "var declarative = chrome.declarative;\n"
      "\n"
      "var PageStateMatcher = chrome.declarativeContent.PageStateMatcher;\n"
      "var ShowPageAction = chrome.declarativeContent.ShowPageAction;\n"
      "var actionInstance = new ShowPageAction();\n"
      "\n"
      "var rule1 = {\n"
      "  conditions: [new PageStateMatcher({\n"
      "                   pageUrl: {hostPrefix: \"test1\"}})],\n"
      "  actions: [actionInstance]\n"
      "};\n"
      "var rule2 = {\n"
      "  conditions: [new PageStateMatcher({\n"
      "                   pageUrl: {hostPrefix: \"test\"}})],\n"
      "  actions: [actionInstance]\n"
      "};\n"
      "\n"
      "var testEvent = chrome.declarativeContent.onPageChanged;\n"
      "\n"
      "testEvent.removeRules(undefined, function() {\n"
      "  testEvent.addRules([rule1, rule2], function() {\n"
      "    chrome.test.sendMessage(\"ready\");\n"
      "  });\n"
      "});\n");
  ExtensionTestMessageListener ready("ready", false);
  const Extension* extension = LoadExtension(ext_dir_.UnpackedPath());
  ASSERT_TRUE(extension);
  const ExtensionAction* page_action =
      ExtensionActionManager::Get(browser()->profile())
          ->GetPageAction(*extension);
  ASSERT_TRUE(page_action);

  ASSERT_TRUE(ready.WaitUntilSatisfied());
  content::WebContents* const tab =
      browser()->tab_strip_model()->GetWebContentsAt(0);
  const int tab_id = ExtensionTabUtil::GetTabId(tab);

  // This navigation matches both rules.
  NavigateInRenderer(tab, GURL("http://test1/"));

  EXPECT_TRUE(page_action->GetIsVisible(tab_id));
}

// Tests that the rules are evaluated at the time they are added or removed.
IN_PROC_BROWSER_TEST_F(DeclarativeContentApiTest, RulesEvaluatedOnAddRemove) {
  ext_dir_.WriteManifest(kDeclarativeContentManifest);
  ext_dir_.WriteFile(FILE_PATH_LITERAL("background.js"), kBackgroundHelpers);
  const Extension* extension = LoadExtension(ext_dir_.UnpackedPath());
  ASSERT_TRUE(extension);
  const ExtensionAction* page_action =
      ExtensionActionManager::Get(browser()->profile())->
      GetPageAction(*extension);
  ASSERT_TRUE(page_action);

  content::WebContents* const tab =
      browser()->tab_strip_model()->GetWebContentsAt(0);
  const int tab_id = ExtensionTabUtil::GetTabId(tab);

  NavigateInRenderer(tab, GURL("http://test1/"));

  const std::string kAddTestRules =
      "addRules([{\n"
      "  id: '1',\n"
      "  conditions: [new PageStateMatcher({\n"
      "                   pageUrl: {hostPrefix: \"test1\"}})],\n"
      "  actions: [new ShowPageAction()]\n"
      "}, {\n"
      "  id: '2',\n"
      "  conditions: [new PageStateMatcher({\n"
      "                   pageUrl: {hostPrefix: \"test2\"}})],\n"
      "  actions: [new ShowPageAction()]\n"
      "}], 'add_rules');\n";
  EXPECT_EQ("add_rules",
            ExecuteScriptInBackgroundPage(extension->id(), kAddTestRules));

  EXPECT_TRUE(page_action->GetIsVisible(tab_id));

  const std::string kRemoveTestRule1 = "removeRule('1', 'remove_rule1');\n";
  EXPECT_EQ("remove_rule1",
            ExecuteScriptInBackgroundPage(extension->id(), kRemoveTestRule1));

  EXPECT_FALSE(page_action->GetIsVisible(tab_id));

  NavigateInRenderer(tab, GURL("http://test2/"));

  EXPECT_TRUE(page_action->GetIsVisible(tab_id));

  const std::string kRemoveTestRule2 = "removeRule('2', 'remove_rule2');\n";
  EXPECT_EQ("remove_rule2",
            ExecuteScriptInBackgroundPage(extension->id(), kRemoveTestRule2));

  EXPECT_FALSE(page_action->GetIsVisible(tab_id));
}

// Tests that rules from manifest are added and evaluated properly.
IN_PROC_BROWSER_TEST_F(DeclarativeContentApiTest, RulesAddedFromManifest) {
  const char manifest[] =
      "{\n"
      "  \"name\": \"Declarative Content apitest\",\n"
      "  \"version\": \"0.1\",\n"
      "  \"manifest_version\": 2,\n"
      "  \"page_action\": {},\n"
      "  \"permissions\": [\n"
      "    \"declarativeContent\"\n"
      "  ],\n"
      "  \"event_rules\": [{\n"
      "    \"event\": \"declarativeContent.onPageChanged\",\n"
      "    \"actions\": [{\n"
      "      \"type\": \"declarativeContent.ShowPageAction\"\n"
      "    }],\n"
      "    \"conditions\": [{\n"
      "      \"type\": \"declarativeContent.PageStateMatcher\",\n"
      "      \"pageUrl\": {\"hostPrefix\": \"test1\"}\n"
      "    }]\n"
      "  }]\n"
      "}\n";
  ext_dir_.WriteManifest(manifest);
  const Extension* extension = LoadExtension(ext_dir_.UnpackedPath());
  ASSERT_TRUE(extension);
  const ExtensionAction* page_action =
      ExtensionActionManager::Get(browser()->profile())
          ->GetPageAction(*extension);
  ASSERT_TRUE(page_action);

  content::WebContents* const tab =
      browser()->tab_strip_model()->GetWebContentsAt(0);
  const int tab_id = ExtensionTabUtil::GetTabId(tab);

  NavigateInRenderer(tab, GURL("http://blank/"));
  EXPECT_FALSE(page_action->GetIsVisible(tab_id));
  NavigateInRenderer(tab, GURL("http://test1/"));
  EXPECT_TRUE(page_action->GetIsVisible(tab_id));
  NavigateInRenderer(tab, GURL("http://test2/"));
  EXPECT_FALSE(page_action->GetIsVisible(tab_id));
}

// Tests that rules are not evaluated in incognito browser windows when the
// extension specifies spanning incognito mode but is not enabled for incognito.
IN_PROC_BROWSER_TEST_F(DeclarativeContentApiTest,
                       DisabledForSpanningIncognito) {
  CheckIncognito(SPANNING, false);
}

// Tests that rules are evaluated in incognito browser windows when the
// extension specifies spanning incognito mode and is enabled for incognito.
IN_PROC_BROWSER_TEST_F(DeclarativeContentApiTest, EnabledForSpanningIncognito) {
  CheckIncognito(SPANNING, true);
}

// Tests that rules are not evaluated in incognito browser windows when the
// extension specifies split incognito mode but is not enabled for incognito.
IN_PROC_BROWSER_TEST_F(DeclarativeContentApiTest, DisabledForSplitIncognito) {
  CheckIncognito(SPLIT, false);
}

// Tests that rules are evaluated in incognito browser windows when the
// extension specifies split incognito mode and is enabled for incognito.
IN_PROC_BROWSER_TEST_F(DeclarativeContentApiTest, EnabledForSplitIncognito) {
  CheckIncognito(SPLIT, true);
}

// Tests that rules are evaluated for an incognito tab that exists at the time
// the rules are added.
IN_PROC_BROWSER_TEST_F(DeclarativeContentApiTest,
                       RulesEvaluatedForExistingIncognitoTab) {
  Browser* incognito_browser = CreateIncognitoBrowser();
  content::WebContents* const incognito_tab =
      incognito_browser->tab_strip_model()->GetWebContentsAt(0);
  const int incognito_tab_id = ExtensionTabUtil::GetTabId(incognito_tab);

  NavigateInRenderer(incognito_tab, GURL("http://test_normal/"));

  ext_dir_.WriteManifest(kDeclarativeContentManifest);
  ext_dir_.WriteFile(FILE_PATH_LITERAL("background.js"),
                     kIncognitoSpecificBackground);
  ExtensionTestMessageListener ready("ready", false);
  const Extension* extension = LoadExtensionIncognito(ext_dir_.UnpackedPath());
  ASSERT_TRUE(extension);
  ASSERT_TRUE(ready.WaitUntilSatisfied());

  const ExtensionAction* incognito_page_action =
      ExtensionActionManager::Get(incognito_browser->profile())->
      GetPageAction(*extension);
  ASSERT_TRUE(incognito_page_action);

  // The page action should be shown.
  EXPECT_TRUE(incognito_page_action->GetIsVisible(incognito_tab_id));
}

// Frequently times out on ChromiumOS debug builders: https://crbug.com/512431.
#if defined(OS_CHROMEOS) && !defined(NDEBUG)
#define MAYBE_PRE_RulesPersistence DISABLED_PRE_RulesPersistence
#define MAYBE_RulesPersistence DISABLED_RulesPersistence
#else
#define MAYBE_PRE_RulesPersistence PRE_RulesPersistence
#define MAYBE_RulesPersistence RulesPersistence
#endif

// Sets up rules matching http://test1/ in a normal and incognito browser.
IN_PROC_BROWSER_TEST_F(DeclarativeContentApiTest, MAYBE_PRE_RulesPersistence) {
  ExtensionTestMessageListener ready("ready", false);
  ExtensionTestMessageListener ready_split("ready (split)", false);
  // An on-disk extension is required so that it can be reloaded later in the
  // RulesPersistence test.
  const Extension* extension =
      LoadExtensionIncognito(test_data_dir_.AppendASCII("declarative_content")
                             .AppendASCII("persistence"));
  ASSERT_TRUE(extension);
  ASSERT_TRUE(ready.WaitUntilSatisfied());

  CreateIncognitoBrowser();
  ASSERT_TRUE(ready_split.WaitUntilSatisfied());
}

// Reloads the extension from PRE_RulesPersistence and checks that the rules
// continue to work as expected after being persisted and reloaded.
IN_PROC_BROWSER_TEST_F(DeclarativeContentApiTest, MAYBE_RulesPersistence) {
  ExtensionTestMessageListener ready("second run ready", false);
  ExtensionTestMessageListener ready_split("second run ready (split)", false);
  ASSERT_TRUE(ready.WaitUntilSatisfied());

  ExtensionRegistry* registry = ExtensionRegistry::Get(profile());
  const Extension* extension =
      GetExtensionByPath(registry->enabled_extensions(),
                         test_data_dir_.AppendASCII("declarative_content")
                         .AppendASCII("persistence"));

  // Check non-incognito browser.
  content::WebContents* const tab =
      browser()->tab_strip_model()->GetWebContentsAt(0);
  const int tab_id = ExtensionTabUtil::GetTabId(tab);

  const ExtensionAction* page_action =
      ExtensionActionManager::Get(browser()->profile())->
      GetPageAction(*extension);
  ASSERT_TRUE(page_action);
  EXPECT_FALSE(page_action->GetIsVisible(tab_id));

  NavigateInRenderer(tab, GURL("http://test_normal/"));
  EXPECT_TRUE(page_action->GetIsVisible(tab_id));

  NavigateInRenderer(tab, GURL("http://test_split/"));
  EXPECT_FALSE(page_action->GetIsVisible(tab_id));

  // Check incognito browser.
  Browser* incognito_browser = CreateIncognitoBrowser();
  ASSERT_TRUE(ready_split.WaitUntilSatisfied());
  content::WebContents* const incognito_tab =
      incognito_browser->tab_strip_model()->GetWebContentsAt(0);
  const int incognito_tab_id = ExtensionTabUtil::GetTabId(incognito_tab);

  const ExtensionAction* incognito_page_action =
      ExtensionActionManager::Get(incognito_browser->profile())->
      GetPageAction(*extension);
  ASSERT_TRUE(incognito_page_action);

  NavigateInRenderer(incognito_tab, GURL("http://test_split/"));
  EXPECT_TRUE(incognito_page_action->GetIsVisible(incognito_tab_id));

  NavigateInRenderer(incognito_tab, GURL("http://test_normal/"));
  EXPECT_FALSE(incognito_page_action->GetIsVisible(incognito_tab_id));
}

// http://crbug.com/304373
#if defined(OS_WIN)
// Fails on XP: http://crbug.com/515717
#define MAYBE_UninstallWhileActivePageAction \
  DISABLED_UninstallWhileActivePageAction
#else
#define MAYBE_UninstallWhileActivePageAction UninstallWhileActivePageAction
#endif
IN_PROC_BROWSER_TEST_F(DeclarativeContentApiTest,
                       MAYBE_UninstallWhileActivePageAction) {
  ext_dir_.WriteManifest(kDeclarativeContentManifest);
  ext_dir_.WriteFile(FILE_PATH_LITERAL("background.js"), kBackgroundHelpers);
  const Extension* extension = LoadExtension(ext_dir_.UnpackedPath());
  ASSERT_TRUE(extension);
  const std::string extension_id = extension->id();
  const ExtensionAction* page_action = ExtensionActionManager::Get(
      browser()->profile())->GetPageAction(*extension);
  ASSERT_TRUE(page_action);

  const std::string kTestRule =
      "setRules([{\n"
      "  conditions: [new PageStateMatcher({\n"
      "                   pageUrl: {hostPrefix: \"test\"}})],\n"
      "  actions: [new ShowPageAction()]\n"
      "}], 'test_rule');\n";
  EXPECT_EQ("test_rule",
            ExecuteScriptInBackgroundPage(extension_id, kTestRule));

  content::WebContents* const tab =
      browser()->tab_strip_model()->GetWebContentsAt(0);
  const int tab_id = ExtensionTabUtil::GetTabId(tab);

  NavigateInRenderer(tab, GURL("http://test/"));

  EXPECT_TRUE(page_action->GetIsVisible(tab_id));
  EXPECT_TRUE(WaitForPageActionVisibilityChangeTo(1));
  EXPECT_EQ(1u, extension_action_test_util::GetVisiblePageActionCount(tab));
  EXPECT_EQ(1u, extension_action_test_util::GetTotalPageActionCount(tab));

  ReloadExtension(extension_id);  // Invalidates page_action and extension.
  EXPECT_EQ("test_rule",
            ExecuteScriptInBackgroundPage(extension_id, kTestRule));
  // TODO(jyasskin): Apply new rules to existing tabs, without waiting for a
  // navigation.
  NavigateInRenderer(tab, GURL("http://test/"));
  EXPECT_TRUE(WaitForPageActionVisibilityChangeTo(1));
  EXPECT_EQ(1u, extension_action_test_util::GetVisiblePageActionCount(tab));
  EXPECT_EQ(1u, extension_action_test_util::GetTotalPageActionCount(tab));

  UnloadExtension(extension_id);
  NavigateInRenderer(tab, GURL("http://test/"));
  EXPECT_TRUE(WaitForPageActionVisibilityChangeTo(0));
  EXPECT_EQ(0u, extension_action_test_util::GetVisiblePageActionCount(tab));
  EXPECT_EQ(0u, extension_action_test_util::GetTotalPageActionCount(tab));
}

// This tests against a renderer crash that was present during development.
IN_PROC_BROWSER_TEST_F(DeclarativeContentApiTest,
                       DISABLED_AddExtensionMatchingExistingTabWithDeadFrames) {
  ext_dir_.WriteManifest(kDeclarativeContentManifest);
  ext_dir_.WriteFile(FILE_PATH_LITERAL("background.js"), kBackgroundHelpers);
  content::WebContents* const tab =
      browser()->tab_strip_model()->GetWebContentsAt(0);
  const int tab_id = ExtensionTabUtil::GetTabId(tab);

  ASSERT_TRUE(content::ExecuteScript(
      tab, "document.body.innerHTML = '<iframe src=\"http://test2\">';"));
  // Replace the iframe to destroy its WebFrame.
  ASSERT_TRUE(content::ExecuteScript(
      tab, "document.body.innerHTML = '<span class=\"foo\">';"));

  const Extension* extension = LoadExtension(ext_dir_.UnpackedPath());
  ASSERT_TRUE(extension);
  const ExtensionAction* page_action = ExtensionActionManager::Get(
      browser()->profile())->GetPageAction(*extension);
  ASSERT_TRUE(page_action);
  EXPECT_FALSE(page_action->GetIsVisible(tab_id));

  EXPECT_EQ("rule0",
            ExecuteScriptInBackgroundPage(
                extension->id(),
                "setRules([{\n"
                "  conditions: [new PageStateMatcher({\n"
                "                   css: [\"span[class=foo]\"]})],\n"
                "  actions: [new ShowPageAction()]\n"
                "}], 'rule0');\n"));
  // Give the renderer a chance to apply the rules change and notify the
  // browser.  This takes one time through the Blink message loop to receive
  // the rule change and apply the new stylesheet, and a second to dedupe the
  // update.
  ASSERT_TRUE(content::ExecuteScript(tab, std::string()));
  ASSERT_TRUE(content::ExecuteScript(tab, std::string()));

  EXPECT_FALSE(tab->IsCrashed());
  EXPECT_TRUE(page_action->GetIsVisible(tab_id))
      << "Loading an extension when an open page matches its rules "
      << "should show the page action.";

  EXPECT_EQ("removed",
            ExecuteScriptInBackgroundPage(
                extension->id(),
                "onPageChanged.removeRules(undefined, function() {\n"
                "  window.domAutomationController.send('removed');\n"
                "});\n"));
  EXPECT_FALSE(page_action->GetIsVisible(tab_id));
}

namespace {

class ShowPageActionWithoutPageActionTest
    : public DeclarativeContentApiTest,
      public testing::WithParamInterface<bool> {};

}  // namespace

IN_PROC_BROWSER_TEST_P(ShowPageActionWithoutPageActionTest, Test) {
  bool has_browser_action = GetParam();
  // Load an extension without a page action.
  std::string manifest_without_page_action = kDeclarativeContentManifest;
  std::string replacement = has_browser_action ? "\"browser_action\": {}," : "";
  base::ReplaceSubstringsAfterOffset(&manifest_without_page_action, 0,
                                     "\"page_action\": {},",
                                     replacement.c_str());
  ASSERT_NE(kDeclarativeContentManifest, manifest_without_page_action);
  ext_dir_.WriteManifest(manifest_without_page_action);
  ext_dir_.WriteFile(FILE_PATH_LITERAL("background.js"), kBackgroundHelpers);

  ChromeTestExtensionLoader loader(profile());
  scoped_refptr<const Extension> extension =
      loader.LoadExtension(ext_dir_.UnpackedPath());
  ASSERT_TRUE(extension);

  const char kScript[] =
      "setRules([{\n"
      "  conditions: [new PageStateMatcher({\n"
      "                   pageUrl: {hostPrefix: \"test\"}})],\n"
      "  actions: [new ShowPageAction()]\n"
      "}], 'test_rule');\n";
  const char kErrorSubstr[] = "without a page action";
  std::string result = ExecuteScriptInBackgroundPage(extension->id(), kScript);

  // Extensions with no action provided are given a page action by default
  // (for visibility reasons). If an extension has a browser action, it
  // should cause an error.
  if (has_browser_action)
    EXPECT_THAT(result, testing::HasSubstr(kErrorSubstr));
  else
    EXPECT_THAT(result, testing::Not(testing::HasSubstr(kErrorSubstr)));

  content::WebContents* const tab =
      browser()->tab_strip_model()->GetWebContentsAt(0);
  NavigateInRenderer(tab, GURL("http://test/"));

  bool expect_page_action = !has_browser_action;
  ExtensionAction* page_action =
      ExtensionActionManager::Get(browser()->profile())
          ->GetPageAction(*extension);
  EXPECT_EQ(expect_page_action, page_action != nullptr);
  EXPECT_EQ(expect_page_action ? 1u : 0u,
            extension_action_test_util::GetVisiblePageActionCount(tab));
}

INSTANTIATE_TEST_CASE_P(ShowPageActionWithoutPageActionTest,
                        ShowPageActionWithoutPageActionTest,
                        ::testing::Bool());

IN_PROC_BROWSER_TEST_F(DeclarativeContentApiTest,
                       CanonicalizesPageStateMatcherCss) {
  ext_dir_.WriteManifest(kDeclarativeContentManifest);
  ext_dir_.WriteFile(
      FILE_PATH_LITERAL("background.js"),
      "var PageStateMatcher = chrome.declarativeContent.PageStateMatcher;\n"
      "function Return(obj) {\n"
      "  window.domAutomationController.send('' + obj);\n"
      "}\n");
  const Extension* extension = LoadExtension(ext_dir_.UnpackedPath());
  ASSERT_TRUE(extension);

  EXPECT_EQ("input[type=\"password\"]",
            ExecuteScriptInBackgroundPage(
                extension->id(),
                "var psm = new PageStateMatcher(\n"
                "    {css: [\"input[type='password']\"]});\n"
                "Return(psm.css);"));

  EXPECT_THAT(ExecuteScriptInBackgroundPage(
                  extension->id(),
                  "try {\n"
                  "  new PageStateMatcher({css: 'Not-an-array'});\n"
                  "  Return('Failed to throw');\n"
                  "} catch (e) {\n"
                  "  Return(e.message);\n"
                  "}\n"),
              testing::ContainsRegex("css.*xpected '?array'?"));
  EXPECT_THAT(ExecuteScriptInBackgroundPage(
                  extension->id(),
                  "try {\n"
                  "  new PageStateMatcher({css: [null]});\n"  // Not a string.
                  "  Return('Failed to throw');\n"
                  "} catch (e) {\n"
                  "  Return(e.message);\n"
                  "}\n"),
              testing::ContainsRegex("css.*0.*xpected '?string'?"));
  EXPECT_THAT(ExecuteScriptInBackgroundPage(
                  extension->id(),
                  "try {\n"
                  // Invalid CSS:
                  "  new PageStateMatcher({css: [\"input''\"]});\n"
                  "  Return('Failed to throw');\n"
                  "} catch (e) {\n"
                  "  Return(e.message);\n"
                  "}\n"),
              testing::ContainsRegex("valid.*: input''$"));
  EXPECT_THAT(ExecuteScriptInBackgroundPage(
                  extension->id(),
                  "try {\n"
                  // "Complex" selector:
                  "  new PageStateMatcher({css: ['div input']});\n"
                  "  Return('Failed to throw');\n"
                  "} catch (e) {\n"
                  "  Return(e.message);\n"
                  "}\n"),
              testing::ContainsRegex("selector.*: div input$"));
}

// Tests that the rules with isBookmarked: true are evaluated when handling
// bookmarking events.
IN_PROC_BROWSER_TEST_F(DeclarativeContentApiTest,
                       IsBookmarkedRulesEvaluatedOnBookmarkEvents) {
  CheckBookmarkEvents(true);
}

// Tests that the rules with isBookmarked: false are evaluated when handling
// bookmarking events.
IN_PROC_BROWSER_TEST_F(DeclarativeContentApiTest,
                       NotBookmarkedRulesEvaluatedOnBookmarkEvents) {
  CheckBookmarkEvents(false);
}

// https://crbug.com/497586
IN_PROC_BROWSER_TEST_F(DeclarativeContentApiTest,
                       WebContentsWithoutTabAddedNotificationAtOnLoaded) {
  // Add a web contents to the tab strip in a way that doesn't trigger
  // NOTIFICATION_TAB_ADDED.
  std::unique_ptr<content::WebContents> contents = content::WebContents::Create(
      content::WebContents::CreateParams(profile()));
  browser()->tab_strip_model()->AppendWebContents(std::move(contents), false);

  // The actual extension contents don't matter here -- we're just looking to
  // trigger OnExtensionLoaded.
  ext_dir_.WriteManifest(kDeclarativeContentManifest);
  ext_dir_.WriteFile(FILE_PATH_LITERAL("background.js"), kBackgroundHelpers);
  ASSERT_TRUE(LoadExtension(ext_dir_.UnpackedPath()));
}

// https://crbug.com/501225
IN_PROC_BROWSER_TEST_F(DeclarativeContentApiTest,
                       PendingWebContentsClearedOnRemoveRules) {
  ext_dir_.WriteManifest(kDeclarativeContentManifest);
  ext_dir_.WriteFile(FILE_PATH_LITERAL("background.js"), kBackgroundHelpers);
  const Extension* extension = LoadExtension(ext_dir_.UnpackedPath());
  ASSERT_TRUE(extension);
  const ExtensionAction* page_action = ExtensionActionManager::Get(
      browser()->profile())->GetPageAction(*extension);
  ASSERT_TRUE(page_action);

  // Create two tabs.
  content::WebContents* const tab1 =
      browser()->tab_strip_model()->GetWebContentsAt(0);

  AddTabAtIndex(1, GURL("http://test2/"), ui::PAGE_TRANSITION_LINK);
  content::WebContents* tab2 =
      browser()->tab_strip_model()->GetWebContentsAt(1);

  // Add a rule matching the second tab.
  const std::string kAddTestRules =
      "addRules([{\n"
      "  id: '1',\n"
      "  conditions: [new PageStateMatcher({\n"
      "                   pageUrl: {hostPrefix: \"test1\"}})],\n"
      "  actions: [new ShowPageAction()]\n"
      "}, {\n"
      "  id: '2',\n"
      "  conditions: [new PageStateMatcher({\n"
      "                   pageUrl: {hostPrefix: \"test2\"}})],\n"
      "  actions: [new ShowPageAction()]\n"
      "}], 'add_rules');\n";
  EXPECT_EQ("add_rules",
            ExecuteScriptInBackgroundPage(extension->id(), kAddTestRules));
  EXPECT_TRUE(page_action->GetIsVisible(ExtensionTabUtil::GetTabId(tab2)));

  // Remove the rule.
  const std::string kRemoveTestRule1 = "removeRule('2', 'remove_rule1');\n";
  EXPECT_EQ("remove_rule1",
            ExecuteScriptInBackgroundPage(extension->id(), kRemoveTestRule1));

  // Remove the second tab, then trigger a rule evaluation for the remaining
  // tab.
  browser()->tab_strip_model()->DetachWebContentsAt(
      browser()->tab_strip_model()->GetIndexOfWebContents(tab2));
  NavigateInRenderer(tab1, GURL("http://test1/"));
  EXPECT_TRUE(page_action->GetIsVisible(ExtensionTabUtil::GetTabId(tab1)));
}

// https://crbug.com/517492
#if defined(OS_WIN)
// Fails on XP: http://crbug.com/515717
#define MAYBE_RemoveAllRulesAfterExtensionUninstall \
  DISABLED_RemoveAllRulesAfterExtensionUninstall
#else
#define MAYBE_RemoveAllRulesAfterExtensionUninstall \
  RemoveAllRulesAfterExtensionUninstall
#endif
IN_PROC_BROWSER_TEST_F(DeclarativeContentApiTest,
                       MAYBE_RemoveAllRulesAfterExtensionUninstall) {
  ext_dir_.WriteManifest(kDeclarativeContentManifest);
  ext_dir_.WriteFile(FILE_PATH_LITERAL("background.js"), kBackgroundHelpers);

  // Load the extension, add a rule, then uninstall the extension.
  const Extension* extension = LoadExtension(ext_dir_.UnpackedPath());
  ASSERT_TRUE(extension);

  const std::string kAddTestRule =
      "addRules([{\n"
      "  id: '1',\n"
      "  conditions: [],\n"
      "  actions: [new ShowPageAction()]\n"
      "}], 'add_rule');\n";
  EXPECT_EQ("add_rule",
            ExecuteScriptInBackgroundPage(extension->id(), kAddTestRule));

  ExtensionService* extension_service = extensions::ExtensionSystem::Get(
      browser()->profile())->extension_service();

  base::string16 error;
  ASSERT_TRUE(extension_service->UninstallExtension(
      extension->id(),
      UNINSTALL_REASON_FOR_TESTING,
      &error));
  ASSERT_EQ(base::ASCIIToUTF16(""), error);

  // Reload the extension, then add and remove a rule.
  extension = LoadExtension(ext_dir_.UnpackedPath());
  ASSERT_TRUE(extension);

  EXPECT_EQ("add_rule",
            ExecuteScriptInBackgroundPage(extension->id(), kAddTestRule));

  const std::string kRemoveTestRule1 = "removeRule('1', 'remove_rule1');\n";
  EXPECT_EQ("remove_rule1",
            ExecuteScriptInBackgroundPage(extension->id(), kRemoveTestRule1));
}


// TODO(wittman): Once ChromeContentRulesRegistry operates on condition and
// action interfaces, add a test that checks that a navigation always evaluates
// consistent URL state for all conditions. i.e.: if condition1 evaluates to
// false on url0 and true on url1, and condition2 evaluates to true on url0 and
// false on url1, navigate from url0 to url1 and validate that no action is
// triggered. Do the same when navigating back to url0. This kind of test is
// unfortunately not feasible with the current implementation and the existing
// supported conditions and actions.

}  // namespace
}  // namespace extensions
