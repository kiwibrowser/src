// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/extensions/extension_context_menu_model.h"

#include <utility>

#include "base/macros.h"
#include "base/memory/ptr_util.h"
#include "base/strings/utf_string_conversions.h"
#include "base/test/scoped_feature_list.h"
#include "chrome/app/chrome_command_ids.h"
#include "chrome/browser/extensions/api/extension_action/extension_action_api.h"
#include "chrome/browser/extensions/context_menu_matcher.h"
#include "chrome/browser/extensions/extension_action_runner.h"
#include "chrome/browser/extensions/extension_action_test_util.h"
#include "chrome/browser/extensions/extension_service.h"
#include "chrome/browser/extensions/extension_service_test_base.h"
#include "chrome/browser/extensions/menu_manager.h"
#include "chrome/browser/extensions/menu_manager_factory.h"
#include "chrome/browser/extensions/scripting_permissions_modifier.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/common/extensions/api/context_menus.h"
#include "chrome/grit/chromium_strings.h"
#include "chrome/grit/generated_resources.h"
#include "chrome/test/base/test_browser_window.h"
#include "chrome/test/base/testing_profile.h"
#include "components/crx_file/id_util.h"
#include "content/public/test/browser_side_navigation_test_utils.h"
#include "content/public/test/web_contents_tester.h"
#include "extensions/browser/extension_dialog_auto_confirm.h"
#include "extensions/browser/extension_registry.h"
#include "extensions/browser/extension_system.h"
#include "extensions/browser/test_extension_registry_observer.h"
#include "extensions/browser/test_management_policy.h"
#include "extensions/common/extension_builder.h"
#include "extensions/common/extension_features.h"
#include "extensions/common/manifest.h"
#include "extensions/common/manifest_constants.h"
#include "extensions/common/manifest_handlers/options_page_info.h"
#include "extensions/common/permissions/permissions_data.h"
#include "extensions/common/value_builder.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/display/test/scoped_screen_override.h"
#include "ui/display/test/test_screen.h"
#include "ui/gfx/image/image.h"

namespace extensions {

using display::test::ScopedScreenOverride;

namespace {

void Increment(int* i) {
  CHECK(i);
  ++(*i);
}

// Label for test extension menu item.
const char* kTestExtensionItemLabel = "test-ext-item";

std::string item_label() {
  return kTestExtensionItemLabel;
}

class MenuBuilder {
 public:
  MenuBuilder(scoped_refptr<const Extension> extension,
              Browser* browser,
              MenuManager* menu_manager)
      : extension_(extension),
        browser_(browser),
        menu_manager_(menu_manager),
        cur_id_(0) {}
  ~MenuBuilder() {}

  std::unique_ptr<ExtensionContextMenuModel> BuildMenu() {
    return std::make_unique<ExtensionContextMenuModel>(
        extension_.get(), browser_, ExtensionContextMenuModel::VISIBLE,
        nullptr);
  }

  void AddContextItem(MenuItem::Context context) {
    MenuItem::Id id(false /* not incognito */,
                    MenuItem::ExtensionKey(extension_->id()));
    id.uid = ++cur_id_;
    menu_manager_->AddContextItem(
        extension_.get(),
        std::make_unique<MenuItem>(id, kTestExtensionItemLabel,
                                   false,  // check`ed
                                   true,   // visible
                                   true,   // enabled
                                   MenuItem::NORMAL,
                                   MenuItem::ContextList(context)));
  }

  void SetItemVisibility(int item_id, bool visible) {
    MenuItem::Id id(false /* not incognito */,
                    MenuItem::ExtensionKey(extension_->id()));
    id.uid = item_id;

    menu_manager_->GetItemById(id)->set_visible(visible);
  }

  void SetItemTitle(int item_id, const std::string& title) {
    MenuItem::Id id(false /* not incognito */,
                    MenuItem::ExtensionKey(extension_->id()));
    id.uid = item_id;

    menu_manager_->GetItemById(id)->set_title(title);
  }

 private:
  scoped_refptr<const Extension> extension_;
  Browser* browser_;
  MenuManager* menu_manager_;
  int cur_id_;

