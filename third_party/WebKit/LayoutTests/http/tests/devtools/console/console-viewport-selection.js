// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

(async function() {
  TestRunner.addResult(`Tests that console viewport handles selection properly.\n`);
  await TestRunner.loadModule('console_test_runner');
  await TestRunner.showPanel('console');
  await TestRunner.evaluateInPagePromise(`
      function populateConsoleWithMessages(count)
      {
          for (var i = 0; i < count - 1; ++i)
              console.log("Message #" + i);
          console.log("hello %cworld", "color: blue");
      }
      //# sourceURL=console-viewport-selection.js
    `);

  ConsoleTestRunner.fixConsoleViewportDimensions(600, 200);
  var consoleView = Console.ConsoleView.instance();
  var viewport = consoleView._viewport;
  const minimumViewportMessagesCount = 10;
  const messagesCount = 150;
  const middleMessage = messagesCount / 2;
  var viewportMessagesCount;

  var testSuite = [
    function testSelectionSingleLineText(next) {
      viewport.invalidate();
      viewport.forceScrollItemToBeFirst(0);
      viewportMessagesCount = viewport.lastVisibleIndex() - viewport.firstVisibleIndex() + 1;
      selectMessages(middleMessage, 2, middleMessage, 7);
      dumpSelectionText();
      dumpViewportRenderedItems();
      next();
    },

    function testReversedSelectionSingleLineText(next) {
      selectMessages(middleMessage, 7, middleMessage, 2);
      dumpSelectionText();
      dumpViewportRenderedItems();
      next();
    },

    function testSelectionMultiLineText(next) {
      selectMessages(middleMessage - 1, 4, middleMessage + 1, 7);
      dumpSelectionText();
      dumpViewportRenderedItems();
      next();
    },

    function testSimpleVisibleSelection(next) {
      selectMessages(middleMessage - 3, 6, middleMessage + 2, 6);
      dumpSelectionModel();
      dumpViewportRenderedItems();
      next();
    },

    function testHalfScrollSelectionUp(next) {
      viewport.forceScrollItemToBeFirst(middleMessage);
      dumpSelectionModel();
      dumpViewportRenderedItems();
      next();
    },

    function testHalfScrollSelectionDown(next) {
      viewport.forceScrollItemToBeLast(middleMessage);
      dumpSelectionModel();
      dumpViewportRenderedItems();
      next();
    },

    function testScrollSelectionAwayUp(next) {
      viewport.forceScrollItemToBeFirst(0);
      dumpSelectionModel();
      dumpViewportRenderedItems();
      next();
    },

    function testScrollSelectionAwayDown(next) {
      consoleView._immediatelyScrollToBottom();
      viewport.refresh();
      dumpSelectionModel();
      dumpViewportRenderedItems();
      next();
    },

    function testShiftClickSelectionOver(next) {
      emulateShiftClickOnMessage(minimumViewportMessagesCount);
      dumpSelectionModel();
      dumpViewportRenderedItems();
      next();
    },

    function testShiftClickSelectionBelow(next) {
      emulateShiftClickOnMessage(messagesCount - minimumViewportMessagesCount);
      dumpSelectionModel();
      dumpViewportRenderedItems();
      next();
    },

    function testRemoveSelection(next) {
      var selection = window.getSelection();
      selection.removeAllRanges();
      dumpSelectionModel();
      dumpViewportRenderedItems();
      next();
    },

    function testReversedVisibleSelection(next) {
      selectMessages(middleMessage + 1, 6, middleMessage - 4, 6);
      dumpSelectionModel();
      dumpViewportRenderedItems();
      next();
    },

    function testShiftClickReversedSelectionOver(next) {
      emulateShiftClickOnMessage(minimumViewportMessagesCount);
      dumpSelectionModel();
      dumpViewportRenderedItems();
      next();
    },

    function testShiftClickReversedSelectionBelow(next) {
      emulateShiftClickOnMessage(messagesCount - minimumViewportMessagesCount);
      dumpSelectionModel();
      dumpViewportRenderedItems();
      next();
    },

    function testZeroOffsetSelection(next) {
      viewport.forceScrollItemToBeLast(messagesCount - 1);
      var lastMessageElement = viewport.renderedElementAt(messagesCount - 1);
      // there is a blue-colored "world" span in last message.
      var blueSpan = lastMessageElement;
      while (blueSpan.nodeName !== 'SPAN' || blueSpan.textContent !== 'world')
        blueSpan = blueSpan.traverseNextNode();

      window.getSelection().setBaseAndExtent(blueSpan, 0, blueSpan, blueSpan.childNodes.length);
      TestRunner.addResult('Selected text: ' + viewport._selectedText());
      next();
    },

    function testSelectAll(next) {
      viewport.forceScrollItemToBeFirst(0);

      // Set some initial selection in console.
      var base = consoleView.itemElement(messagesCount - 2).element();
      var extent = consoleView.itemElement(messagesCount - 1).element();
      window.getSelection().setBaseAndExtent(base, 0, extent, 0);

      // Try to select all messages.
      document.execCommand('selectAll');

      var text = viewport._selectedText();
      var count = text ? text.split('\n').length : 0;
      TestRunner.addResult(
          count === messagesCount ? 'Selected all ' + count + ' messages.' :
                                    'Selected ' + count + ' messages instead of ' + messagesCount);
      next();
    },

    function testSelectWithNonTextNodeContainer(next) {
      viewport.forceScrollItemToBeFirst(0);

      var nonTextNodeBase = consoleView.itemElement(1).element();
      var nonTextNodeExtent = consoleView.itemElement(2).element();
      var textNodeBase = consoleView.itemElement(1).element().traverseNextTextNode();
      var textNodeExtent = consoleView.itemElement(2).element().traverseNextTextNode();

      window.getSelection().setBaseAndExtent(nonTextNodeBase, 0, nonTextNodeExtent, 0);
      TestRunner.addResult('Selected text: ' + viewport._selectedText());

      window.getSelection().setBaseAndExtent(textNodeBase, 0, nonTextNodeExtent, 0);
      TestRunner.addResult('Selected text: ' + viewport._selectedText());

      window.getSelection().setBaseAndExtent(nonTextNodeBase, 0, textNodeExtent, 0);
      TestRunner.addResult('Selected text: ' + viewport._selectedText());

      next();
    }
  ];

  var awaitingMessagesCount = messagesCount;
  function messageAdded() {
    if (!--awaitingMessagesCount)
      TestRunner.runTestSuite(testSuite);
  }

  ConsoleTestRunner.addConsoleSniffer(messageAdded, true);
  TestRunner.evaluateInPage(String.sprintf('populateConsoleWithMessages(%d)', messagesCount));

  function dumpSelectionModelElement(model) {
    if (!model)
      return 'null';
    return String.sprintf('{item: %d, offset: %d}', model.item, model.offset);
  }

  function dumpSelectionModel() {
    viewport.refresh();
    var text = String.sprintf(
        'anchor = %s, head = %s', dumpSelectionModelElement(viewport._anchorSelection),
        dumpSelectionModelElement(viewport._headSelection));
    TestRunner.addResult(text);
  }

  function dumpSelectionText() {
    viewport.refresh();
    var text = viewport._selectedText();
    TestRunner.addResult('Selected text:<<<EOL\n' + text + '\nEOL');
  }

  function dumpViewportRenderedItems() {
    viewport.refresh();
    var firstVisibleIndex = viewport.firstVisibleIndex();
    var lastVisibleIndex = viewport.lastVisibleIndex();
    TestRunner.addResult('first visible message index: ' + firstVisibleIndex);
  }

  function emulateShiftClickOnMessage(messageIndex) {
    viewport.refresh();
    var selection = window.getSelection();
    if (!selection || !selection.rangeCount) {
      TestRunner.addResult('FAILURE: There\'s no selection');
      return;
    }
    viewport.forceScrollItemToBeFirst(Math.max(messageIndex - minimumViewportMessagesCount / 2, 0));
    var element = consoleView.itemElement(messageIndex).element();
    selection.setBaseAndExtent(selection.anchorNode, selection.anchorOffset, element, 0);
    viewport.refresh();
  }

  function selectMessages(fromMessage, fromTextOffset, toMessage, toTextOffset) {
    if (Math.abs(toMessage - fromMessage) > minimumViewportMessagesCount) {
      TestRunner.addResult(String.sprintf(
          'FAILURE: Cannot select more than %d messages (requested to select from %d to %d',
          minimumViewportMessagesCount, fromMessage, toMessage));
      TestRunner.completeTest();
      return;
    }
    viewport.forceScrollItemToBeFirst(Math.min(fromMessage, toMessage));

    ConsoleTestRunner.selectConsoleMessages(fromMessage, fromTextOffset, toMessage, toTextOffset);
    viewport.refresh();
  }
})();
