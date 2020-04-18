// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/** @fileoverview Runs the Polymer Settings tests. */

/** @const {string} Path to source root. */
var ROOT_PATH = '../../../../../';

// Polymer BrowserTest fixture.
GEN_INCLUDE(
    [ROOT_PATH + 'chrome/test/data/webui/polymer_browser_test_base.js']);
GEN('#include "chrome/browser/ui/webui/extensions/' +
    'extension_settings_browsertest.h"');
GEN('#include "chrome/common/chrome_features.h"');

/**
 * Basic test fixture for the MD chrome://extensions page. Installs no
 * extensions.
 */
var CrExtensionsBrowserTest = class extends PolymerTest {
  /** @override */
  get browsePreload() {
    return 'chrome://extensions/';
  }

  /** @override */
  get featureList() {
    return ['features::kMaterialDesignExtensions', ''];
  }

  /** @override */
  get extraLibraries() {
    return PolymerTest.getLibraries(ROOT_PATH).concat([
      ROOT_PATH + 'ui/webui/resources/js/assert.js',
      'extension_test_util.js',
      '../mock_controller.js',
      '../../../../../ui/webui/resources/js/promise_resolver.js',
      '../../../../../ui/webui/resources/js/webui_resource_test.js',
      '../fake_chrome_event.js',
      '../settings/test_util.js',
      '../test_browser_proxy.js',
      'test_service.js',
    ]);
  }

  /** @override */
  get typedefCppFixture() {
    return 'ExtensionSettingsUIBrowserTest';
  }

  // The name of the mocha suite. Should be overriden by subclasses.
  get suiteName() {
    return null;
  }

  /** @param {string} testName The name of the test to run. */
  runMochaTest(testName) {
    runMochaTest(this.suiteName, testName);
  }
};

/**
 * Test fixture with one installed extension.
 */
var CrExtensionsBrowserTestWithInstalledExtension =
    class extends CrExtensionsBrowserTest {
  /** @override */
  testGenPreamble() {
    GEN('  InstallGoodExtension();');
    GEN('  SetAutoConfirmUninstall();');
  }
};

////////////////////////////////////////////////////////////////////////////////
// Extension Sidebar Tests

var CrExtensionsSidebarTest = class extends CrExtensionsBrowserTest {
  /** @override */
  get browsePreload() {
    return 'chrome://extensions/sidebar.html';
  }

  /** @override */
  get extraLibraries() {
    return super.extraLibraries.concat([
      'extension_sidebar_test.js',
    ]);
  }

  /** @override */
  get suiteName() {
    return extension_sidebar_tests.suiteName;
  }
};

TEST_F('CrExtensionsSidebarTest', 'LayoutAndClickHandlers', function() {
  this.runMochaTest(extension_sidebar_tests.TestNames.LayoutAndClickHandlers);
});

TEST_F('CrExtensionsSidebarTest', 'SetSelected', function() {
  this.runMochaTest(extension_sidebar_tests.TestNames.SetSelected);
});

////////////////////////////////////////////////////////////////////////////////
// Extension Toolbar Tests

var CrExtensionsToolbarTest = class extends CrExtensionsBrowserTest {
  /** @override */
  get extraLibraries() {
    return super.extraLibraries.concat([
      'extension_toolbar_test.js',
    ]);
  }

  /** @override */
  get suiteName() {
    return extension_toolbar_tests.suiteName;
  }
};

TEST_F('CrExtensionsToolbarTest', 'Layout', function() {
  this.runMochaTest(extension_toolbar_tests.TestNames.Layout);
});

TEST_F('CrExtensionsToolbarTest', 'DevModeToggle', function() {
  this.runMochaTest(extension_toolbar_tests.TestNames.DevModeToggle);
});

TEST_F('CrExtensionsToolbarTest', 'ClickHandlers', function() {
  this.runMochaTest(extension_toolbar_tests.TestNames.ClickHandlers);
});

GEN('#if defined(OS_CHROMEOS)');
TEST_F('CrExtensionsToolbarTest', 'KioskMode', function() {
  this.runMochaTest(extension_toolbar_tests.TestNames.KioskMode);
});
GEN('#endif');

////////////////////////////////////////////////////////////////////////////////
// Extension Item Tests