  DISALLOW_COPY_AND_ASSIGN(MenuBuilder);
};

// Returns the number of extension menu items that show up in |model|.
// For this test, all the extension items have same label
// |kTestExtensionItemLabel|.
int CountExtensionItems(const ExtensionContextMenuModel& model) {
  base::string16 expected_label = base::ASCIIToUTF16(kTestExtensionItemLabel);
  int num_items_found = 0;
  int num_custom_found = 0;
  for (int i = 0; i < model.GetItemCount(); ++i) {
    base::string16 actual_label = model.GetLabelAt(i);
    int command_id = model.GetCommandIdAt(i);
    // If the command id is not visible, it should not be counted.
    if (model.IsCommandIdVisible(command_id)) {
      // The last character of |expected_label| can be the item number (e.g
      // "test-ext-item" -> "test-ext-item1"). In checking that extensions items
      // have the same label |kTestExtensionItemLabel|, the specific item number
      // is ignored, [0, expected_label.size).
      if (base::StartsWith(actual_label, expected_label,
                           base::CompareCase::SENSITIVE))
        ++num_items_found;
      if (ContextMenuMatcher::IsExtensionsCustomCommandId(command_id))
        ++num_custom_found;
    }
  }
  // The only custom extension items present on the menu should be those we
  // added in the test.
  EXPECT_EQ(num_items_found, num_custom_found);
  return num_items_found;
}

// Checks that the model has the extension items in the exact order specified by
// |item_number|.
void VerifyItems(const ExtensionContextMenuModel& model,
                 std::vector<std::string> item_number) {
  size_t j = 0;
  for (int i = 0; i < model.GetItemCount(); i++) {
    int command_id = model.GetCommandIdAt(i);
    if (ContextMenuMatcher::IsExtensionsCustomCommandId(command_id) &&
        model.IsCommandIdVisible(command_id)) {
      ASSERT_LT(j, item_number.size());
      EXPECT_EQ(base::ASCIIToUTF16(item_label() + item_number[j]),
                model.GetLabelAt(i));
      j++;
    }
  }
  EXPECT_EQ(item_number.size(), j);
}

}  // namespace

class ExtensionContextMenuModelTest : public ExtensionServiceTestBase {
 public:
  ExtensionContextMenuModelTest();

  // Build an extension to pass to the menu constructor, with the action
  // specified by |action_key|.
  const Extension* AddExtension(const std::string& name,
                                const char* action_key,
                                Manifest::Location location);
  const Extension* AddExtensionWithHostPermission(
      const std::string& name,
      const char* action_key,
      Manifest::Location location,
      const std::string& host_permission);

  Browser* GetBrowser();

  void SetUp() override;
  void TearDown() override;

 private:
  std::unique_ptr<TestBrowserWindow> test_window_;
  std::unique_ptr<Browser> browser_;
  display::test::TestScreen test_screen_;
  std::unique_ptr<ScopedScreenOverride> scoped_screen_override_;

