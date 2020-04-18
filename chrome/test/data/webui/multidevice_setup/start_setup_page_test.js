// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * In the testing framework, a click on a select option does not cause a change
 * in the select tag's attribute or trigger a change event so this method
 * emulates that behavior.
 *
 * @param {!HTMLSelectElement} Dropdown menu to endow with emulated behavior.
 */
let emulateDropdownBehavior = function(dropdown) {
  for (let i = 0; i < dropdown.length; i++) {
    dropdown.options[i].addEventListener('click', function() {
      dropdown.selectedIndex = i;
      dropdown.dispatchEvent(new CustomEvent('change'));
    });
  }
};

/**
 * @fileoverview Suite of tests for page-specific behaviors of StartSetupPage.
 */
cr.define('multidevice_setup', () => {
  function registerStartSetupPageTests() {
    suite('MultiDeviceSetup', () => {
      /**
       * StartSetupPage created before each test. Defined in setUp.
       * @type {StartSetupPage|undefined}
       */
      let startSetupPageElement;

      const START = 'start-setup-page';
      const DEVICES = [
        {name: 'Pixel XL', publicKey: 'abcdxl'},
        {name: 'Nexus 6P', publicKey: 'PpPpPp'},
        {name: 'Nexus 5', publicKey: '12345'},
      ];

      setup(() => {
        startSetupPageElement = document.createElement('start-setup-page');
        document.body.appendChild(startSetupPageElement);
        startSetupPageElement.devices = DEVICES;
        Polymer.dom.flush();
        emulateDropdownBehavior(startSetupPageElement.$.deviceDropdown);
      });

      let selectOptionByTextContent = function(optionText) {
        const optionNodeList =
            startSetupPageElement.querySelectorAll('* /deep/ option');
        for (option of optionNodeList.values()) {
          if (option.textContent.trim() == optionText) {
            MockInteractions.tap(option);
            return;
          }
        }
      };

      test(
          'Finding devices populates dropdown and defines selected device',
          () => {
            assertEquals(
                startSetupPageElement.querySelectorAll('* /deep/ option')
                    .length,
                DEVICES.length);
            assertEquals(startSetupPageElement.selectedPublicKey, 'abcdxl');
          });

      test(
          'selectedPublicKey changes when dropdown options are selected',
          () => {
            selectOptionByTextContent('Nexus 6P');
            assertEquals(startSetupPageElement.selectedPublicKey, 'PpPpPp');
            selectOptionByTextContent('Nexus 5');
            assertEquals(startSetupPageElement.selectedPublicKey, '12345');
            selectOptionByTextContent('Pixel XL');
            assertEquals(startSetupPageElement.selectedPublicKey, 'abcdxl');
          });
    });
  }
  return {registerStartSetupPageTests: registerStartSetupPageTests};
});
