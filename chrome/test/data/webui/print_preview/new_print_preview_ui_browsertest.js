// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/** @fileoverview Runs the Print Preview tests for the new UI. */

const ROOT_PATH = '../../../../../';

GEN_INCLUDE(
    [ROOT_PATH + 'chrome/test/data/webui/polymer_browser_test_base.js']);
GEN('#include "chrome/common/chrome_features.h"');

function PrintPreviewSettingsSectionsTest() {}

const NewPrintPreviewTest = class extends PolymerTest {
  /** @override */
  get browsePreload() {
    return 'chrome://print/';
  }

  /** @override */
  get featureList() {
    return ['features::kNewPrintPreview', ''];
  }

  /** @override */
  get extraLibraries() {
    return PolymerTest.getLibraries(ROOT_PATH).concat([
      ROOT_PATH + 'ui/webui/resources/js/assert.js',
    ]);
  }

  // The name of the mocha suite. Should be overridden by subclasses.
  get suiteName() {
    return null;
  }

  /** @param {string} testName The name of the test to run. */
  runMochaTest(testName) {
    runMochaTest(this.suiteName, testName);
  }
};

PrintPreviewSettingsSectionsTest = class extends NewPrintPreviewTest {
  /** @override */
  get browsePreload() {
    return 'chrome://print/new/app.html';
  }

  /** @override */
  get extraLibraries() {
    return super.extraLibraries.concat([
      ROOT_PATH + 'chrome/test/data/webui/settings/test_util.js',
      '../test_browser_proxy.js',
      'native_layer_stub.js',
      'plugin_stub.js',
      'print_preview_test_utils.js',
      'settings_section_test.js',
    ]);
  }

  /** @override */
  get suiteName() {
    return settings_sections_tests.suiteName;
  }
};

TEST_F('PrintPreviewSettingsSectionsTest', 'Copies', function() {
  this.runMochaTest(settings_sections_tests.TestNames.Copies);
});

TEST_F('PrintPreviewSettingsSectionsTest', 'Layout', function() {
  this.runMochaTest(settings_sections_tests.TestNames.Layout);
});

TEST_F('PrintPreviewSettingsSectionsTest', 'Color', function() {
  this.runMochaTest(settings_sections_tests.TestNames.Color);
});

TEST_F('PrintPreviewSettingsSectionsTest', 'MediaSize', function() {
  this.runMochaTest(settings_sections_tests.TestNames.MediaSize);
});

TEST_F('PrintPreviewSettingsSectionsTest', 'MediaSizeCustomNames', function() {
  this.runMochaTest(settings_sections_tests.TestNames.MediaSizeCustomNames);
});

TEST_F('PrintPreviewSettingsSectionsTest', 'Margins', function() {
  this.runMochaTest(settings_sections_tests.TestNames.Margins);
});

TEST_F('PrintPreviewSettingsSectionsTest', 'Dpi', function() {
  this.runMochaTest(settings_sections_tests.TestNames.Dpi);
});

TEST_F('PrintPreviewSettingsSectionsTest', 'Scaling', function() {
  this.runMochaTest(settings_sections_tests.TestNames.Scaling);
});

TEST_F('PrintPreviewSettingsSectionsTest', 'Other', function() {
  this.runMochaTest(settings_sections_tests.TestNames.Other);
});

TEST_F('PrintPreviewSettingsSectionsTest', 'HeaderFooter', function() {
  this.runMochaTest(settings_sections_tests.TestNames.HeaderFooter);
});

TEST_F('PrintPreviewSettingsSectionsTest', 'SetPages', function() {
  this.runMochaTest(settings_sections_tests.TestNames.SetPages);
});

TEST_F('PrintPreviewSettingsSectionsTest', 'SetCopies', function() {
  this.runMochaTest(settings_sections_tests.TestNames.SetCopies);
});

TEST_F('PrintPreviewSettingsSectionsTest', 'SetLayout', function() {
  this.runMochaTest(settings_sections_tests.TestNames.SetLayout);
});