  DISALLOW_COPY_AND_ASSIGN(ExtensionContextMenuModelTest);
};

ExtensionContextMenuModelTest::ExtensionContextMenuModelTest() {}

const Extension* ExtensionContextMenuModelTest::AddExtension(
    const std::string& name,
    const char* action_key,
    Manifest::Location location) {
  return AddExtensionWithHostPermission(name, action_key, location,
                                        std::string());
}

const Extension* ExtensionContextMenuModelTest::AddExtensionWithHostPermission(
    const std::string& name,
    const char* action_key,
    Manifest::Location location,
    const std::string& host_permission) {
  DictionaryBuilder manifest;
  manifest.Set("name", name)
      .Set("version", "1")
      .Set("manifest_version", 2);
  if (action_key)
    manifest.Set(action_key, DictionaryBuilder().Build());
  if (!host_permission.empty())
    manifest.Set("permissions", ListBuilder().Append(host_permission).Build());
  scoped_refptr<const Extension> extension =
      ExtensionBuilder()
          .SetManifest(manifest.Build())
          .SetID(crx_file::id_util::GenerateId(name))
          .SetLocation(location)
          .Build();
  if (!extension.get())
    ADD_FAILURE();
  service()->GrantPermissions(extension.get());
  service()->AddExtension(extension.get());
  return extension.get();
}

Browser* ExtensionContextMenuModelTest::GetBrowser() {
  if (!browser_) {
    Browser::CreateParams params(profile(), true);
    test_window_.reset(new TestBrowserWindow());
    params.window = test_window_.get();
    browser_.reset(new Browser(params));
  }
  return browser_.get();
}

void ExtensionContextMenuModelTest::SetUp() {
  ExtensionServiceTestBase::SetUp();
  content::BrowserSideNavigationSetUp();
  scoped_screen_override_ =
      std::make_unique<ScopedScreenOverride>(&test_screen_);
}

void ExtensionContextMenuModelTest::TearDown() {
  content::BrowserSideNavigationTearDown();
  ExtensionServiceTestBase::TearDown();
}

// Tests that applicable menu items are disabled when a ManagementPolicy
// prohibits them.
TEST_F(ExtensionContextMenuModelTest, RequiredInstallationsDisablesItems) {
  InitializeEmptyExtensionService();

  // Test that management policy can determine whether or not policy-installed
  // extensions can be installed/uninstalled.
  const Extension* extension = AddExtension(
      "extension", manifest_keys::kPageAction, Manifest::EXTERNAL_POLICY);

  ExtensionContextMenuModel menu(extension, GetBrowser(),
                                 ExtensionContextMenuModel::VISIBLE, nullptr);

  ExtensionSystem* system = ExtensionSystem::Get(profile());
  system->management_policy()->UnregisterAllProviders();

  // Uninstallation should be, by default, enabled.
  EXPECT_TRUE(menu.IsCommandIdEnabled(ExtensionContextMenuModel::UNINSTALL));
  // Uninstallation should always be visible.
  EXPECT_TRUE(menu.IsCommandIdVisible(ExtensionContextMenuModel::UNINSTALL));

  TestManagementPolicyProvider policy_provider(
      TestManagementPolicyProvider::PROHIBIT_MODIFY_STATUS);
  system->management_policy()->RegisterProvider(&policy_provider);

  // If there's a policy provider that requires the extension stay enabled, then
  // uninstallation should be disabled.
  EXPECT_FALSE(menu.IsCommandIdEnabled(ExtensionContextMenuModel::UNINSTALL));
  int uninstall_index =
      menu.GetIndexOfCommandId(ExtensionContextMenuModel::UNINSTALL);
  // There should also be an icon to visually indicate why uninstallation is
  // forbidden.
  gfx::Image icon;
  EXPECT_TRUE(menu.GetIconAt(uninstall_index, &icon));
  EXPECT_FALSE(icon.IsEmpty());

  // Don't leave |policy_provider| dangling.
  system->management_policy()->UnregisterProvider(&policy_provider);
}

// Tests the context menu for a component extension.
TEST_F(ExtensionContextMenuModelTest, ComponentExtensionContextMenu) {
  InitializeEmptyExtensionService();

  std::string name("component");
  std::unique_ptr<base::DictionaryValue> manifest =
      DictionaryBuilder()
          .Set("name", name)
          .Set("version", "1")
          .Set("manifest_version", 2)
          .Set("browser_action", DictionaryBuilder().Build())
          .Build();

  {
    scoped_refptr<const Extension> extension =
        ExtensionBuilder()
            .SetManifest(base::WrapUnique(manifest->DeepCopy()))
            .SetID(crx_file::id_util::GenerateId("component"))
            .SetLocation(Manifest::COMPONENT)
            .Build();
    service()->AddExtension(extension.get());

    ExtensionContextMenuModel menu(extension.get(), GetBrowser(),
                                   ExtensionContextMenuModel::VISIBLE, nullptr);

    // A component extension's context menu should not include options for
    // managing extensions or removing it, and should only include an option for
    // the options page if the extension has one (which this one doesn't).
    EXPECT_EQ(-1,
              menu.GetIndexOfCommandId(ExtensionContextMenuModel::CONFIGURE));
    EXPECT_EQ(-1,
              menu.GetIndexOfCommandId(ExtensionContextMenuModel::UNINSTALL));
    EXPECT_EQ(-1, menu.GetIndexOfCommandId(ExtensionContextMenuModel::MANAGE));
    // The "name" option should be present, but not enabled for component
    // extensions.
    EXPECT_NE(-1, menu.GetIndexOfCommandId(ExtensionContextMenuModel::NAME));
    EXPECT_FALSE(menu.IsCommandIdEnabled(ExtensionContextMenuModel::NAME));
  }

  {
    // Check that a component extension with an options page does have the
    // options
    // menu item, and it is enabled.
    manifest->SetString("options_page", "options_page.html");
    scoped_refptr<const Extension> extension =
        ExtensionBuilder()
            .SetManifest(std::move(manifest))
            .SetID(crx_file::id_util::GenerateId("component_opts"))
            .SetLocation(Manifest::COMPONENT)
            .Build();
    ExtensionContextMenuModel menu(extension.get(), GetBrowser(),
                                   ExtensionContextMenuModel::VISIBLE, nullptr);
    service()->AddExtension(extension.get());
    EXPECT_TRUE(extensions::OptionsPageInfo::HasOptionsPage(extension.get()));
    EXPECT_NE(-1,
              menu.GetIndexOfCommandId(ExtensionContextMenuModel::CONFIGURE));
    EXPECT_TRUE(menu.IsCommandIdEnabled(ExtensionContextMenuModel::CONFIGURE));
  }
}

TEST_F(ExtensionContextMenuModelTest, ExtensionItemTest) {
  InitializeEmptyExtensionService();
  const Extension* extension =
      AddExtension("extension", manifest_keys::kPageAction, Manifest::INTERNAL);

  // Create a MenuManager for adding context items.
  MenuManager* manager = static_cast<MenuManager*>(
      (MenuManagerFactory::GetInstance()->SetTestingFactoryAndUse(
          profile(),
          &MenuManagerFactory::BuildServiceInstanceForTesting)));
  ASSERT_TRUE(manager);

  MenuBuilder builder(extension, GetBrowser(), manager);

  // There should be no extension items yet.
  EXPECT_EQ(0, CountExtensionItems(*builder.BuildMenu()));

  // Add a browser action menu item.
  builder.AddContextItem(MenuItem::BROWSER_ACTION);
  // Since |extension| has a page action, the browser action menu item should
  // not be present.
  EXPECT_EQ(0, CountExtensionItems(*builder.BuildMenu()));

  // Add a page action menu item. This should be present because |extension|
  // has a page action.
  builder.AddContextItem(MenuItem::PAGE_ACTION);
  EXPECT_EQ(1, CountExtensionItems(*builder.BuildMenu()));

  // Create more page action items to test top-level menu item limitations.
  // We start at 1, so this should try to add the limit + 1.
  for (int i = 0; i < api::context_menus::ACTION_MENU_TOP_LEVEL_LIMIT; ++i)
    builder.AddContextItem(MenuItem::PAGE_ACTION);

  // We shouldn't go above the limit of top-level items.
  EXPECT_EQ(api::context_menus::ACTION_MENU_TOP_LEVEL_LIMIT,
            CountExtensionItems(*builder.BuildMenu()));
}

// Top-level extension actions, like 'browser_action' or 'page_action',
// are subject to an item limit chrome.contextMenus.ACTION_MENU_TOP_LEVEL_LIMIT.
// The test below ensures that:
//
// 1. The limit is respected for top-level items. In this case, we test
//    MenuItem::PAGE_ACTION.
// 2. Adding more items than the limit are ignored; only items within the limit
//    are visible.
// 3. Hiding items within the limit makes "extra" ones visible.
// 4. Unhiding an item within the limit hides a visible "extra" one.
TEST_F(ExtensionContextMenuModelTest,
       TestItemVisibilityAgainstItemLimitForTopLevelItems) {
  InitializeEmptyExtensionService();
  const Extension* extension =
      AddExtension("extension", manifest_keys::kPageAction, Manifest::INTERNAL);

  // Create a MenuManager for adding context items.
  MenuManager* manager = static_cast<MenuManager*>(
      MenuManagerFactory::GetInstance()->SetTestingFactoryAndUse(
          profile(), &MenuManagerFactory::BuildServiceInstanceForTesting));
  ASSERT_TRUE(manager);

  MenuBuilder builder(extension, GetBrowser(), manager);

  // There should be no extension items yet.
  EXPECT_EQ(0, CountExtensionItems(*builder.BuildMenu()));

  // Create more page action items to test top-level menu item limitations.
  for (int i = 1; i <= api::context_menus::ACTION_MENU_TOP_LEVEL_LIMIT; ++i) {
    builder.AddContextItem(MenuItem::PAGE_ACTION);
    builder.SetItemTitle(i, item_label().append(base::StringPrintf("%d", i)));
  }

  // We shouldn't go above the limit of top-level items.
  EXPECT_EQ(api::context_menus::ACTION_MENU_TOP_LEVEL_LIMIT,
            CountExtensionItems(*builder.BuildMenu()));

  // Add three more page actions items. This exceeds the top-level menu item
  // limit, so the three added should not be visible in the menu.
  builder.AddContextItem(MenuItem::PAGE_ACTION);
  builder.SetItemTitle(7, item_label() + "7");

  // By default, the additional page action items have their visibility set to
  // true. Test creating the eigth item such that it is hidden.
  builder.AddContextItem(MenuItem::PAGE_ACTION);
  builder.SetItemTitle(8, item_label() + "8");
  builder.SetItemVisibility(8, false);

  builder.AddContextItem(MenuItem::PAGE_ACTION);
  builder.SetItemTitle(9, item_label() + "9");

  std::unique_ptr<ExtensionContextMenuModel> model = builder.BuildMenu();

  // Ensure that the menu item limit is obeyed, meaning that the three
  // additional items are not visible in the menu.
  EXPECT_EQ(api::context_menus::ACTION_MENU_TOP_LEVEL_LIMIT,
            CountExtensionItems(*model));
  // Items 7 to 9 should not be visible in the model.
  VerifyItems(*model, {"1", "2", "3", "4", "5", "6"});

  // Hide the first two items.
  builder.SetItemVisibility(1, false);
  builder.SetItemVisibility(2, false);
  model = builder.BuildMenu();

  // Ensure that the menu item limit is obeyed.
  EXPECT_EQ(api::context_menus::ACTION_MENU_TOP_LEVEL_LIMIT,
            CountExtensionItems(*model));
  // Hiding the first two items in the model should make visible the "extra"
  // items -- items 7 and 9. Note, item 8 was set to hidden, so it should not
  // show in the model.
  VerifyItems(*model, {"3", "4", "5", "6", "7", "9"});

  // Unhide the eigth item.
  builder.SetItemVisibility(8, true);
  model = builder.BuildMenu();

  // Ensure that the menu item limit is obeyed.
  EXPECT_EQ(api::context_menus::ACTION_MENU_TOP_LEVEL_LIMIT,
            CountExtensionItems(*model));
  // The ninth item should be replaced with the eigth.
  VerifyItems(*model, {"3", "4", "5", "6", "7", "8"});

  // Unhide the first two items.
  builder.SetItemVisibility(1, true);
  builder.SetItemVisibility(2, true);
  model = builder.BuildMenu();

  EXPECT_EQ(api::context_menus::ACTION_MENU_TOP_LEVEL_LIMIT,
            CountExtensionItems(*model));
  // Unhiding the first two items should respect the menu item limit and
  // exclude the "extra" items -- items 7, 8, and 9 -- from the model.
  VerifyItems(*model, {"1", "2", "3", "4", "5", "6"});
}

// Tests that the standard menu items (e.g. uninstall, manage) are always
// visible.
TEST_F(ExtensionContextMenuModelTest,
       ExtensionContextMenuStandardItemsAlwaysVisible) {
  InitializeEmptyExtensionService();
  const Extension* extension =
      AddExtension("extension", manifest_keys::kPageAction, Manifest::INTERNAL);

  ExtensionContextMenuModel menu(extension, GetBrowser(),
                                 ExtensionContextMenuModel::VISIBLE, nullptr);
  EXPECT_TRUE(menu.IsCommandIdVisible(ExtensionContextMenuModel::NAME));
  EXPECT_TRUE(menu.IsCommandIdVisible(ExtensionContextMenuModel::CONFIGURE));
  EXPECT_TRUE(
      menu.IsCommandIdVisible(ExtensionContextMenuModel::TOGGLE_VISIBILITY));
  EXPECT_TRUE(menu.IsCommandIdVisible(ExtensionContextMenuModel::UNINSTALL));
  EXPECT_TRUE(menu.IsCommandIdVisible(ExtensionContextMenuModel::MANAGE));
  EXPECT_TRUE(
      menu.IsCommandIdVisible(ExtensionContextMenuModel::INSPECT_POPUP));
  EXPECT_TRUE(menu.IsCommandIdVisible(
      ExtensionContextMenuModel::PAGE_ACCESS_RUN_ON_CLICK));
  EXPECT_TRUE(menu.IsCommandIdVisible(
      ExtensionContextMenuModel::PAGE_ACCESS_RUN_ON_SITE));
  EXPECT_TRUE(menu.IsCommandIdVisible(
      ExtensionContextMenuModel::PAGE_ACCESS_RUN_ON_ALL_SITES));
}

// Test that the "show" and "hide" menu items appear correctly in the extension
// context menu.
TEST_F(ExtensionContextMenuModelTest, ExtensionContextMenuShowAndHide) {
  InitializeEmptyExtensionService();
  Browser* browser = GetBrowser();
  extension_action_test_util::CreateToolbarModelForProfile(profile());
  const Extension* page_action =
      AddExtension("page_action_extension",
                     manifest_keys::kPageAction,
                     Manifest::INTERNAL);
  const Extension* browser_action =
      AddExtension("browser_action_extension",
                     manifest_keys::kBrowserAction,
                     Manifest::INTERNAL);

  // For laziness.
  const ExtensionContextMenuModel::MenuEntries visibility_command =
      ExtensionContextMenuModel::TOGGLE_VISIBILITY;
  base::string16 hide_string =
      l10n_util::GetStringUTF16(IDS_EXTENSIONS_HIDE_BUTTON_IN_MENU);
  base::string16 show_string =
      l10n_util::GetStringUTF16(IDS_EXTENSIONS_SHOW_BUTTON_IN_TOOLBAR);
  base::string16 keep_string =
      l10n_util::GetStringUTF16(IDS_EXTENSIONS_KEEP_BUTTON_IN_TOOLBAR);

  {
    // Even page actions should have a visibility option.
    ExtensionContextMenuModel menu(page_action, browser,
                                   ExtensionContextMenuModel::VISIBLE, nullptr);
    int index = menu.GetIndexOfCommandId(visibility_command);
    EXPECT_NE(-1, index);
    EXPECT_EQ(hide_string, menu.GetLabelAt(index));
  }

  {
    ExtensionContextMenuModel menu(browser_action, browser,
                                   ExtensionContextMenuModel::VISIBLE, nullptr);
    int index = menu.GetIndexOfCommandId(visibility_command);
    EXPECT_NE(-1, index);
    EXPECT_EQ(hide_string, menu.GetLabelAt(index));

    ExtensionActionAPI* action_api = ExtensionActionAPI::Get(profile());
    EXPECT_TRUE(action_api->GetBrowserActionVisibility(browser_action->id()));
    // Executing the 'hide' command shouldn't modify the prefs (the ordering
    // behavior is tested in ToolbarActionsModel tests).
    // TODO(devlin): We should be able to get rid of the pref.
    menu.ExecuteCommand(visibility_command, 0);
    EXPECT_TRUE(action_api->GetBrowserActionVisibility(browser_action->id()));
  }

  {
    // If the action is overflowed, it should have the "Show button in toolbar"
    // string.
    ExtensionContextMenuModel menu(browser_action, browser,
                                   ExtensionContextMenuModel::OVERFLOWED,
                                   nullptr);
    int index = menu.GetIndexOfCommandId(visibility_command);
    EXPECT_NE(-1, index);
    EXPECT_EQ(show_string, menu.GetLabelAt(index));
  }

  {
    // If the action is transitively visible, as happens when it is showing a
    // popup, we should use a "Keep button in toolbar" string.
    ExtensionContextMenuModel menu(
        browser_action, browser,
        ExtensionContextMenuModel::TRANSITIVELY_VISIBLE, nullptr);
    int index = menu.GetIndexOfCommandId(visibility_command);
    EXPECT_NE(-1, index);
    EXPECT_EQ(keep_string, menu.GetLabelAt(index));
  }
}

TEST_F(ExtensionContextMenuModelTest, ExtensionContextUninstall) {
  InitializeEmptyExtensionService();

  const Extension* extension = AddExtension(
      "extension", manifest_keys::kBrowserAction, Manifest::INTERNAL);
  const std::string extension_id = extension->id();
  ASSERT_TRUE(registry()->enabled_extensions().GetByID(extension_id));

  ScopedTestDialogAutoConfirm auto_confirm(ScopedTestDialogAutoConfirm::ACCEPT);
  TestExtensionRegistryObserver uninstalled_observer(registry());
  {
    // Scope the menu so that it's destroyed during the uninstall process. This
    // reflects what normally happens (Chrome closes the menu when the uninstall
    // dialog shows up).
    ExtensionContextMenuModel menu(extension, GetBrowser(),
                                   ExtensionContextMenuModel::VISIBLE, nullptr);
    menu.ExecuteCommand(ExtensionContextMenuModel::UNINSTALL, 0);
  }
  uninstalled_observer.WaitForExtensionUninstalled();
  EXPECT_FALSE(registry()->GetExtensionById(extension_id,
                                            ExtensionRegistry::EVERYTHING));
}

TEST_F(ExtensionContextMenuModelTest, TestPageAccessSubmenu) {
  // This test relies on the click-to-script feature.
  auto scoped_feature_list = std::make_unique<base::test::ScopedFeatureList>();
  scoped_feature_list->InitAndEnableFeature(features::kRuntimeHostPermissions);
  InitializeEmptyExtensionService();

  // Add an extension with all urls, and withhold permission.
  const Extension* extension =
      AddExtensionWithHostPermission("extension", manifest_keys::kBrowserAction,
                                     Manifest::INTERNAL, "*://*/*");
  ScriptingPermissionsModifier(profile(), extension).SetAllowedOnAllUrls(false);
  EXPECT_TRUE(registry()->enabled_extensions().Contains(extension->id()));

  const GURL kActiveUrl("http://www.example.com/");
  const GURL kOtherUrl("http://www.google.com/");

  // Add a web contents to the browser.
  std::unique_ptr<content::WebContents> contents(
      content::WebContentsTester::CreateTestWebContents(profile(), nullptr));
  content::WebContents* raw_contents = contents.get();
  Browser* browser = GetBrowser();
  browser->tab_strip_model()->AppendWebContents(std::move(contents), true);
  EXPECT_EQ(browser->tab_strip_model()->GetActiveWebContents(), raw_contents);
  content::WebContentsTester* web_contents_tester =
      content::WebContentsTester::For(raw_contents);
  web_contents_tester->NavigateAndCommit(kActiveUrl);

  ExtensionActionRunner* action_runner =
      ExtensionActionRunner::GetForWebContents(raw_contents);
  ASSERT_TRUE(action_runner);

  // Pretend the extension wants to run.
  int run_count = 0;
  base::Closure increment_run_count(base::Bind(&Increment, &run_count));
  action_runner->RequestScriptInjectionForTesting(
      extension, UserScript::DOCUMENT_IDLE, increment_run_count);

  ExtensionContextMenuModel menu(extension, GetBrowser(),
                                 ExtensionContextMenuModel::VISIBLE, nullptr);

  EXPECT_NE(-1, menu.GetIndexOfCommandId(
                    ExtensionContextMenuModel::PAGE_ACCESS_SUBMENU));

  // For laziness.
  const ExtensionContextMenuModel::MenuEntries kRunOnClick =
      ExtensionContextMenuModel::PAGE_ACCESS_RUN_ON_CLICK;
  const ExtensionContextMenuModel::MenuEntries kRunOnSite =
      ExtensionContextMenuModel::PAGE_ACCESS_RUN_ON_SITE;
  const ExtensionContextMenuModel::MenuEntries kRunOnAllSites =
      ExtensionContextMenuModel::PAGE_ACCESS_RUN_ON_ALL_SITES;

  // Initial state: The extension should be in "run on click" mode.
  EXPECT_TRUE(menu.IsCommandIdChecked(kRunOnClick));
  EXPECT_FALSE(menu.IsCommandIdChecked(kRunOnSite));
  EXPECT_FALSE(menu.IsCommandIdChecked(kRunOnAllSites));

  // Initial state: The extension should have all permissions withheld, so
  // shouldn't be allowed to run on the active url or another arbitrary url, and
  // should have withheld permissions.
  ScriptingPermissionsModifier permissions_modifier(profile(), extension);
  EXPECT_FALSE(permissions_modifier.HasGrantedHostPermission(kActiveUrl));
  EXPECT_FALSE(permissions_modifier.HasGrantedHostPermission(kOtherUrl));
  const PermissionsData* permissions = extension->permissions_data();
  EXPECT_FALSE(permissions->withheld_permissions().IsEmpty());

  // Change the mode to be "Run on site".
  menu.ExecuteCommand(kRunOnSite, 0);
  EXPECT_FALSE(menu.IsCommandIdChecked(kRunOnClick));
  EXPECT_TRUE(menu.IsCommandIdChecked(kRunOnSite));
  EXPECT_FALSE(menu.IsCommandIdChecked(kRunOnAllSites));

  // The extension should have access to the active url, but not to another
  // arbitrary url, and the extension should still have withheld permissions.
  EXPECT_TRUE(permissions_modifier.HasGrantedHostPermission(kActiveUrl));
  EXPECT_FALSE(permissions_modifier.HasGrantedHostPermission(kOtherUrl));
  EXPECT_FALSE(permissions->withheld_permissions().IsEmpty());

  // Since the extension has permission, it should have ran.
  EXPECT_EQ(1, run_count);
  EXPECT_FALSE(action_runner->WantsToRun(extension));

  // On another url, the mode should still be run on click.
  web_contents_tester->NavigateAndCommit(kOtherUrl);
  EXPECT_TRUE(menu.IsCommandIdChecked(kRunOnClick));
  EXPECT_FALSE(menu.IsCommandIdChecked(kRunOnSite));
  EXPECT_FALSE(menu.IsCommandIdChecked(kRunOnAllSites));

  // And returning to the first url should return the mode to run on site.
  web_contents_tester->NavigateAndCommit(kActiveUrl);
  EXPECT_FALSE(menu.IsCommandIdChecked(kRunOnClick));
  EXPECT_TRUE(menu.IsCommandIdChecked(kRunOnSite));
  EXPECT_FALSE(menu.IsCommandIdChecked(kRunOnAllSites));

  // Request another run.
  action_runner->RequestScriptInjectionForTesting(
      extension, UserScript::DOCUMENT_IDLE, increment_run_count);

  // Change the mode to be "Run on all sites".
  menu.ExecuteCommand(kRunOnAllSites, 0);
  EXPECT_FALSE(menu.IsCommandIdChecked(kRunOnClick));
  EXPECT_FALSE(menu.IsCommandIdChecked(kRunOnSite));
  EXPECT_TRUE(menu.IsCommandIdChecked(kRunOnAllSites));

  // The extension should be able to run on any url, and shouldn't have any
  // withheld permissions.
  EXPECT_TRUE(permissions_modifier.HasGrantedHostPermission(kActiveUrl));
  EXPECT_TRUE(permissions_modifier.HasGrantedHostPermission(kOtherUrl));
  EXPECT_TRUE(permissions->withheld_permissions().IsEmpty());

  // It should have ran again.
  EXPECT_EQ(2, run_count);
  EXPECT_FALSE(action_runner->WantsToRun(extension));

  // On another url, the mode should also be run on all sites.
  web_contents_tester->NavigateAndCommit(kOtherUrl);
  EXPECT_FALSE(menu.IsCommandIdChecked(kRunOnClick));
  EXPECT_FALSE(menu.IsCommandIdChecked(kRunOnSite));
  EXPECT_TRUE(menu.IsCommandIdChecked(kRunOnAllSites));

  web_contents_tester->NavigateAndCommit(kActiveUrl);
  EXPECT_FALSE(menu.IsCommandIdChecked(kRunOnClick));
  EXPECT_FALSE(menu.IsCommandIdChecked(kRunOnSite));
  EXPECT_TRUE(menu.IsCommandIdChecked(kRunOnAllSites));

  action_runner->RequestScriptInjectionForTesting(
      extension, UserScript::DOCUMENT_IDLE, increment_run_count);

  // Return the mode to "Run on click".
  menu.ExecuteCommand(kRunOnClick, 0);
  EXPECT_TRUE(menu.IsCommandIdChecked(kRunOnClick));
  EXPECT_FALSE(menu.IsCommandIdChecked(kRunOnSite));
  EXPECT_FALSE(menu.IsCommandIdChecked(kRunOnAllSites));

  // We should return to the initial state - no access.
  EXPECT_FALSE(permissions_modifier.HasGrantedHostPermission(kActiveUrl));
  EXPECT_FALSE(permissions_modifier.HasGrantedHostPermission(kOtherUrl));
  EXPECT_FALSE(permissions->withheld_permissions().IsEmpty());

  // And the extension shouldn't have ran.
  EXPECT_EQ(2, run_count);
  EXPECT_TRUE(action_runner->WantsToRun(extension));

  // Install an extension requesting only a single host. Since the extension
  // doesn't request all hosts, it shouldn't have withheld permissions, and
  // thus shouldn't have the page access submenu.
  const Extension* single_host_extension = AddExtensionWithHostPermission(
      "single_host_extension", manifest_keys::kBrowserAction,
      Manifest::INTERNAL, "http://www.google.com/*");
  ExtensionContextMenuModel single_host_menu(
      single_host_extension, GetBrowser(), ExtensionContextMenuModel::VISIBLE,
      nullptr);
  EXPECT_EQ(-1, single_host_menu.GetIndexOfCommandId(
                    ExtensionContextMenuModel::PAGE_ACCESS_SUBMENU));

  // Disable the click-to-script feature, and install a new extension requiring
  // all hosts. Since the feature isn't on, it shouldn't have the page access
  // submenu either.
  scoped_feature_list.reset();  // Need to delete the old list first.
  scoped_feature_list = std::make_unique<base::test::ScopedFeatureList>();
  scoped_feature_list->InitAndDisableFeature(features::kRuntimeHostPermissions);
  const Extension* feature_disabled_extension = AddExtensionWithHostPermission(
      "feature_disabled_extension", manifest_keys::kBrowserAction,
      Manifest::INTERNAL, "http://www.google.com/*");
  ExtensionContextMenuModel feature_disabled_menu(
      feature_disabled_extension, GetBrowser(),
      ExtensionContextMenuModel::VISIBLE, nullptr);
  EXPECT_EQ(-1, feature_disabled_menu.GetIndexOfCommandId(
                    ExtensionContextMenuModel::PAGE_ACCESS_SUBMENU));
  browser->tab_strip_model()->DetachWebContentsAt(
      browser->tab_strip_model()->GetIndexOfWebContents(raw_contents));
}

TEST_F(ExtensionContextMenuModelTest, TestInspectPopupPresence) {
  InitializeEmptyExtensionService();
  {
    const Extension* page_action = AddExtension(
        "page_action", manifest_keys::kPageAction, Manifest::INTERNAL);
    ASSERT_TRUE(page_action);
    ExtensionContextMenuModel menu(page_action, GetBrowser(),
                                   ExtensionContextMenuModel::VISIBLE, nullptr);
    int inspect_popup_index =
        menu.GetIndexOfCommandId(ExtensionContextMenuModel::INSPECT_POPUP);
    EXPECT_GE(0, inspect_popup_index);
  }
  {
    const Extension* browser_action = AddExtension(
        "browser_action", manifest_keys::kBrowserAction, Manifest::INTERNAL);
    ExtensionContextMenuModel menu(browser_action, GetBrowser(),
                                   ExtensionContextMenuModel::VISIBLE, nullptr);
    int inspect_popup_index =
        menu.GetIndexOfCommandId(ExtensionContextMenuModel::INSPECT_POPUP);
    EXPECT_GE(0, inspect_popup_index);
  }
  {
    // An extension with no specified action has one synthesized. However,
    // there will never be a popup to inspect, so we shouldn't add a menu item.
    const Extension* no_action = AddExtension(
        "no_action", nullptr, Manifest::INTERNAL);
    ExtensionContextMenuModel menu(no_action, GetBrowser(),
                                   ExtensionContextMenuModel::VISIBLE, nullptr);
    int inspect_popup_index =
        menu.GetIndexOfCommandId(ExtensionContextMenuModel::INSPECT_POPUP);
    EXPECT_EQ(-1, inspect_popup_index);
  }
}

}  // namespace extensions