var CrExtensionsItemsTest = class extends CrExtensionsBrowserTest {
  get browsePreload() {
    return 'chrome://extensions/item.html';
  }

  /** @override */
  get extraLibraries() {
    return super.extraLibraries.concat([
      'extension_item_test.js',
    ]);
  }

  /** @override */
  get suiteName() {
    return extension_item_tests.suiteName;
  }
};

TEST_F('CrExtensionsItemsTest', 'NormalState', function() {
  this.runMochaTest(
      extension_item_tests.TestNames.ElementVisibilityNormalState);
});

TEST_F('CrExtensionsItemsTest', 'DeveloperState', function() {
  this.runMochaTest(
      extension_item_tests.TestNames.ElementVisibilityDeveloperState);
});

TEST_F('CrExtensionsItemsTest', 'ClickableItems', function() {
  this.runMochaTest(extension_item_tests.TestNames.ClickableItems);
});

TEST_F('CrExtensionsItemsTest', 'FailedReloadFiresLoadError', function() {
  this.runMochaTest(extension_item_tests.TestNames.FailedReloadFiresLoadError);
});

TEST_F('CrExtensionsItemsTest', 'Warnings', function() {
  this.runMochaTest(extension_item_tests.TestNames.Warnings);
});

TEST_F('CrExtensionsItemsTest', 'SourceIndicator', function() {
  this.runMochaTest(extension_item_tests.TestNames.SourceIndicator);
});

TEST_F('CrExtensionsItemsTest', 'EnableToggle', function() {
  this.runMochaTest(extension_item_tests.TestNames.EnableToggle);
});

TEST_F('CrExtensionsItemsTest', 'RemoveButton', function() {
  this.runMochaTest(extension_item_tests.TestNames.RemoveButton);
});

TEST_F('CrExtensionsItemsTest', 'HtmlInName', function() {
  this.runMochaTest(extension_item_tests.TestNames.HtmlInName);
});

////////////////////////////////////////////////////////////////////////////////
// Extension Detail View Tests

var CrExtensionsDetailViewTest = class extends CrExtensionsBrowserTest {
  /** @override */
  get browsePreload() {
    return 'chrome://extensions/detail_view.html';
  }

  /** @override */
  get extraLibraries() {
    return super.extraLibraries.concat([
      'extension_detail_view_test.js',
    ]);
  }

  /** @override */
  get suiteName() {
    return extension_detail_view_tests.suiteName;
  }
};

TEST_F('CrExtensionsDetailViewTest', 'Layout', function() {
  this.runMochaTest(extension_detail_view_tests.TestNames.Layout);
});

TEST_F('CrExtensionsDetailViewTest', 'LayoutSource', function() {
  this.runMochaTest(extension_detail_view_tests.TestNames.LayoutSource);
});

TEST_F('CrExtensionsDetailViewTest', 'ClickableElements', function() {
  this.runMochaTest(extension_detail_view_tests.TestNames.ClickableElements);
});

TEST_F('CrExtensionsDetailViewTest', 'IndicatorTest', function() {
  this.runMochaTest(extension_detail_view_tests.TestNames.Indicator);
});

TEST_F('CrExtensionsDetailViewTest', 'Warnings', function() {
  this.runMochaTest(extension_detail_view_tests.TestNames.Warnings);
});

////////////////////////////////////////////////////////////////////////////////
// Extension Item List Tests

var CrExtensionsItemListTest = class extends CrExtensionsBrowserTest {
  /** @override */
  get browsePreload() {
    return 'chrome://extensions/item_list.html';
  }

  /** @override */
  get extraLibraries() {
    return super.extraLibraries.concat([
      'extension_item_list_test.js',
    ]);
  }

  /** @override */
  get suiteName() {
    return extension_item_list_tests.suiteName;
  }
};

// This test is flaky on Mac10.9 Tests (dbg). See https://crbug.com/771099.
GEN('#if defined(OS_MACOSX)');
GEN('#define MAYBE_Filtering DISABLED_Filtering');
GEN('#else');
GEN('#define MAYBE_Filtering Filtering');
GEN('#endif');
TEST_F('CrExtensionsItemListTest', 'MAYBE_Filtering', function() {
  this.runMochaTest(extension_item_list_tests.TestNames.Filtering);
});

// This test is flaky on Mac10.9 Tests (dbg). See https://crbug.com/771099.
GEN('#if defined(OS_MACOSX)');
GEN('#define MAYBE_NoItems DISABLED_NoItems');
GEN('#else');
GEN('#define MAYBE_NoItems NoItems');
GEN('#endif');
TEST_F('CrExtensionsItemListTest', 'MAYBE_NoItems', function() {
  this.runMochaTest(extension_item_list_tests.TestNames.NoItemsMsg);
});

