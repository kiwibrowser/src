// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/** @fileoverview Suite of tests for extension-options-dialog. */
cr.define('extension_options_dialog_tests', function() {
  /** @enum {string} */
  var TestNames = {
    Layout: 'Layout',
  };

  var suiteName = 'ExtensionOptionsDialogTests';

  suite(suiteName, function() {
    /** @type {extensions.OptionsDialog} */
    var optionsDialog;

    /** @type {chrome.developerPrivate.ExtensionInfo} */
    var data;

    setup(function() {
      PolymerTest.clearBody();
      optionsDialog = new extensions.OptionsDialog();
      document.body.appendChild(optionsDialog);

      var service = extensions.Service.getInstance();
      return service.getExtensionsInfo().then(function(info) {
        assertEquals(1, info.length);
        data = info[0];
      });
    });

    function isDialogVisible() {
      var dialogElement = optionsDialog.$.dialog.getNative();
      var rect = dialogElement.getBoundingClientRect();
      return rect.width * rect.height > 0;
    }

    test(assert(TestNames.Layout), function() {
      // Try showing the dialog.
      assertFalse(isDialogVisible());
      optionsDialog.show(data);
      const dialogElement = optionsDialog.$.dialog.getNative();
      return test_util.whenAttributeIs(dialogElement, 'open', '')
          .then(function() {
            assertTrue(isDialogVisible());

            const rect = dialogElement.getBoundingClientRect();
            assertGE(rect.width, extensions.OptionsDialogMinWidth);
            assertLE(rect.height, extensions.OptionsDialogMaxHeight);

            assertEquals(
                data.name,
                assert(optionsDialog.$$('#icon-and-name-wrapper span'))
                    .textContent.trim());
          });
    });
  });

  return {
    suiteName: suiteName,
    TestNames: TestNames,
  };
});