TEST_F('PrintPreviewSettingsSectionsTest', 'SetColor', function() {
  this.runMochaTest(settings_sections_tests.TestNames.SetColor);
});

TEST_F('PrintPreviewSettingsSectionsTest', 'SetMediaSize', function() {
  this.runMochaTest(settings_sections_tests.TestNames.SetMediaSize);
});

TEST_F('PrintPreviewSettingsSectionsTest', 'SetDpi', function() {
  this.runMochaTest(settings_sections_tests.TestNames.SetDpi);
});

TEST_F('PrintPreviewSettingsSectionsTest', 'SetMargins', function() {
  this.runMochaTest(settings_sections_tests.TestNames.SetMargins);
});

TEST_F('PrintPreviewSettingsSectionsTest', 'SetPagesPerSheet', function() {
  loadTimeData.overrideValues({pagesPerSheetEnabled: true});
  this.runMochaTest(settings_sections_tests.TestNames.SetPagesPerSheet);
});

TEST_F('PrintPreviewSettingsSectionsTest', 'SetScaling', function() {
  this.runMochaTest(settings_sections_tests.TestNames.SetScaling);
});

TEST_F('PrintPreviewSettingsSectionsTest', 'SetOther', function() {
  this.runMochaTest(settings_sections_tests.TestNames.SetOther);
});

TEST_F('PrintPreviewSettingsSectionsTest', 'PresetCopies', function() {
  this.runMochaTest(settings_sections_tests.TestNames.PresetCopies);
});

TEST_F('PrintPreviewSettingsSectionsTest', 'PresetDuplex', function() {
  this.runMochaTest(settings_sections_tests.TestNames.PresetDuplex);
});

PrintPreviewSettingsSelectTest = class extends NewPrintPreviewTest {
  /** @override */
  get browsePreload() {
    return 'chrome://print/new/settings_select.html';
  }

  /** @override */
  get extraLibraries() {
    return super.extraLibraries.concat([
      'print_preview_test_utils.js',
      'settings_select_test.js',
    ]);
  }

  /** @override */
  get suiteName() {
    return settings_select_test.suiteName;
  }
};

TEST_F('PrintPreviewSettingsSelectTest', 'CustomMediaNames', function() {
  this.runMochaTest(settings_select_test.TestNames.CustomMediaNames);
});

PrintPreviewPagesSettingsTest = class extends NewPrintPreviewTest {
  /** @override */
  get browsePreload() {
    return 'chrome://print/new/pages_settings.html';
  }

  /** @override */
  get extraLibraries() {
    return super.extraLibraries.concat([
      ROOT_PATH + 'chrome/test/data/webui/settings/test_util.js',
      'print_preview_test_utils.js',
      'pages_settings_test.js',
    ]);
  }

  /** @override */
  get suiteName() {
    return pages_settings_test.suiteName;
  }
};

TEST_F('PrintPreviewPagesSettingsTest', 'ValidPageRanges', function() {
  this.runMochaTest(pages_settings_test.TestNames.ValidPageRanges);
});

TEST_F('PrintPreviewPagesSettingsTest', 'InvalidPageRanges', function() {
  this.runMochaTest(pages_settings_test.TestNames.InvalidPageRanges);
});

PrintPreviewRestoreStateTest = class extends NewPrintPreviewTest {
  /** @override */
  get browsePreload() {
    return 'chrome://print/new/app.html';
  }

  /** @override */
  get extraLibraries() {
    return super.extraLibraries.concat([
      '../test_browser_proxy.js',
      'native_layer_stub.js',
      'plugin_stub.js',
      'print_preview_test_utils.js',
      'restore_state_test.js',
    ]);
  }

  /** @override */
  get suiteName() {
    return restore_state_test.suiteName;
  }
};

TEST_F('PrintPreviewRestoreStateTest', 'RestoreTrueValues', function() {
  this.runMochaTest(restore_state_test.TestNames.RestoreTrueValues);
});