TEST_F('CrExtensionsItemListTest', 'NoSearchResults', function() {
  this.runMochaTest(extension_item_list_tests.TestNames.NoSearchResultsMsg);
});

////////////////////////////////////////////////////////////////////////////////
// Extension Load Error Tests

var CrExtensionsLoadErrorTest = class extends CrExtensionsBrowserTest {
  /** @override */
  get extraLibraries() {
    return super.extraLibraries.concat([
      'extension_load_error_test.js',
    ]);
  }

  /** @override */
  get suiteName() {
    return extension_load_error_tests.suiteName;
  }
};

TEST_F('CrExtensionsLoadErrorTest', 'RetryError', function() {
  this.runMochaTest(extension_load_error_tests.TestNames.RetryError);
});

TEST_F('CrExtensionsLoadErrorTest', 'RetrySuccess', function() {
  this.runMochaTest(extension_load_error_tests.TestNames.RetrySuccess);
});

TEST_F('CrExtensionsLoadErrorTest', 'CodeSection', function() {
  this.runMochaTest(extension_load_error_tests.TestNames.CodeSection);
});

////////////////////////////////////////////////////////////////////////////////
// Extension Manager Tests

var CrExtensionsManagerUnitTest = class extends CrExtensionsBrowserTest {
  /** @override */
  get browsePreload() {
    return 'chrome://extensions/manager.html';
  }

  /** @override */
  get extraLibraries() {
    return super.extraLibraries.concat([
      'test_kiosk_browser_proxy.js',
      'extension_manager_unit_test.js',
    ]);
  }

  /** @override */
  get suiteName() {
    return extension_manager_tests.suiteName;
  }
};

TEST_F('CrExtensionsManagerUnitTest', 'ItemOrder', function() {
  this.runMochaTest(extension_manager_tests.TestNames.ItemOrder);
});

TEST_F('CrExtensionsManagerUnitTest', 'UpdateItemData', function() {
  this.runMochaTest(extension_manager_tests.TestNames.UpdateItemData);
});

TEST_F('CrExtensionsManagerUnitTest', 'ProfileSettings', function() {
  this.runMochaTest(extension_manager_tests.TestNames.ProfileSettings);
});

TEST_F('CrExtensionsManagerUnitTest', 'Uninstall', function() {
  this.runMochaTest(extension_manager_tests.TestNames.Uninstall);
});

TEST_F('CrExtensionsManagerUnitTest', 'UninstallFromDetails', function() {
  this.runMochaTest(extension_manager_tests.TestNames.UninstallFromDetails);
});

TEST_F('CrExtensionsManagerUnitTest', 'ToggleIncognito', function() {
  this.runMochaTest(extension_manager_tests.TestNames.ToggleIncognitoMode);
});

TEST_F('CrExtensionsManagerUnitTest', 'EnableAndDisable', function() {
  this.runMochaTest(extension_manager_tests.TestNames.EnableAndDisable);
});

GEN('#if defined(OS_CHROMEOS)');
TEST_F('CrExtensionsManagerUnitTest', 'KioskMode', function() {
  this.runMochaTest(extension_manager_tests.TestNames.KioskMode);
});
GEN('#endif');


var CrExtensionsManagerTestWithMultipleExtensionTypesInstalled =
    class extends CrExtensionsBrowserTest {
  /** @override */
  get extraLibraries() {
    return super.extraLibraries.concat([
      'extension_manager_test.js',
    ]);
  }

  /** @override */
  testGenPreamble() {
    GEN('  InstallGoodExtension();');
    GEN('  InstallPackagedApp();');
    GEN('  InstallHostedApp();');
    GEN('  InstallPlatformApp();');
  }

  /** @override */
  get suiteName() {
    return extension_manager_tests.suiteName;
  }
};

TEST_F(
    'CrExtensionsManagerTestWithMultipleExtensionTypesInstalled',
    'ItemListVisibility', function() {
      this.runMochaTest(extension_manager_tests.TestNames.ItemListVisibility);
    });

TEST_F(
    'CrExtensionsManagerTestWithMultipleExtensionTypesInstalled', 'SplitItems',
    function() {
      this.runMochaTest(extension_manager_tests.TestNames.SplitItems);
    });

