// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

(async function() {
    TestRunner.addResult(`Verifies viewport's visible and active message ranges.\n`);
    await TestRunner.loadModule('console_test_runner');
    await TestRunner.showPanel('console');
    await TestRunner.evaluateInPagePromise(`
        function addMessages(count)
        {
            for (var i = 0; i < count; ++i)
                console.log("Message #" + i);
        }

        //# sourceURL=console-viewport-indices.js
      `);

    ConsoleTestRunner.fixConsoleViewportDimensions(600, 200);
    var consoleView = Console.ConsoleView.instance();
    var viewport = consoleView._viewport;

    function logMessages(count) {
      TestRunner.addResult('Logging ' + count + ' messages');
      return new Promise(resolve => {
        var awaitingMessagesCount = count;
        function messageAdded() {
          if (!--awaitingMessagesCount) {
            viewport.invalidate();
            resolve();
          } else {
            ConsoleTestRunner.addConsoleSniffer(messageAdded, false);
          }
        }
        ConsoleTestRunner.addConsoleSniffer(messageAdded, false);
        TestRunner.evaluateInPage(String.sprintf('addMessages(%d)', count));
      });
    }

    function dumpVisibleIndices() {
      var {first, last, count} = ConsoleTestRunner.visibleIndices();
      var activeTotal = viewport._firstActiveIndex === -1 ? 0 : (viewport._lastActiveIndex - viewport._firstActiveIndex + 1);
      var calculatedFirst = viewport.firstVisibleIndex();
      var calculatedLast = viewport.lastVisibleIndex();
      var calculatedTotal = calculatedFirst === -1 ? 0 : (calculatedLast - calculatedFirst + 1);
      TestRunner.addResult(`Calculated visible range: ${calculatedFirst} to ${calculatedLast}, Total: ${calculatedTotal}
Actual visible range: ${first} to ${last}, Total: ${count}
Active range: ${viewport._firstActiveIndex} to ${viewport._lastActiveIndex}, Total: ${activeTotal}\n`);
      if (calculatedFirst !== first || calculatedLast !== last) {
        TestRunner.addResult('TEST ENDED IN ERROR: viewport is calculated incorrect visible indices!');
        TestRunner.completeTest();
      }
    }

    TestRunner.runTestSuite([
      async function testEmptyViewport(next) {
        Console.ConsoleView.clearConsole();
        dumpVisibleIndices();
        next();
      },

      async function testFirstLastVisibleIndices(next) {
        Console.ConsoleView.clearConsole();
        await logMessages(100, false);

        forceItemAndDump(0, true);
        forceItemAndDump(1, true);

        var lessThanOneRowHeight = consoleView.minimumRowHeight() - 1;
        TestRunner.addResult(`Scroll a bit down: ${lessThanOneRowHeight}px`);
        viewport.element.scrollTop += lessThanOneRowHeight;
        viewport.refresh();
        dumpVisibleIndices();

        forceItemAndDump(50, false);
        forceItemAndDump(99, false);
        next();

        function forceItemAndDump(index, first) {
          TestRunner.addResult(`Force item to be ${first ? 'first' : 'last'}: ${index}`);
          if (first)
            viewport.forceScrollItemToBeFirst(index);
          else
            viewport.forceScrollItemToBeLast(index);
          dumpVisibleIndices();
        }
      }
    ]);
  })();