TEST_F('PrintPreviewRestoreStateTest', 'RestoreFalseValues', function() {
  this.runMochaTest(restore_state_test.TestNames.RestoreFalseValues);
});

TEST_F('PrintPreviewRestoreStateTest', 'SaveValues', function() {
  this.runMochaTest(restore_state_test.TestNames.SaveValues);
});

PrintPreviewModelTest = class extends NewPrintPreviewTest {
  /** @override */
  get browsePreload() {
    return 'chrome://print/new/model.html';
  }

  /** @override */
  get extraLibraries() {
    return super.extraLibraries.concat([
      '../settings/test_util.js',
      'model_test.js',
    ]);
  }

  /** @override */
  get suiteName() {
    return model_test.suiteName;
  }
};

TEST_F('PrintPreviewModelTest', 'SetStickySettings', function() {
  this.runMochaTest(model_test.TestNames.SetStickySettings);
});

PrintPreviewPreviewGenerationTest = class extends NewPrintPreviewTest {
  /** @override */
  get browsePreload() {
    return 'chrome://print/new/app.html';
  }

  /** @override */
  get extraLibraries() {
    return super.extraLibraries.concat([
      '../test_browser_proxy.js',
      'native_layer_stub.js',
      'plugin_stub.js',
      'print_preview_test_utils.js',
      'preview_generation_test.js',
    ]);
  }

  /** @override */
  get suiteName() {
    return preview_generation_test.suiteName;
  }
};

TEST_F('PrintPreviewPreviewGenerationTest', 'Color', function() {
  this.runMochaTest(preview_generation_test.TestNames.Color);
});

TEST_F('PrintPreviewPreviewGenerationTest', 'CssBackground', function() {
  this.runMochaTest(preview_generation_test.TestNames.CssBackground);
});

TEST_F('PrintPreviewPreviewGenerationTest', 'FitToPage', function() {
  this.runMochaTest(preview_generation_test.TestNames.FitToPage);
});

TEST_F('PrintPreviewPreviewGenerationTest', 'HeaderFooter', function() {
  this.runMochaTest(preview_generation_test.TestNames.HeaderFooter);
});

TEST_F('PrintPreviewPreviewGenerationTest', 'Layout', function() {
  this.runMochaTest(preview_generation_test.TestNames.Layout);
});

TEST_F('PrintPreviewPreviewGenerationTest', 'Margins', function() {
  this.runMochaTest(preview_generation_test.TestNames.Margins);
});

TEST_F('PrintPreviewPreviewGenerationTest', 'MediaSize', function() {
  this.runMochaTest(preview_generation_test.TestNames.MediaSize);
});

TEST_F('PrintPreviewPreviewGenerationTest', 'PageRange', function() {
  this.runMochaTest(preview_generation_test.TestNames.PageRange);
});

TEST_F('PrintPreviewPreviewGenerationTest', 'SelectionOnly', function() {
  this.runMochaTest(preview_generation_test.TestNames.SelectionOnly);
});

TEST_F('PrintPreviewPreviewGenerationTest', 'PagesPerSheet', function() {
  loadTimeData.overrideValues({pagesPerSheetEnabled: true});
  this.runMochaTest(preview_generation_test.TestNames.PagesPerSheet);
});

TEST_F('PrintPreviewPreviewGenerationTest', 'Scaling', function() {
  this.runMochaTest(preview_generation_test.TestNames.Scaling);
});

GEN('#if !defined(OS_WIN) && !defined(OS_MACOSX)');
TEST_F('PrintPreviewPreviewGenerationTest', 'Rasterize', function() {
  this.runMochaTest(preview_generation_test.TestNames.Rasterize);
});
GEN('#endif');

TEST_F('PrintPreviewPreviewGenerationTest', 'Destination', function() {
  this.runMochaTest(preview_generation_test.TestNames.Destination);
});

