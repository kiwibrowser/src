// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

(async function() {
  TestRunner.addResult(`Tests that console copies truncated text in messages properly.\n`);

  await TestRunner.loadModule('console_test_runner');
  await TestRunner.showPanel('console');

  var longUrl = 'www.' + 'z123456789'.repeat(15) + '.com';
  var shortUrl = 'www.bar.com';
  var mixedUrl = longUrl + ' ' + shortUrl + ' ' + longUrl;
  var shortUrlWithHashes = 'www.' + '0123456789'.repeat(2) + 'zfoobarz' + '0123456789'.repeat(2);
  var urlWithHashes = 'www.' + '0123456789'.repeat(2) + 'z'.repeat(150) + '0123456789'.repeat(2);
  var highlightedUrl = 'www.' + 'z'.repeat(200) + '.com';
  var prepareCode = `
        // Keep this as the first url logged to record the max truncated length.
        console.log("${longUrl}");

        console.log("${shortUrl}");
        console.log("${longUrl}");
        console.log("${mixedUrl}");
        console.log("${shortUrlWithHashes}");
        console.log("${urlWithHashes}");
        console.log("${highlightedUrl}");
    `;

  var expectedMessageCount = 8;
  var consoleView = Console.ConsoleView.instance();
  var viewport = Console.ConsoleView.instance()._viewport;
  var maxLength;
  var halfMaxLength;
  var secondLongUrlIndexInMixedUrl;

  var tests = [
    function testSelectWithinTruncatedUrl(next) {
      makeSelectionAndDump(1, 0, 1, halfMaxLength);
      makeSelectionAndDump(1, 0, 1, halfMaxLength + 1);
      makeSelectionAndDump(1, 0, 1, maxLength);
      makeSelectionAndDump(1, halfMaxLength, 1, halfMaxLength + 1);
      makeSelectionAndDump(1, halfMaxLength, 1, maxLength);
      makeSelectionAndDump(1, halfMaxLength + 1, 1, maxLength);
      next();
    },

    function testSelectAcrossMultipleMessages(next) {
      makeSelectionAndDump(1, 0, 2, shortUrl.length);
      makeSelectionAndDump(1, halfMaxLength, 2, shortUrl.length);
      makeSelectionAndDump(1, halfMaxLength + 1, 2, shortUrl.length);
      next();
    },

    function testSelectAcrossMultipleMessagesWithTruncatedUrls(next) {
      makeSelectionAndDump(1, 0, 3, halfMaxLength);
      makeSelectionAndDump(1, 0, 3, halfMaxLength + 1);
      makeSelectionAndDump(1, 0, 3, maxLength);
      next();
    },

    function testSelectWithinMessageWithMultipleTruncatedUrls(next) {
      makeSelectionAndDump(4, 0, 4, halfMaxLength);
      makeSelectionAndDump(4, 0, 4, halfMaxLength + 1);
      makeSelectionAndDump(4, 0, 4, secondLongUrlIndexInMixedUrl);
      makeSelectionAndDump(4, 0, 4, secondLongUrlIndexInMixedUrl + halfMaxLength);
      makeSelectionAndDump(4, 0, 4, secondLongUrlIndexInMixedUrl + halfMaxLength + 1);
      makeSelectionAndDump(4, 0, 4, secondLongUrlIndexInMixedUrl + maxLength);

      makeSelectionAndDump(4, halfMaxLength, 4, halfMaxLength + 1);
      makeSelectionAndDump(4, halfMaxLength, 4, secondLongUrlIndexInMixedUrl);
      makeSelectionAndDump(4, halfMaxLength, 4, secondLongUrlIndexInMixedUrl + halfMaxLength);
      makeSelectionAndDump(4, halfMaxLength, 4, secondLongUrlIndexInMixedUrl + halfMaxLength + 1);
      makeSelectionAndDump(4, halfMaxLength, 4, secondLongUrlIndexInMixedUrl + maxLength);

      makeSelectionAndDump(4, halfMaxLength + 1, 4, secondLongUrlIndexInMixedUrl);
      makeSelectionAndDump(4, halfMaxLength + 1, 4, secondLongUrlIndexInMixedUrl + halfMaxLength);
      makeSelectionAndDump(4, halfMaxLength + 1, 4, secondLongUrlIndexInMixedUrl + halfMaxLength + 1);
      makeSelectionAndDump(4, halfMaxLength + 1, 4, secondLongUrlIndexInMixedUrl + maxLength);

      makeSelectionAndDump(4, secondLongUrlIndexInMixedUrl, 4, secondLongUrlIndexInMixedUrl + halfMaxLength);
      makeSelectionAndDump(4, secondLongUrlIndexInMixedUrl, 4, secondLongUrlIndexInMixedUrl + halfMaxLength + 1);
      makeSelectionAndDump(4, secondLongUrlIndexInMixedUrl, 4, secondLongUrlIndexInMixedUrl + maxLength);

      makeSelectionAndDump(
        4,
        secondLongUrlIndexInMixedUrl + halfMaxLength,
        4,
        secondLongUrlIndexInMixedUrl + halfMaxLength + 1
      );
      makeSelectionAndDump(
        4,
        secondLongUrlIndexInMixedUrl + halfMaxLength,
        4,
        secondLongUrlIndexInMixedUrl + maxLength
      );

      makeSelectionAndDump(
        4,
        secondLongUrlIndexInMixedUrl + halfMaxLength + 1,
        4,
        secondLongUrlIndexInMixedUrl + maxLength
      );
      next();
    },

    function testSelectWithinShortUrlWithHashes(next) {
      var hashedUrlMaxLength = consoleMessageText(5).length;
      var hashedUrlHalfMaxLength = Math.ceil(hashedUrlMaxLength / 2);
      makeSelectionAndDump(5, 0, 5, hashedUrlHalfMaxLength);
      makeSelectionAndDump(5, 0, 5, hashedUrlMaxLength);
      makeSelectionAndDump(5, hashedUrlHalfMaxLength, 5, hashedUrlMaxLength);
      next();
    },

    function testSelectWithinUrlWithHashes(next) {
      var hashedUrlMaxLength = consoleMessageText(6).length;
      var hashedUrlHalfMaxLength = Math.ceil(hashedUrlMaxLength / 2);
      makeSelectionAndDump(6, 0, 6, hashedUrlHalfMaxLength);
      makeSelectionAndDump(6, 0, 6, hashedUrlHalfMaxLength + 1);
      makeSelectionAndDump(6, 0, 6, hashedUrlMaxLength);
      makeSelectionAndDump(6, hashedUrlHalfMaxLength, 6, hashedUrlHalfMaxLength + 1);
      makeSelectionAndDump(6, hashedUrlHalfMaxLength, 6, hashedUrlMaxLength);
      makeSelectionAndDump(6, hashedUrlHalfMaxLength + 1, 6, hashedUrlMaxLength);
      next();
    },

    function testSelectWithinHighlightedUrlBeginning(next) {
      testHighlightedUrlWithSearchQuery('www.', next);
    },

    function testSelectWithinHighlightedUrlMiddle(next) {
      testHighlightedUrlWithSearchQuery('zzzzz', next);
    },

    function testSelectWithinHighlightedUrlEnd(next) {
      testHighlightedUrlWithSearchQuery('.com', next);
    }
  ];

  ConsoleTestRunner.waitForConsoleMessages(expectedMessageCount, () => {
    viewport.invalidate();

    // Get the max truncated length from the first longUrl logged.
    try {
      var longUrlMessageText = consoleMessageText(1);
      maxLength = longUrlMessageText.length;
      halfMaxLength = Math.ceil(maxLength / 2);
      secondLongUrlIndexInMixedUrl = maxLength + 1 + shortUrl.length + 1;
      TestRunner.addResult('Long url has max length: ' + maxLength + ', text: ' + longUrlMessageText);
    } catch (e) {
      TestRunner.addResult('FAIL: Could not get max truncation length from first longUrl message.');
      TestRunner.completeTest();
      return;
    }
    TestRunner.runTestSuite(tests);
  });
  ConsoleTestRunner.evaluateInConsole(prepareCode);

  function consoleMessageText(index) {
    var messageElement = consoleView._visibleViewMessages[index].element();
    return messageElement.querySelector('.console-message-text').deepTextContent();
  }

  function makeSelectionAndDump(fromMessage, fromTextOffset, toMessage, toTextOffset) {
    TestRunner.addResult(
      '\nMaking selection: ' + fromMessage + ', ' + fromTextOffset + ', ' + toMessage + ', ' + toTextOffset
    );

    // Ignore the anchor text on the start/end message, just use their contents.
    var fromAnchor = consoleView.itemElement(fromMessage).element().querySelector('.console-message-anchor');
    var toAnchor = consoleView.itemElement(toMessage).element().querySelector('.console-message-anchor');
    fromTextOffset += fromAnchor ? fromAnchor.deepTextContent().length : 0;
    toTextOffset += toAnchor ? toAnchor.deepTextContent().length : 0;
    ConsoleTestRunner.selectConsoleMessages(fromMessage, fromTextOffset, toMessage, toTextOffset);
    var selectedText = viewport._selectedText();
    if (selectedText) {
      selectedText = selectedText.replace(/\bVM\d+/g, 'VM');
      TestRunner.addResult('Selection length: ' + selectedText.length + ', text: ' + selectedText);
    } else {
      TestRunner.addResult('No selection');
    }
  }

  function testHighlightedUrlWithSearchQuery(query, next) {
    // Clear any existing ranges to avoid using them as the query.
    window.getSelection().removeAllRanges();
    TestRunner.addSniffer(consoleView, '_searchFinishedForTests', onSearch);
    consoleView._searchableView._searchInputElement.value = query;
    consoleView._searchableView.showSearchField();
    TestRunner.addResult('Searching for text: ' + query);

    function onSearch() {
      var matches = consoleView.element
        .childTextNodes()
        .filter(node => node.parentElement.classList.contains('highlighted-search-result'))
        .map(node => node.parentElement);
      TestRunner.addResult('Highlighted ' + matches.length + ' matches');

      // Use TextNodes for containers to get inside the highlighted match element.
      makeSelectionAndDump(7, 0, 7, halfMaxLength);
      makeSelectionAndDump(7, 0, 7, halfMaxLength + 1);
      makeSelectionAndDump(7, 0, 7, maxLength);
      makeSelectionAndDump(7, halfMaxLength, 7, halfMaxLength + 1);
      makeSelectionAndDump(7, halfMaxLength, 7, maxLength);
      makeSelectionAndDump(7, halfMaxLength + 1, 7, maxLength);
      next();
    }
  }
})();
