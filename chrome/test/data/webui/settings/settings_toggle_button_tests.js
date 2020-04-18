// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/** @fileoverview Suite of tests for settings-toggle-button. */
cr.define('settings_toggle_button', () => {
  suite('SettingsToggleButton', () => {
    /**
     * Toggle button created before each test.
     * @type {SettingsCheckbox}
     */
    let testElement;

    // Initialize a checked control before each test.
    setup(() => {
      /**
       * Pref value used in tests, should reflect the 'checked' attribute.
       * Create a new pref for each test() to prevent order (state)
       * dependencies between tests.
       * @type {chrome.settingsPrivate.PrefObject}
       */
      const pref = {
        key: 'test',
        type: chrome.settingsPrivate.PrefType.BOOLEAN,
        value: true
      };
      PolymerTest.clearBody();
      testElement = document.createElement('settings-toggle-button');
      testElement.set('pref', pref);
      document.body.appendChild(testElement);
    });

    test('value changes on tap', () => {
      assertTrue(testElement.checked);
      assertTrue(testElement.pref.value);

      MockInteractions.tap(testElement);
      assertFalse(testElement.checked);
      assertFalse(testElement.pref.value);

      MockInteractions.tap(testElement);
      assertTrue(testElement.checked);
      assertTrue(testElement.pref.value);
    });

    test('fires a change event', (done) => {
      testElement.addEventListener('change', () => {
        assertFalse(testElement.checked);
        done();
      });
      assertTrue(testElement.checked);
      MockInteractions.tap(testElement);
    });

    test('fires a change event for label', (done) => {
      testElement.addEventListener('change', () => {
        assertFalse(testElement.checked);
        done();
      });
      assertTrue(testElement.checked);
      MockInteractions.tap(testElement.$.labelWrapper);
    });

    test('fires a change event for toggle', (done) => {
      testElement.addEventListener('change', () => {
        assertFalse(testElement.checked);
        done();
      });
      assertTrue(testElement.checked);
      MockInteractions.tap(testElement.$.control);
    });

    test('fires a single change event per tap', () => {
      counter = 0;
      testElement.addEventListener('change', () => {
        ++counter;
      });
      MockInteractions.tap(testElement);
      assertEquals(1, counter);
      MockInteractions.tap(testElement.$.labelWrapper);
      assertEquals(2, counter);
      MockInteractions.tap(testElement.$.control);
      assertEquals(3, counter);
    });

    test('does not change when disabled', () => {
      testElement.checked = false;
      testElement.setAttribute('disabled', '');
      assertTrue(testElement.disabled);
      assertTrue(testElement.$.control.disabled);

      MockInteractions.tap(testElement);
      assertFalse(testElement.checked);
      assertFalse(testElement.$.control.checked);
    });

    test('inverted', () => {
      testElement.inverted = true;
      testElement.set('pref', {
        key: 'test',
        type: chrome.settingsPrivate.PrefType.BOOLEAN,
        value: true
      });

      assertTrue(testElement.pref.value);
      assertFalse(testElement.checked);

      MockInteractions.tap(testElement);
      assertFalse(testElement.pref.value);
      assertTrue(testElement.checked);

      MockInteractions.tap(testElement);
      assertTrue(testElement.pref.value);
      assertFalse(testElement.checked);
    });

    test('numerical pref', () => {
      const prefNum = {
        key: 'test',
        type: chrome.settingsPrivate.PrefType.NUMBER,
        value: 1
      };

      testElement.set('pref', prefNum);
      assertTrue(testElement.checked);

      MockInteractions.tap(testElement);
      assertFalse(testElement.checked);
      assertEquals(0, prefNum.value);

      MockInteractions.tap(testElement);
      assertTrue(testElement.checked);
      assertEquals(1, prefNum.value);
    });

    test('numerical pref with custom values', () => {
      const prefNum = {
        key: 'test',
        type: chrome.settingsPrivate.PrefType.NUMBER,
        value: 5
      };

      testElement.numericUncheckedValue = 5;

      testElement.set('pref', prefNum);
      assertFalse(testElement.checked);

      MockInteractions.tap(testElement);
      assertTrue(testElement.checked);
      assertEquals(1, prefNum.value);

      MockInteractions.tap(testElement);
      assertFalse(testElement.checked);
      assertEquals(5, prefNum.value);
    });

    test('numerical pref with unknown initial value', () => {
      const prefNum = {
        key: 'test',
        type: chrome.settingsPrivate.PrefType.NUMBER,
        value: 3
      };

      testElement.numericUncheckedValue = 5;

      testElement.set('pref', prefNum);

      // Unknown value should still count as checked.
      assertTrue(testElement.checked);

      // The control should not clobber an existing unknown value.
      assertEquals(3, prefNum.value);

      // Unchecking should still send the unchecked value to prefs.
      MockInteractions.tap(testElement);
      assertFalse(testElement.checked);
      assertEquals(5, prefNum.value);

      // Checking should still send the normal checked value to prefs.
      MockInteractions.tap(testElement);
      assertTrue(testElement.checked);
      assertEquals(1, prefNum.value);
    });

    test('shows controlled indicator when pref is controlled', () => {
      assertFalse(!!testElement.$$('cr-policy-pref-indicator'));

      const pref = {
        key: 'test',
        type: chrome.settingsPrivate.PrefType.NUMBER,
        value: 3,
        enforcement: chrome.settingsPrivate.Enforcement.ENFORCED,
        controlledBy: chrome.settingsPrivate.ControlledBy.EXTENSION
      };

      testElement.set('pref', pref);
      Polymer.dom.flush();

      assertTrue(!!testElement.$$('cr-policy-pref-indicator'));
    });

    test('no indicator with no-extension-indicator flag', () => {
      assertFalse(!!testElement.$$('cr-policy-pref-indicator'));

      testElement.noExtensionIndicator = true;
      const pref = {
        key: 'test',
        type: chrome.settingsPrivate.PrefType.NUMBER,
        value: 3,
        enforcement: chrome.settingsPrivate.Enforcement.ENFORCED,
        controlledBy: chrome.settingsPrivate.ControlledBy.EXTENSION
      };

      testElement.set('pref', pref);
      Polymer.dom.flush();

      assertFalse(!!testElement.$$('cr-policy-pref-indicator'));
    });
  });
});