GEN('#if !defined(OS_CHROMEOS)');
PrintPreviewLinkContainerTest = class extends NewPrintPreviewTest {
  /** @override */
  get browsePreload() {
    return 'chrome://print/new/link_container.html';
  }

  /** @override */
  get extraLibraries() {
    return super.extraLibraries.concat([
      '../settings/test_util.js',
      'print_preview_test_utils.js',
      'link_container_test.js',
    ]);
  }

  /** @override */
  get suiteName() {
    return link_container_test.suiteName;
  }
};

TEST_F('PrintPreviewLinkContainerTest', 'HideInAppKioskMode', function() {
  this.runMochaTest(link_container_test.TestNames.HideInAppKioskMode);
});

TEST_F('PrintPreviewLinkContainerTest', 'SystemDialogLinkClick', function() {
  this.runMochaTest(link_container_test.TestNames.SystemDialogLinkClick);
});
GEN('#endif');  // !defined(OS_CHROMEOS)

GEN('#if defined(OS_MACOSX)');
TEST_F('PrintPreviewLinkContainerTest', 'OpenInPreviewLinkClick', function() {
  this.runMochaTest(link_container_test.TestNames.OpenInPreviewLinkClick);
});
GEN('#endif');  // defined(OS_MACOSX)

GEN('#if defined(OS_WIN) || defined(OS_MACOSX)');
PrintPreviewSystemDialogBrowserTest = class extends NewPrintPreviewTest {
  /** @override */
  get browsePreload() {
    return 'chrome://print/new/app.html';
  }

  /** @override */
  get extraLibraries() {
    return super.extraLibraries.concat([
      ROOT_PATH + 'chrome/test/data/webui/settings/test_util.js',
      '../test_browser_proxy.js',
      'native_layer_stub.js',
      'plugin_stub.js',
      'print_preview_test_utils.js',
      'system_dialog_browsertest.js',
    ]);
  }

  /** @override */
  get suiteName() {
    return system_dialog_browsertest.suiteName;
  }
};

TEST_F(
    'PrintPreviewSystemDialogBrowserTest', 'LinkTriggersLocalPrint',
    function() {
      this.runMochaTest(
          system_dialog_browsertest.TestNames.LinkTriggersLocalPrint);
    });

TEST_F(
    'PrintPreviewSystemDialogBrowserTest', 'InvalidSettingsDisableLink',
    function() {
      this.runMochaTest(
          system_dialog_browsertest.TestNames.InvalidSettingsDisableLink);
    });
GEN('#endif');  // defined(OS_WIN) || defined(OS_MACOSX)

PrintPreviewInvalidSettingsBrowserTest = class extends NewPrintPreviewTest {
  /** @override */
  get browsePreload() {
    return 'chrome://print/new/app.html';
  }

  /** @override */
  get extraLibraries() {
    return super.extraLibraries.concat([
      ROOT_PATH + 'chrome/test/data/webui/settings/test_util.js',
      ROOT_PATH + 'ui/webui/resources/js/cr/event_target.js',
      '../test_browser_proxy.js',
      'cloud_print_interface_stub.js',
      'native_layer_stub.js',
      'plugin_stub.js',
      'print_preview_test_utils.js',
      'invalid_settings_browsertest.js',
    ]);
  }

  /** @override */
  get suiteName() {
    return invalid_settings_browsertest.suiteName;
  }
};

TEST_F(
    'PrintPreviewInvalidSettingsBrowserTest', 'NoPDFPluginError', function() {
      this.runMochaTest(
          invalid_settings_browsertest.TestNames.NoPDFPluginError);
    });

TEST_F(
    'PrintPreviewInvalidSettingsBrowserTest', 'InvalidSettingsError',
    function() {
      this.runMochaTest(
          invalid_settings_browsertest.TestNames.InvalidSettingsError);
    });

TEST_F(
    'PrintPreviewInvalidSettingsBrowserTest', 'InvalidCertificateError',
    function() {
      loadTimeData.overrideValues({isEnterpriseManaged: false});
      this.runMochaTest(
          invalid_settings_browsertest.TestNames.InvalidCertificateError);
    });

