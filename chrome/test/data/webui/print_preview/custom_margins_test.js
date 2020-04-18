// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

cr.define('custom_margins_test', function() {
  /** @enum {string} */
  const TestNames = {
    ControlsCheck: 'controls check',
  };

  const suiteName = 'CustomMarginsTest';
  suite(suiteName, function() {
    /** @type {?PrintPreviewMarginControlContainerElement} */
    let container = null;

    /** @override */
    setup(function() {
      PolymerTest.clearBody();

      // Only care about marginType and customMargins.
      const settings = {
        margins: {
          value: print_preview.ticket_items.MarginsTypeValue.DEFAULT,
          unavailableValue: print_preview.ticket_items.MarginsTypeValue.DEFAULT,
          valid: true,
          available: true,
          key: 'marginsType',
        },
        customMargins: {
          value: {
            marginTop: 0,
            marginRight: 0,
            marginBottom: 0,
            marginLeft: 0,
          },
          unavailableValue: {},
          valid: true,
          available: true,
          key: 'customMargins',
        }
      };

      // Other inputs needed by margin control container.
      const pageSize = new print_preview.Size(5100, 6600);
      const documentMargins = new print_preview.Margins(300, 300, 300, 300);
      const measurementSystem = new print_preview.MeasurementSystem(
          ',', '.', print_preview.MeasurementSystemUnitType.IMPERIAL);

      // Set up container
      container =
          document.createElement('print-preview-margin-control-container');
      container.settings = settings;
      container.pageSize = pageSize;
      container.documentMargins = documentMargins;
      container.previewLoaded = false;
      container.measurementSystem = measurementSystem;
      document.body.appendChild(container);
      container.updateClippingMask(new print_preview.Size(5100, 6600));
      container.updateScaleTransform(1);
      Polymer.dom.flush();
    });

    // Test that destinations are correctly displayed in the lists.
    test(assert(TestNames.ControlsCheck), function() {
      const controls =
          container.shadowRoot.querySelectorAll('print-preview-margin-control');
      assertEquals(4, controls.length);

      // Controls are not visible when margin type DEFAULT is selected.
      container.previewLoaded = true;
      controls.forEach(control => {
        assertEquals('0', window.getComputedStyle(control).opacity);
      });

      /**
       * @param {!Array<!PrintPreviewMarginControlElement>} controls
       * @return {!Promise} Promise that resolves when transitionend has fired
       *     for all of the controls.
       */
      const getAllTransitions = function(controls) {
        return Promise.all(Array.from(controls).map(
            control => test_util.eventToPromise('transitionend', control)));
      };

      let onTransitionEnd = getAllTransitions(controls);
      // Controls become visible when margin type CUSTOM is selected.
      container.set(
          'settings.margins.value',
          print_preview.ticket_items.MarginsTypeValue.CUSTOM);

      // Wait for the opacity transitions to finish.
      return onTransitionEnd
          .then(function() {
            // Verify margins are correctly set based on previous value.
            assertEquals(300, container.settings.customMargins.value.marginTop);
            assertEquals(
                300, container.settings.customMargins.value.marginLeft);
            assertEquals(
                300, container.settings.customMargins.value.marginRight);
            assertEquals(
                300, container.settings.customMargins.value.marginBottom);

            // Verify there is one control for each side and that controls are
            // visible and positioned correctly.
            const sides = [
              print_preview.ticket_items.CustomMarginsOrientation.TOP,
              print_preview.ticket_items.CustomMarginsOrientation.RIGHT,
              print_preview.ticket_items.CustomMarginsOrientation.BOTTOM,
              print_preview.ticket_items.CustomMarginsOrientation.LEFT
            ];
            controls.forEach((control, index) => {
              assertFalse(control.invisible);
              assertEquals('1', window.getComputedStyle(control).opacity);
              assertEquals(sides[index], control.side);
              assertEquals(300, control.getPositionInPts());
            });

            let onTransitionEnd = getAllTransitions(controls);

            // Disappears when preview is loading or an error message is shown.
            // Check that all the controls also disappear.
            container.previewLoaded = false;
            // Wait for the opacity transitions to finish.
            return onTransitionEnd;
          })
          .then(function() {
            controls.forEach((control, index) => {
              assertEquals('0', window.getComputedStyle(control).opacity);
              assertTrue(control.invisible);
            });
          });
    });
  });

  return {
    suiteName: suiteName,
    TestNames: TestNames,
  };
});
