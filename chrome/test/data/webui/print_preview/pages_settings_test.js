// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

cr.define('pages_settings_test', function() {
  /** @enum {string} */
  const TestNames = {
    ValidPageRanges: 'valid page ranges',
    InvalidPageRanges: 'invalid page ranges',
  };

  const suiteName = 'PagesSettingsTest';
  suite(suiteName, function() {
    /** @type {?PrintPreviewPagesSettingsElement} */
    let pagesSection = null;

    /** @type {?print_preview.DocumentInfo} */
    let documentInfo = null;

    /** @override */
    setup(function() {
      documentInfo = new print_preview.DocumentInfo();
      documentInfo.init(true, 'title', false);

      PolymerTest.clearBody();
      pagesSection = document.createElement('print-preview-pages-settings');
      pagesSection.settings = {
        pages: {
          value: [1],
          unavailableValue: [],
          valid: true,
          available: true,
          key: '',
        },
        ranges: {
          value: [],
          unavailableValue: [],
          valid: true,
          available: true,
          key: '',
        },
      };
      pagesSection.documentInfo = documentInfo;
      pagesSection.disabled = false;
      document.body.appendChild(pagesSection);
    });

    /**
     * Sets up the pages section to use the custom input with the input string
     * given by |inputString|, with the document page count set to |pageCount|
     * @param {string} inputString
     * @param {number} pageCount
     * @return {!Promise} Promise that resolves when the input-change event
     *     has fired.
     */
    function setupInput(inputString, pageCount) {
      // Set page count.
      documentInfo.updatePageCount(pageCount);
      pagesSection.notifyPath('documentInfo.pageCount');
      Polymer.dom.flush();

      // Select custom
      pagesSection.$$('#custom-radio-button').checked = true;
      pagesSection.$$('#all-radio-button')
          .dispatchEvent(new CustomEvent('change'));

      // Set input string
      const input = pagesSection.$.pageSettingsCustomInput;
      input.value = inputString;
      input.dispatchEvent(new CustomEvent('input'));

      // Validate results
      return test_util.eventToPromise('input-change', pagesSection);
    }

    // Tests that the page ranges set are valid for different user inputs.
    test(assert(TestNames.ValidPageRanges), function() {
      /** @param {!Array<number>} expectedPages The expected pages value. */
      const validateState = function(expectedPages) {
        const pagesValue = pagesSection.getSettingValue('pages');
        assertEquals(expectedPages.length, pagesValue.length);
        expectedPages.forEach((page, index) => {
          assertEquals(page, pagesValue[index]);
        });
        assertTrue(pagesSection.$$('.hint').hidden);
      };

      const oneToHundred = Array.from({length: 100}, (x, i) => i + 1);
      const tenToHundred = Array.from({length: 91}, (x, i) => i + 10);

      return setupInput('1, 2, 3, 1, 56', 100)
          .then(function() {
            validateState([1, 2, 3, 56]);
            return setupInput('1-3, 6-9, 6-10', 100);
          })
          .then(function() {
            validateState([1, 2, 3, 6, 7, 8, 9, 10]);
            return setupInput('10-', 100);
          })
          .then(function() {
            validateState(tenToHundred);
            return setupInput('10-100', 100);
          })
          .then(function() {
            validateState(tenToHundred);
            return setupInput('-', 100);
          })
          .then(function() {
            validateState(oneToHundred);
            // https://crbug.com/806165
            return setupInput('1\u30012\u30013\u30011\u300156', 100);
          })
          .then(function() {
            validateState([1, 2, 3, 56]);
            return setupInput('1,2,3\u30011\u300156', 100);
          })
          .then(function() {
            validateState([1, 2, 3, 56]);
          });
    });

    // Tests that the correct error messages are shown for different user
    // inputs.
    test(assert(TestNames.InvalidPageRanges), function() {
      const limitError = 'Out of bounds page reference, limit is ';
      const syntaxError = 'Invalid page range, use e.g. 1-5, 8, 11-13';

      /** @param {string} expectedMessage The expected error message. */
      const validateState = function(expectedMessage) {
        assertFalse(pagesSection.$$('.hint').hidden);
        assertEquals(
            expectedMessage, pagesSection.$$('.hint').textContent.trim());
      };

      return setupInput('10-100000', 100)
          .then(function() {
            validateState(limitError + '100');
            return setupInput('1, 100000', 100);
          })
          .then(function() {
            validateState(limitError + '100');
            return setupInput('1, 2, 0, 56', 100);
          })
          .then(function() {
            validateState(syntaxError);
            return setupInput('-1, 1, 2,, 56', 100);
          })
          .then(function() {
            validateState(syntaxError);
            return setupInput('1,2,56-40', 100);
          })
          .then(function() {
            validateState(syntaxError);
            return setupInput('101-110', 100);
          })
          .then(function() {
            validateState(limitError + '100');
            return setupInput('1\u30012\u30010\u300156', 100);
          })
          .then(function() {
            validateState(syntaxError);
            return setupInput('-1,1,2\u3001\u300156', 100);
          })
          .then(function() {
            validateState(syntaxError);
          });
    });
  });

  return {
    suiteName: suiteName,
    TestNames: TestNames,
  };
});
