// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

suite('cr-checkbox', function() {
  let checkbox;

  setup(function() {
    PolymerTest.clearBody();
    document.body.innerHTML = `
      <cr-checkbox>
        <div>label
          <a>link</a>
        </div>
      </cr-checkbox>
    `;

    checkbox = document.querySelector('cr-checkbox');
    assertNotChecked();
  });

  function assertChecked() {
    assertTrue(checkbox.checked);
    assertTrue(checkbox.hasAttribute('checked'));
    assertEquals('true', checkbox.getAttribute('aria-checked'));
  }

  function assertNotChecked() {
    assertFalse(checkbox.checked);
    assertEquals(null, checkbox.getAttribute('checked'));
    assertEquals('false', checkbox.getAttribute('aria-checked'));
  }

  function assertDisabled() {
    assertTrue(checkbox.disabled);
    assertEquals('-1', checkbox.getAttribute('tabindex'));
    assertTrue(checkbox.hasAttribute('disabled'));
    assertEquals('true', checkbox.getAttribute('aria-disabled'));
    assertEquals('none', getComputedStyle(checkbox).pointerEvents);
  }

  function assertNotDisabled() {
    assertFalse(checkbox.disabled);
    assertEquals('0', checkbox.getAttribute('tabindex'));
    assertFalse(checkbox.hasAttribute('disabled'));
    assertEquals('false', checkbox.getAttribute('aria-disabled'));
  }

  /**
   * @param {string} keyName The name of the key to trigger.
   * @param {string} keyCode The event keyCode and code to trigger.
   * @param {HTMLElement=} element
   */
  function triggerKeyPressEvent(keyName, keyCode, element) {
    element = element || checkbox;

    // Note: MockInteractions incorrectly populates |keyCode| and |code| with
    // the same value. The intention of passing a string here is only to set
    // |code|, since |keyCode| is not used its value doesn't matter.
    MockInteractions.keyEventOn(
        element, 'keypress', keyCode, undefined, keyName);
  }

  // Test that the control is checked when the user taps on it (no movement
  // between pointerdown and pointerup).
  test('ToggleByMouse', function() {
    let whenChanged = test_util.eventToPromise('change', checkbox);
    checkbox.click();
    return whenChanged
        .then(function() {
          assertChecked();
          whenChanged = test_util.eventToPromise('change', checkbox);
          checkbox.click();
          return whenChanged;
        })
        .then(function() {
          assertNotChecked();
        });
  });

  // Test that the control is checked when the |checked| attribute is
  // programmatically changed.
  test('ToggleByAttribute', function(done) {
    test_util.eventToPromise('change', checkbox).then(function() {
      // Should not fire 'change' event when state is changed programmatically.
      // Only user interaction should result in 'change' event.
      assertFalse(true);
    });

    checkbox.checked = true;
    assertChecked();

    checkbox.checked = false;
    assertNotChecked();

    // Wait 1 cycle to make sure change-event was not fired.
    setTimeout(done);
  });

  // Test that the control is checked when the user presses the 'Enter' or
  // 'Space' key.
  test('ToggleByKey', function() {
    let whenChanged = test_util.eventToPromise('change', checkbox);
    triggerKeyPressEvent('Enter', 'Enter');
    return whenChanged
        .then(function() {
          assertChecked();
          whenChanged = test_util.eventToPromise('change', checkbox);
          triggerKeyPressEvent(' ', 'Space');
          return whenChanged;
        })
        .then(function() {
          assertNotChecked();
          whenChanged = test_util.eventToPromise('change', checkbox);
          triggerKeyPressEvent('Enter', 'NumpadEnter');
          return whenChanged;
        })
        .then(function() {
          assertChecked();
        });
  });

  // Test that the control is not affected by user interaction when disabled.
  test('ToggleWhenDisabled', function(done) {
    assertNotDisabled();
    checkbox.disabled = true;
    assertDisabled();

    test_util.eventToPromise('change', checkbox).then(function() {
      assertFalse(true);
    });

    checkbox.click();
    triggerKeyPressEvent('Enter', 'Enter');

    // Wait 1 cycle to make sure change-event was not fired.
    setTimeout(done);
  });

  test('LabelDisplay', function() {
    const labelContainer = checkbox.$['label-container'];
    // Test that there's actually a label that's more than just the padding.
    assertTrue(labelContainer.offsetWidth > 20);

    checkbox.classList.add('no-label');
    assertEquals('none', getComputedStyle(labelContainer).display);
  });

  test('ClickedOnLinkDoesNotToggleCheckbox', function(done) {
    test_util.eventToPromise('change', checkbox).then(function() {
      assertFalse(true);
    });

    assertNotChecked();
    link = document.querySelector('a');
    link.click();
    assertNotChecked();

    triggerKeyPressEvent('Enter', 'Enter', link);
    assertNotChecked();

    // Wait 1 cycle to make sure change-event was not fired.
    setTimeout(done);
  });

  test('InitializingWithTabindex', function() {
    PolymerTest.clearBody();
    document.body.innerHTML = `
      <cr-checkbox id="checkbox" tabindex="-1"></cr-checkbox>
    `;

    checkbox = document.querySelector('cr-checkbox');

    // Should not override tabindex if it is initialized.
    assertEquals(-1, checkbox.tabIndex);
  });


  test('InitializingWithDisabled', function() {
    PolymerTest.clearBody();
    document.body.innerHTML = `
      <cr-checkbox id="checkbox" disabled></cr-checkbox>
    `;

    checkbox = document.querySelector('cr-checkbox');

    // Initializing with disabled should make tabindex="-1".
    assertEquals(-1, checkbox.tabIndex);
  });

  // Ensure that even if user clicks on the element, the entire element gets
  // focused, as opposed to focusing the inner <button> only.
  test('FocusAfterClicking', function() {
    const innerButton = checkbox.$$('button');
    innerButton.focus();
    assertNotChecked();
    assertEquals(null, checkbox.shadowRoot.activeElement);
    assertEquals('CR-CHECKBOX', document.activeElement.tagName);
  });

});
