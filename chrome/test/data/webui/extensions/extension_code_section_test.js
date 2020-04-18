// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/** @fileoverview Suite of tests for extensions-code-section. */
cr.define('extension_code_section_tests', function() {
  /** @enum {string} */
  var TestNames = {
    Layout: 'layout',
    LongSource: 'long source',
  };

  var suiteName = 'ExtensionCodeSectionTest';

  suite(suiteName, function() {
    /** @type {extensions.CodeSection} */
    var codeSection;

    var couldNotDisplayCode = 'No code here';

    // Initialize an extension item before each test.
    setup(function() {
      PolymerTest.clearBody();
      codeSection = new extensions.CodeSection();
      codeSection.couldNotDisplayCode = couldNotDisplayCode;
      document.body.appendChild(codeSection);
    });

    test(assert(TestNames.Layout), function() {
      /** @type {chrome.developerPrivate.RequestFileSourceResponse} */
      var code = {
        beforeHighlight: 'this part before the highlight\nAnd this too\n',
        highlight: 'highlight this part\n',
        afterHighlight: 'this part after the highlight',
        message: 'Highlight message',
      };

      var testIsVisible = extension_test_util.isVisible.bind(null, codeSection);
      expectFalse(!!codeSection.code);
      expectTrue(codeSection.$$('#scroll-container').hidden);
      expectFalse(testIsVisible('#main'));
      expectTrue(testIsVisible('#no-code'));

      codeSection.code = code;
      expectTrue(testIsVisible('#main'));
      expectFalse(testIsVisible('#no-code'));

      let codeSections =
          codeSection.querySelectorAll('* /deep/ #source span span');

      expectEquals(code.beforeHighlight, codeSections[0].textContent);
      expectEquals(code.highlight, codeSections[1].textContent);
      expectEquals(code.afterHighlight, codeSections[2].textContent);

      expectEquals(
          '1\n2\n3\n4',
          codeSection.$$('#line-numbers span').textContent.trim());
    });

    test(assert(TestNames.LongSource), function() {
      /** @type {chrome.developerPrivate.RequestFileSourceResponse} */
      var code;
      var lineNums;

      function setCodeContent(beforeLineCount, afterLineCount) {
        code = {
          beforeHighlight: '',
          highlight: 'highlight',
          afterHighlight: '',
          message: 'Highlight message',
        };
        for (let i = 0; i < beforeLineCount; i++)
          code.beforeHighlight += 'a\n';
        for (let i = 0; i < afterLineCount; i++)
          code.afterHighlight += 'a\n';
      }

      setCodeContent(0, 2000);
      codeSection.code = code;
      lineNums = codeSection.$$('#line-numbers span').textContent;
      // Length should be 1000 +- 1.
      expectTrue(lineNums.split('\n').length >= 999);
      expectTrue(lineNums.split('\n').length <= 1001);
      expectTrue(!!lineNums.match(/^1\n/));
      expectTrue(!!lineNums.match(/1000/));
      expectFalse(!!lineNums.match(/1001/));
      expectTrue(codeSection.$$('#line-numbers .more-code.before').hidden);
      expectFalse(codeSection.$$('#line-numbers .more-code.after').hidden);

      setCodeContent(1000, 1000);
      codeSection.code = code;
      lineNums = codeSection.$$('#line-numbers span').textContent;
      // Length should be 1000 +- 1.
      expectTrue(lineNums.split('\n').length >= 999);
      expectTrue(lineNums.split('\n').length <= 1001);
      expectFalse(!!lineNums.match(/^1\n/));
      expectTrue(!!lineNums.match(/1000/));
      expectFalse(!!lineNums.match(/1999/));
      expectFalse(codeSection.$$('#line-numbers .more-code.before').hidden);
      expectFalse(codeSection.$$('#line-numbers .more-code.after').hidden);

      setCodeContent(2000, 0);
      codeSection.code = code;
      lineNums = codeSection.$$('#line-numbers span').textContent;
      // Length should be 1000 +- 1.
      expectTrue(lineNums.split('\n').length >= 999);
      expectTrue(lineNums.split('\n').length <= 1001);
      expectFalse(!!lineNums.match(/^1\n/));
      expectTrue(!!lineNums.match(/1002/));
      expectTrue(!!lineNums.match(/2000/));
      expectFalse(codeSection.$$('#line-numbers .more-code.before').hidden);
      expectTrue(codeSection.$$('#line-numbers .more-code.after').hidden);
    });
  });

  return {
    suiteName: suiteName,
    TestNames: TestNames,
  };
});