TEST_F(
    'PrintPreviewInvalidSettingsBrowserTest',
    'InvalidCertificateErrorReselectDestination', function() {
      loadTimeData.overrideValues({isEnterpriseManaged: false});
      this.runMochaTest(invalid_settings_browsertest.TestNames
                            .InvalidCertificateErrorReselectDestination);
    });

PrintPreviewDestinationSelectTest = class extends NewPrintPreviewTest {
  /** @override */
  get browsePreload() {
    return 'chrome://print/new/app.html';
  }

  /** @override */
  get extraLibraries() {
    return super.extraLibraries.concat([
      '../test_browser_proxy.js',
      'native_layer_stub.js',
      'print_preview_test_utils.js',
      'destination_select_test.js',
    ]);
  }

  /** @override */
  get suiteName() {
    return destination_select_test.suiteName;
  }
};

TEST_F(
    'PrintPreviewDestinationSelectTest', 'SingleRecentDestination', function() {
      this.runMochaTest(
          destination_select_test.TestNames.SingleRecentDestination);
    });

TEST_F(
    'PrintPreviewDestinationSelectTest', 'MultipleRecentDestinations',
    function() {
      this.runMochaTest(
          destination_select_test.TestNames.MultipleRecentDestinations);
    });

TEST_F(
    'PrintPreviewDestinationSelectTest', 'MultipleRecentDestinationsOneRequest',
    function() {
      this.runMochaTest(destination_select_test.TestNames
                            .MultipleRecentDestinationsOneRequest);
    });

TEST_F(
    'PrintPreviewDestinationSelectTest', 'DefaultDestinationSelectionRules',
    function() {
      this.runMochaTest(
          destination_select_test.TestNames.DefaultDestinationSelectionRules);
    });

GEN('#if !defined(OS_CHROMEOS)');
TEST_F(
    'PrintPreviewDestinationSelectTest', 'SystemDefaultPrinterPolicy',
    function() {
      loadTimeData.overrideValues({useSystemDefaultPrinter: true});
      this.runMochaTest(
          destination_select_test.TestNames.SystemDefaultPrinterPolicy);
    });
GEN('#endif');

PrintPreviewDestinationDialogTest = class extends NewPrintPreviewTest {
  /** @override */
  get browsePreload() {
    return 'chrome://print/new/destination_dialog.html';
  }

  /** @override */
  get extraLibraries() {
    return super.extraLibraries.concat([
      ROOT_PATH + 'chrome/test/data/webui/settings/test_util.js',
      ROOT_PATH + 'ui/webui/resources/js/webui_listener_tracker.js',
      ROOT_PATH + 'ui/webui/resources/js/cr/event_target.js',
      '../test_browser_proxy.js',
      'cloud_print_interface_stub.js',
      'native_layer_stub.js',
      'print_preview_test_utils.js',
      'destination_dialog_test.js',
    ]);
  }

  /** @override */
  get suiteName() {
    return destination_dialog_test.suiteName;
  }
};

TEST_F('PrintPreviewDestinationDialogTest', 'PrinterList', function() {
  this.runMochaTest(destination_dialog_test.TestNames.PrinterList);
});

PrintPreviewAdvancedDialogTest = class extends NewPrintPreviewTest {
  /** @override */
  get browsePreload() {
    return 'chrome://print/new/advanced_settings_dialog.html';
  }

  /** @override */
  get extraLibraries() {
    return super.extraLibraries.concat([
      'print_preview_test_utils.js',
      'advanced_dialog_test.js',
    ]);
  }

  /** @override */
  get suiteName() {
    return advanced_dialog_test.suiteName;
  }
};

TEST_F('PrintPreviewAdvancedDialogTest', 'AdvancedSettings1Option', function() {
  this.runMochaTest(advanced_dialog_test.TestNames.AdvancedSettings1Option);
});

TEST_F(
    'PrintPreviewAdvancedDialogTest', 'AdvancedSettings2Options', function() {
      this.runMochaTest(
          advanced_dialog_test.TestNames.AdvancedSettings2Options);
    });