TEST_F(
    'CrExtensionsManagerTestWithMultipleExtensionTypesInstalled', 'ChangePages',
    function() {
      this.runMochaTest(extension_manager_tests.TestNames.ChangePages);
    });

var CrExtensionsManagerTestWithIdQueryParam =
    class extends CrExtensionsBrowserTestWithInstalledExtension {
  /** @override */
  get browsePreload() {
    return 'chrome://extensions/?id=ldnnhddmnhbkjipkidpdiheffobcpfmf';
  }

  /** @override */
  get extraLibraries() {
    return super.extraLibraries.concat([
      'extension_manager_test.js',
    ]);
  }

  /** @override */
  get suiteName() {
    return extension_manager_tests.suiteName;
  }
};

TEST_F(
    'CrExtensionsManagerTestWithIdQueryParam', 'NavigationToDetails',
    function() {
      this.runMochaTest(
          extension_manager_tests.TestNames.UrlNavigationToDetails);
    });


////////////////////////////////////////////////////////////////////////////////
// Extension Keyboard Shortcuts Tests

var CrExtensionsShortcutTest = class extends CrExtensionsBrowserTest {
  /** @override */
  get browsePreload() {
    return 'chrome://extensions/keyboard_shortcuts.html';
  }

  /** @override */
  get extraLibraries() {
    return super.extraLibraries.concat([
      'extension_keyboard_shortcuts_test.js',
    ]);
  }

  /** @override */
  get suiteName() {
    return extension_shortcut_tests.suiteName;
  }
};

TEST_F('CrExtensionsShortcutTest', 'Layout', function() {
  this.runMochaTest(extension_shortcut_tests.TestNames.Layout);
});

TEST_F('CrExtensionsShortcutTest', 'IsValidKeyCode', function() {
  this.runMochaTest(extension_shortcut_tests.TestNames.IsValidKeyCode);
});

TEST_F('CrExtensionsShortcutTest', 'KeyStrokeToString', function() {
  this.runMochaTest(extension_shortcut_tests.TestNames.IsValidKeyCode);
});

TEST_F('CrExtensionsShortcutTest', 'ScopeChange', function() {
  this.runMochaTest(extension_shortcut_tests.TestNames.ScopeChange);
});

var CrExtensionsShortcutInputTest = class extends CrExtensionsBrowserTest {
  /** @override */
  get browsePreload() {
    return 'chrome://extensions/keyboard_shortcuts.html';
  }

  /** @override */
  get extraLibraries() {
    return super.extraLibraries.concat([
      'extension_shortcut_input_test.js',
    ]);
  }

  /** @override */
  get suiteName() {
    return extension_shortcut_input_tests.suiteName;
  }
};

TEST_F('CrExtensionsShortcutInputTest', 'Basic', function() {
  this.runMochaTest(extension_shortcut_input_tests.TestNames.Basic);
});

////////////////////////////////////////////////////////////////////////////////
// Extension Pack Dialog Tests

var CrExtensionsPackDialogTest = class extends CrExtensionsBrowserTest {
  /** @override */
  get extraLibraries() {
    return super.extraLibraries.concat([
      'extension_pack_dialog_test.js',
    ]);
  }

  /** @override */
  get suiteName() {
    return extension_pack_dialog_tests.suiteName;
  }
};

TEST_F('CrExtensionsPackDialogTest', 'Interaction', function() {
  this.runMochaTest(extension_pack_dialog_tests.TestNames.Interaction);
});

// Disabling on Windows due to flaky timeout on some build bots.
// http://crbug.com/832885
GEN('#if defined(OS_WIN)');
GEN('#define MAYBE_PackSuccess DISABLED_PackSuccess');
GEN('#else');
GEN('#define MAYBE_PackSuccess PackSuccess');
GEN('#endif');
TEST_F('CrExtensionsPackDialogTest', 'MAYBE_PackSuccess', function() {
  this.runMochaTest(extension_pack_dialog_tests.TestNames.PackSuccess);
});

TEST_F('CrExtensionsPackDialogTest', 'PackError', function() {
  this.runMochaTest(extension_pack_dialog_tests.TestNames.PackError);
});

TEST_F('CrExtensionsPackDialogTest', 'PackWarning', function() {
  this.runMochaTest(extension_pack_dialog_tests.TestNames.PackWarning);
});

////////////////////////////////////////////////////////////////////////////////
// Extension Options Dialog Tests

