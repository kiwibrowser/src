// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

'use strict';

(function() {

var appId;

/**
 * Returns steps for initializing test cases.
 * @param {string} manifest The manifest name of testing provider extension
 *     to launch for the test case.
 * @return {!Array<function>}
 */
function getSetupSteps(manifest) {
  return [
    function() {
      chrome.test.sendMessage(
          JSON.stringify({
            name: 'launchProviderExtension',
            manifest: manifest
          }), this.next);
    },
    function() {
      setupAndWaitUntilReady(null, RootPath.DOWNLOADS, this.next);
    },
    function(results) {
      appId = results.windowId;
      this.next();
    }
  ];
}

/**
 * Returns steps for clicking on the "Add new services" menu button.
 * @return {!Array<function>}
 */
function getClickMenuSteps() {
  return [
    function() {
      remoteCall.callRemoteTestUtil(
          'fakeMouseClick',
          appId,
          ['div[menu=\'#add-new-services-menu\']'],
          this.next);
    }
  ];
}

/**
 * Returns steps for confirming that a provided volume is mounted.
 * @return {!Array<function>}
 */
function getConfirmVolumeSteps(ejectExpected) {
  return [
    function() {
      remoteCall.waitForElement(
          appId,
          '.tree-row .icon[volume-type-icon="provided"]')
          .then(this.next);
    },
    function() {
      remoteCall.callRemoteTestUtil(
          'fakeMouseClick',
          appId,
          ['.tree-row .icon[volume-type-icon="provided"]'],
          this.next);
    },
    function(result) {
      chrome.test.assertTrue(result);
      remoteCall.waitForElement(
          appId,
          '.tree-row[selected] .icon[volume-type-icon="provided"]')
          .then(this.next);
    },
    function() {
      if (ejectExpected) {
        remoteCall.waitForElement(
            appId,
            '.tree-row[selected] .root-eject')
            .then(this.next);
      } else {
        remoteCall.waitForElementLost(
            appId,
            '.tree-row[selected] .root-eject')
            .then(this.next);
      }
    },
  ];
}

/**
 * Tests that a provided extension with |manifest| is mountable via the menu
 * button.
 *
 * @param {boolean} multipleMounts Whether multiple mounts are supported by the
 *     providing extension.
 * @param {string} manifest Name of the manifest file for the providing
 *     extension.
 */
function requestMountInternal(multipleMounts, manifest) {
  StepsRunner.runGroups([
    getSetupSteps(manifest),
    getClickMenuSteps(),
    [
      function(result) {
        chrome.test.assertTrue(result);
        remoteCall.waitForElement(
            appId,
            '#add-new-services-menu:not([hidden]) cr-menu-item:first-child ' +
                'span')
            .then(this.next);
      },
      function(result) {
        chrome.test.assertEq('Testing Provider', result.text);
        remoteCall.callRemoteTestUtil(
            'fakeMouseClick',
            appId,
            ['#add-new-services-menu cr-menu-item:first-child span'],
            this.next);
      },
    ],
    getConfirmVolumeSteps(false /* ejectExpected */),
    getClickMenuSteps(),
    [
      function() {
        // When multiple mounts are supported, then the first element of the
        // menu should stay the same. Otherwise it should disappear from the
        // list.
        var selector = multipleMounts ?
            '#add-new-services-menu:not([hidden]) cr-menu-item:first-child ' +
                'span' :
            '#add-new-services-menu:not([hidden]) hr:first-child';
        remoteCall.waitForElement(
            appId,
            selector)
            .then(this.next);
      },
      function(result) {
        if (multipleMounts)
          chrome.test.assertEq('Testing Provider', result.text);
        checkIfNoErrorsOccured(this.next);
      }
    ]
  ]);
}

/**
 * Tests that a provided extension with |manifest| is not available in the
 * button menu, but it's mounted automatically.
 *
 * @param {string} manifest Name of the manifest file for the providing
 *     extension.
 */
function requestMountNotInMenuInternal(manifest) {
  StepsRunner.runGroups([
    getSetupSteps(manifest),
    getConfirmVolumeSteps(true /* ejectExpected */),
    getClickMenuSteps(),
    [
      function(result) {
        chrome.test.assertTrue(result);
        // Confirm that it doesn't show up in the menu.
        remoteCall.waitForElement(
            appId,
            '#add-new-services-menu:not([hidden]) hr:first-child')
            .then(this.next);
      },
    ],
    [
      function(result) {
        checkIfNoErrorsOccured(this.next);
      }
    ]
  ]);
}

/**
 * Tests mounting a single mount point in the button menu.
 */
testcase.requestMount = function() {
  const multipleMounts = false;
  requestMountInternal(multipleMounts, 'manifest.json');
};

/**
 * Tests mounting multiple mount points in the button menu.
 */
testcase.requestMountMultipleMounts = function() {
  const multipleMounts = true;
  requestMountInternal(multipleMounts, 'manifest_multiple_mounts.json');
};

/**
 * Tests mounting a device not present in the button menu.
 */
testcase.requestMountSourceDevice = function() {
  requestMountNotInMenuInternal('manifest_source_device.json');
};

/**
 * Tests mounting a file not present in the button menu.
 */
testcase.requestMountSourceFile = function() {
  requestMountNotInMenuInternal('manifest_source_file.json');
};

})();
