// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/** @fileoverview Runs the Print Preview tests. */

const ROOT_PATH = '../../../../../';

/**
 * @constructor
 * @extends {testing.Test}
 */
function PrintPreviewUIBrowserTest() {}

PrintPreviewUIBrowserTest.prototype = {
  __proto__: testing.Test.prototype,

  /**
   * Browse to the sample page, cause print preview & call preLoad().
   * @override
   */
  browsePrintPreload: 'print_preview/print_preview_hello_world_test.html',

  /** @override */
  runAccessibilityChecks: true,

  /** @override */
  accessibilityIssuesAreErrors: true,

  /** @override */
  isAsync: true,

  /** @override */
  preLoad: function() {
    window.isTest = true;
    testing.Test.prototype.preLoad.call(this);

  },

  /** @override */
  setUp: function() {
    testing.Test.prototype.setUp.call(this);

    testing.Test.disableAnimationsAndTransitions();
    // Enable when failure is resolved.
    // AX_TEXT_03: http://crbug.com/559209
    this.accessibilityAuditConfig.ignoreSelectors(
        'multipleLabelableElementsPerLabel',
        '#page-settings > .right-column > *');
  },

  extraLibraries: [
    ROOT_PATH + 'ui/webui/resources/js/cr.js',
    ROOT_PATH + 'ui/webui/resources/js/cr/event_target.js',
    ROOT_PATH + 'ui/webui/resources/js/promise_resolver.js',
    ROOT_PATH + 'third_party/mocha/mocha.js',
    ROOT_PATH + 'chrome/test/data/webui/mocha_adapter.js',
    ROOT_PATH + 'ui/webui/resources/js/util.js',
    ROOT_PATH + 'chrome/test/data/webui/test_browser_proxy.js',
    ROOT_PATH + 'chrome/test/data/webui/settings/test_util.js',
    'print_preview_tests.js',
    'native_layer_stub.js',
    'cloud_print_interface_stub.js',
    'plugin_stub.js',
    'print_preview_test_utils.js',
  ],
};

// Run each mocha test in isolation (within a new TEST_F() call).
['PrinterList', 'RestoreLocalDestination', 'RestoreMultipleDestinations',
 'SaveAppState', 'DefaultDestinationSelectionRules',
 'SystemDialogLinkIsHiddenInAppKioskMode', 'SectionsDisabled',
 'PrintToPDFSelectedCapabilities', 'SourceIsHTMLCapabilities',
 'SourceIsPDFCapabilities', 'ScalingUnchecksFitToPage',
 'CheckNumCopiesPrintPreset', 'CheckDuplexPrintPreset',
 'CustomMarginsControlsCheck', 'PageLayoutHasNoMarginsHideHeaderFooter',
 'PageLayoutHasMarginsShowHeaderFooter',
 'ZeroTopAndBottomMarginsHideHeaderFooter',
 'ZeroTopAndNonZeroBottomMarginShowHeaderFooter', 'SmallPaperSizeHeaderFooter',
 'ColorSettingsMonochrome', 'ColorSettingsCustomMonochrome',
 'ColorSettingsColor', 'ColorSettingsCustomColor',
 'ColorSettingsBothStandardDefaultColor',
 'ColorSettingsBothStandardDefaultMonochrome',
 'ColorSettingsBothCustomDefaultColor', 'DuplexSettingsTrue',
 'DuplexSettingsFalse', 'PrinterChangeUpdatesPreview',
 'NoPDFPluginErrorMessage', 'CustomPaperNames', 'InitIssuesOneRequest',
 'InvalidSettingsError',
].forEach(function(testName) {
  TEST_F('PrintPreviewUIBrowserTest', testName, function() {
    runMochaTest(print_preview_test.suiteName, testName);
  });
});

// Disable accessibility errors for some tests.
['RestoreAppState', 'AdvancedSettings1Option', 'AdvancedSettings2Options', ]
    .forEach(function(testName) {
      TEST_F('PrintPreviewUIBrowserTest', testName, function() {
        this.accessibilityIssuesAreErrors = false;
        runMochaTest(print_preview_test.suiteName, testName);
      });
    });

['InvalidCertificateError', 'InvalidCertificateErrorReselectDestination',
 'InvalidCertificateErrorNoPreview',
].forEach(function(testName) {
  TEST_F('PrintPreviewUIBrowserTest', testName, function() {
    loadTimeData.overrideValues({isEnterpriseManaged: false});
    this.accessibilityIssuesAreErrors = false;
    runMochaTest(print_preview_test.suiteName, testName);
  });
});

GEN('#if !defined(OS_CHROMEOS)');
TEST_F('PrintPreviewUIBrowserTest', 'SystemDefaultPrinterPolicy', function() {
  loadTimeData.overrideValues({useSystemDefaultPrinter: true});
  runMochaTest(print_preview_test.suiteName, 'SystemDefaultPrinterPolicy');
});
GEN('#endif');

GEN('#if defined(OS_MACOSX)');
['MacOpenPDFInPreview', 'MacOpenPDFInPreviewBadPrintTicket', ].forEach(function(
    testName) {
  TEST_F('PrintPreviewUIBrowserTest', testName, function() {
    runMochaTest(print_preview_test.suiteName, testName);
  });
});
GEN('#endif');

GEN('#if defined(OS_WIN)');
['WinSystemDialogLink', 'WinSystemDialogLinkBadPrintTicket', ].forEach(function(
    testName) {
  TEST_F('PrintPreviewUIBrowserTest', testName, function() {
    runMochaTest(print_preview_test.suiteName, testName);
  });
});
GEN('#endif');