var CrExtensionsOptionsDialogTest = class extends CrExtensionsBrowserTest {
  /** @override */
  testGenPreamble() {
    GEN('  InstallExtensionWithInPageOptions();');
  }

  /** @override */
  get extraLibraries() {
    return super.extraLibraries.concat([
      '../settings/test_util.js',
      'extension_options_dialog_test.js',
    ]);
  }

  /** @override */
  get suiteName() {
    return extension_options_dialog_tests.suiteName;
  }
};

TEST_F('CrExtensionsOptionsDialogTest', 'Layout', function() {
  this.runMochaTest(extension_options_dialog_tests.TestNames.Layout);
});

////////////////////////////////////////////////////////////////////////////////
// Extension Error Page Tests

var CrExtensionsErrorPageTest = class extends CrExtensionsBrowserTest {
  /** @override */
  get browsePreload() {
    return 'chrome://extensions/error_page.html';
  }

  /** @override */
  get extraLibraries() {
    return super.extraLibraries.concat([
      'extension_error_page_test.js',
    ]);
  }

  /** @override */
  get suiteName() {
    return extension_error_page_tests.suiteName;
  }
};

TEST_F('CrExtensionsErrorPageTest', 'Layout', function() {
  this.runMochaTest(extension_error_page_tests.TestNames.Layout);
});

TEST_F('CrExtensionsErrorPageTest', 'CodeSection', function() {
  this.runMochaTest(extension_error_page_tests.TestNames.CodeSection);
});

TEST_F('CrExtensionsErrorPageTest', 'ErrorSelection', function() {
  this.runMochaTest(extension_error_page_tests.TestNames.ErrorSelection);
});

////////////////////////////////////////////////////////////////////////////////
// Extension Code Section Tests

var CrExtensionsCodeSectionTest = class extends CrExtensionsBrowserTest {
  /** @override */
  get extraLibraries() {
    return super.extraLibraries.concat([
      'extension_code_section_test.js',
    ]);
  }

  /** @override */
  get suiteName() {
    return extension_code_section_tests.suiteName;
  }
};

TEST_F('CrExtensionsCodeSectionTest', 'Layout', function() {
  this.runMochaTest(extension_code_section_tests.TestNames.Layout);
});

TEST_F('CrExtensionsCodeSectionTest', 'LongSource', function() {
  this.runMochaTest(extension_code_section_tests.TestNames.LongSource);
});

////////////////////////////////////////////////////////////////////////////////
// Extension Navigation Helper Tests

var CrExtensionsNavigationHelperTest = class extends CrExtensionsBrowserTest {
  /** @override */
  get browsePreload() {
    return 'chrome://extensions/navigation_helper.html';
  }

  /** @override */
  get extraLibraries() {
    return super.extraLibraries.concat([
      'extension_navigation_helper_test.js',
    ]);
  }

  /** @override */
  get suiteName() {
    return extension_navigation_helper_tests.suiteName;
  }
};

TEST_F('CrExtensionsNavigationHelperTest', 'Basic', function() {
  this.runMochaTest(extension_navigation_helper_tests.TestNames.Basic);
});

TEST_F('CrExtensionsNavigationHelperTest', 'Conversion', function() {
  this.runMochaTest(extension_navigation_helper_tests.TestNames.Conversions);
});

TEST_F('CrExtensionsNavigationHelperTest', 'PushAndReplaceState', function() {
  this.runMochaTest(
      extension_navigation_helper_tests.TestNames.PushAndReplaceState);
});

TEST_F('CrExtensionsNavigationHelperTest', 'SupportedRoutes', function() {
  this.runMochaTest(
      extension_navigation_helper_tests.TestNames.SupportedRoutes);
});

////////////////////////////////////////////////////////////////////////////////
// Extension View Manager Tests

var CrExtensionsViewManagerTest = class extends CrExtensionsBrowserTest {
  /** @override */
  get browsePreload() {
    return 'chrome://extensions/view_manager.html';
  }

  /** @override */
  get extraLibraries() {
    return super.extraLibraries.concat([
      'extension_view_manager_test.js',
    ]);
  }

  /** @override */
  get suiteName() {
    return extension_view_manager_tests.suiteName;
  }
};

TEST_F('CrExtensionsViewManagerTest', 'VisibilityTest', function() {
  this.runMochaTest(extension_view_manager_tests.TestNames.Visibility);
});

