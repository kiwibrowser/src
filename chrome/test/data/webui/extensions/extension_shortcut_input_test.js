// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/** @fileoverview Suite of tests for extension-keyboard-shortcuts. */
cr.define('extension_shortcut_input_tests', function() {
  /** @enum {string} */
  const TestNames = {
    Basic: 'basic',
  };

  const suiteName = 'ExtensionShortcutInputTest';

  suite(suiteName, function() {
    /** @type {extensions.ShortcutInput} */
    var input;
    setup(function() {
      PolymerTest.clearBody();
      input = new extensions.ShortcutInput();
      input.delegate = new extensions.TestService();
      input.commandName = 'Command';
      input.item = 'itemid';
      document.body.appendChild(input);
      Polymer.dom.flush();
    });

    test(assert(TestNames.Basic), function() {
      const field = input.$['input'];
      expectEquals('', field.value);
      const isClearVisible =
          extension_test_util.isVisible.bind(null, input, '#clear', false);
      expectFalse(isClearVisible());

      // Click the input. Capture should start.
      MockInteractions.tap(field);
      return input.delegate.whenCalled('setShortcutHandlingSuspended')
          .then((arg) => {
            assertTrue(arg);
            input.delegate.reset();

            expectEquals('', field.value);
            expectFalse(isClearVisible());

            // Press character.
            MockInteractions.keyDownOn(field, 'A', []);
            expectEquals('', field.value);
            expectTrue(field.errorMessage.startsWith('Include'));
            // Add shift to character.
            MockInteractions.keyDownOn(field, 'A', ['shift']);
            expectEquals('', field.value);
            expectTrue(field.errorMessage.startsWith('Include'));
            // Press ctrl.
            MockInteractions.keyDownOn(field, 17, ['ctrl']);
            expectEquals('Ctrl', field.value);
            expectEquals('Type a letter', field.errorMessage);
            // Add shift.
            MockInteractions.keyDownOn(field, 16, ['ctrl', 'shift']);
            expectEquals('Ctrl + Shift', field.value);
            expectEquals('Type a letter', field.errorMessage);
            // Remove shift.
            MockInteractions.keyUpOn(field, 16, ['ctrl']);
            expectEquals('Ctrl', field.value);
            expectEquals('Type a letter', field.errorMessage);
            // Add alt (ctrl + alt is invalid).
            MockInteractions.keyDownOn(field, 18, ['ctrl', 'alt']);
            expectEquals('Ctrl', field.value);
            // Remove alt.
            MockInteractions.keyUpOn(field, 18, ['ctrl']);
            expectEquals('Ctrl', field.value);
            expectEquals('Type a letter', field.errorMessage);

            // Add 'A'. Once a valid shortcut is typed (like Ctrl + A), it is
            // committed.
            MockInteractions.keyDownOn(field, 65, ['ctrl']);
            return input.delegate.whenCalled(
                'updateExtensionCommandKeybinding');
          })
          .then((arg) => {
            input.delegate.reset();
            expectDeepEquals(['itemid', 'Command', 'Ctrl+A'], arg);
            expectEquals('Ctrl + A', field.value);
            expectEquals('Ctrl+A', input.shortcut);
            expectTrue(isClearVisible());

            // Test clearing the shortcut.
            MockInteractions.tap(input.$['clear']);
            return input.delegate.whenCalled(
                'updateExtensionCommandKeybinding');
          })
          .then((arg) => {
            input.delegate.reset();
            expectDeepEquals(['itemid', 'Command', ''], arg);
            expectEquals('', input.shortcut);
            expectFalse(isClearVisible());

            MockInteractions.tap(field);
            return input.delegate.whenCalled('setShortcutHandlingSuspended');
          })
          .then((arg) => {
            input.delegate.reset();
            expectTrue(arg);

            // Test ending capture using the escape key.
            MockInteractions.keyDownOn(field, 27);  // Escape key.
            return input.delegate.whenCalled('setShortcutHandlingSuspended');
          })
          .then(expectFalse);
    });
  });

  return {
    suiteName: suiteName,
    TestNames: TestNames,
  };
});