PrintPreviewCustomMarginsTest = class extends NewPrintPreviewTest {
  /** @override */
  get browsePreload() {
    return 'chrome://print/new/margin_control_container.html';
  }

  /** @override */
  get extraLibraries() {
    return super.extraLibraries.concat([
      ROOT_PATH + 'chrome/test/data/webui/settings/test_util.js',
      'print_preview_test_utils.js',
      'custom_margins_test.js',
    ]);
  }

  /** @override */
  get suiteName() {
    return custom_margins_test.suiteName;
  }
};

TEST_F('PrintPreviewCustomMarginsTest', 'ControlsCheck', function() {
  this.runMochaTest(custom_margins_test.TestNames.ControlsCheck);
});

PrintPreviewNewDestinationSearchTest = class extends NewPrintPreviewTest {
  /** @override */
  get browsePreload() {
    return 'chrome://print/new/destination_dialog.html';
  }

  /** @override */
  get extraLibraries() {
    return super.extraLibraries.concat([
      ROOT_PATH + 'chrome/test/data/webui/settings/test_util.js',
      ROOT_PATH + 'ui/webui/resources/js/webui_listener_tracker.js',
      '../test_browser_proxy.js',
      'native_layer_stub.js',
      'print_preview_test_utils.js',
      'destination_search_test.js',
    ]);
  }

  /** @override */
  get suiteName() {
    return destination_search_test.suiteName;
  }
};

TEST_F(
    'PrintPreviewNewDestinationSearchTest', 'ReceiveSuccessfulSetup',
    function() {
      this.runMochaTest(
          destination_search_test.TestNames.ReceiveSuccessfulSetup);
    });

GEN('#if defined(OS_CHROMEOS)');
TEST_F('PrintPreviewNewDestinationSearchTest', 'ResolutionFails', function() {
  this.runMochaTest(destination_search_test.TestNames.ResolutionFails);
});

TEST_F(
    'PrintPreviewNewDestinationSearchTest', 'ReceiveFailedSetup', function() {
      this.runMochaTest(destination_search_test.TestNames.ReceiveFailedSetup);
    });

GEN('#else');  // !defined(OS_CHROMEOS)
TEST_F(
    'PrintPreviewNewDestinationSearchTest', 'GetCapabilitiesFails', function() {
      this.runMochaTest(destination_search_test.TestNames.GetCapabilitiesFails);
    });
GEN('#endif');  // defined(OS_CHROMEOS)

TEST_F('PrintPreviewNewDestinationSearchTest', 'CloudKioskPrinter', function() {
  this.runMochaTest(destination_search_test.TestNames.CloudKioskPrinter);
});

PrintPreviewHeaderTest = class extends NewPrintPreviewTest {
  /** @override */
  get browsePreload() {
    return 'chrome://print/new/header.html';
  }

  /** @override */
  get extraLibraries() {
    return super.extraLibraries.concat([
      'header_test.js',
    ]);
  }

  /** @override */
  get suiteName() {
    return header_test.suiteName;
  }
};

TEST_F('PrintPreviewHeaderTest', 'HeaderPrinterTypes', function() {
  this.runMochaTest(header_test.TestNames.HeaderPrinterTypes);
});

TEST_F('PrintPreviewHeaderTest', 'HeaderWithDuplex', function() {
  this.runMochaTest(header_test.TestNames.HeaderWithDuplex);
});

TEST_F('PrintPreviewHeaderTest', 'HeaderWithCopies', function() {
  this.runMochaTest(header_test.TestNames.HeaderWithCopies);
});

TEST_F('PrintPreviewHeaderTest', 'HeaderWithNup', function() {
  loadTimeData.overrideValues({pagesPerSheetEnabled: true});
  this.runMochaTest(header_test.TestNames.HeaderWithNup);
});

TEST_F('PrintPreviewHeaderTest', 'HeaderChangesForState', function() {
  this.runMochaTest(header_test.TestNames.HeaderChangesForState);
});