TEST_F('CrExtensionsViewManagerTest', 'EventFiringTest', function() {
  this.runMochaTest(extension_view_manager_tests.TestNames.EventFiring);
});

////////////////////////////////////////////////////////////////////////////////
// Error Console tests

var CrExtensionsErrorConsoleTest = class extends CrExtensionsBrowserTest {
  /** @override */
  get suiteName() {
    return 'ErrorConsoleTests';
  }

  /** @override */
  get browsePreload() {
    return 'chrome://extensions/?errors=oehidglfoeondlkoeloailjdmmghacge';
  }

  /** @override */
  testGenPreamble() {
    GEN('  SetDevModeEnabled(true);');
    GEN('  EnableErrorConsole();');
    GEN('  InstallErrorsExtension();');
  }

  /** @override */
  testGenPostamble() {
    GEN('  SetDevModeEnabled(false);');  // Return this to default.
  }
};

TEST_F('CrExtensionsErrorConsoleTest', 'TestUpDownErrors', function() {
  const STACK_ERRORS = '* /deep/ li';
  const ACTIVE_ERROR_IN_STACK = '* /deep/ li[tabindex="0"]';

  let initialFocus = document.querySelector(ACTIVE_ERROR_IN_STACK);
  assertTrue(!!initialFocus);
  assertEquals(1, document.querySelectorAll(ACTIVE_ERROR_IN_STACK).length);
  assertEquals(4, document.querySelectorAll(STACK_ERRORS).length);

  // Pressing up when the first item is focused should NOT change focus.
  MockInteractions.keyDownOn(initialFocus, 38, '', 'ArrowUp');
  assertEquals(initialFocus, document.querySelector(ACTIVE_ERROR_IN_STACK));

  // Pressing down when the first item is focused should change focus.
  MockInteractions.keyDownOn(initialFocus, 40, '', 'ArrowDown');
  assertNotEquals(initialFocus, document.querySelector(ACTIVE_ERROR_IN_STACK));

  // Pressing up when the second item is focused should focus the first again.
  MockInteractions.keyDownOn(initialFocus, 38, '', 'ArrowUp');
  assertEquals(initialFocus, document.querySelector(ACTIVE_ERROR_IN_STACK));

  testDone();
});

////////////////////////////////////////////////////////////////////////////////
// extensions-toggle-row tests.

var CrExtensionsToggleRowTest = class extends CrExtensionsBrowserTest {
  /** @override */
  get browsePreload() {
    return 'chrome://extensions/toggle_row.html';
  }

  /** @override */
  get extraLibraries() {
    return super.extraLibraries.concat([
      'toggle_row_test.js',
    ]);
  }
};

TEST_F('CrExtensionsToggleRowTest', 'ToggleRowTest', function() {
  mocha.run();
});

////////////////////////////////////////////////////////////////////////////////
// kiosk mode tests.

GEN('#if defined(OS_CHROMEOS)');

var CrExtensionsKioskModeTest = class extends CrExtensionsBrowserTest {
  /** @override */
  get browsePreload() {
    return 'chrome://extensions/kiosk_dialog.html';
  }

  /** @override */
  get extraLibraries() {
    return super.extraLibraries.concat([
      'test_kiosk_browser_proxy.js',
      'extension_kiosk_mode_test.js',
    ]);
  }
  /** @override */
  get suiteName() {
    return extension_kiosk_mode_tests.suiteName;
  }
};

TEST_F('CrExtensionsKioskModeTest', 'AddButton', function() {
  this.runMochaTest(extension_kiosk_mode_tests.TestNames.AddButton);
});

TEST_F('CrExtensionsKioskModeTest', 'Layout', function() {
  this.runMochaTest(extension_kiosk_mode_tests.TestNames.Layout);
});

TEST_F('CrExtensionsKioskModeTest', 'AutoLaunch', function() {
  this.runMochaTest(extension_kiosk_mode_tests.TestNames.AutoLaunch);
});

TEST_F('CrExtensionsKioskModeTest', 'Bailout', function() {
  this.runMochaTest(extension_kiosk_mode_tests.TestNames.Bailout);
});

TEST_F('CrExtensionsKioskModeTest', 'Updated', function() {
  this.runMochaTest(extension_kiosk_mode_tests.TestNames.Updated);
});

TEST_F('CrExtensionsKioskModeTest', 'AddError', function() {
  this.runMochaTest(extension_kiosk_mode_tests.TestNames.AddError);
});

GEN('#endif');
