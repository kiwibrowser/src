// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

(async function() {
  TestRunner.addResult('Tests that console logging large messages will be truncated.\n');

  await TestRunner.loadModule('console_test_runner');
  await TestRunner.showPanel('console');
  var consoleView = Console.ConsoleView.instance();

  var maxLength = Console.ConsoleViewMessage._MaxTokenizableStringLength = 40;
  var visibleLength = Console.ConsoleViewMessage._LongStringVisibleLength = 20;
  var overMaxLength = maxLength * 2;
  TestRunner.addResult(`Setting max length to: ${maxLength}`);
  TestRunner.addResult(`Setting long string visible length to: ${visibleLength}`);

  await ConsoleTestRunner.evaluateInConsolePromise(`"a".repeat(${overMaxLength})`);
  await TestRunner.evaluateInPagePromise(`console.log("a".repeat(${overMaxLength}))`);
  await TestRunner.evaluateInPagePromise(`console.log("a".repeat(${overMaxLength}), "b".repeat(${overMaxLength}))`);
  await TestRunner.evaluateInPagePromise(`console.log("%s", "a".repeat(${overMaxLength}))`);
  await TestRunner.evaluateInPagePromise(`console.log("%o", "a".repeat(${overMaxLength}))`);
  await TestRunner.evaluateInPagePromise(`console.log("%c" + "a".repeat(${overMaxLength}), "color: green")`);
  await TestRunner.evaluateInPagePromise(`console.log("foo %s %o bar", "a".repeat(${overMaxLength}), {a: 1})`);
  await TestRunner.evaluateInPagePromise(`console.log({a: 1}, "a".repeat(${overMaxLength}), {b: 1})`);
  await TestRunner.evaluateInPagePromise(`console.log("a".repeat(${overMaxLength}), "https://chromium.org")`);
  await TestRunner.evaluateInPagePromise(`console.log("https://chromium.org", "a".repeat(${overMaxLength}))`);
  await TestRunner.evaluateInPagePromise(`console.log(RegExp("a".repeat(${overMaxLength})))`);
  await TestRunner.evaluateInPagePromise(`console.log(Symbol("a".repeat(${overMaxLength})))`);

  dumpMessageLengths();

  TestRunner.addResult('\nExpanding hidden texts');
  consoleView._visibleViewMessages.forEach(message => {
    message.element().querySelectorAll('.console-inline-button').forEach(button => button.click());
  });

  dumpMessageLengths();
  TestRunner.completeTest();

  function dumpMessageLengths() {
    consoleView._visibleViewMessages.forEach((message, index) => {
      var text = consoleMessageText(index);
      TestRunner.addResult(`Message: ${index}, length: ${text.length}, ${text}`);
    });

    function consoleMessageText(index) {
      var messageElement = consoleView._visibleViewMessages[index].element();
      var anchor = messageElement.querySelector('.console-message-anchor');
      if (anchor)
        anchor.remove();
      var links = messageElement.querySelectorAll('.devtools-link');
      for (var link of links)
        TestRunner.addResult(`Link: ${link.textContent}`);
      return messageElement.deepTextContent();
    }
  }
})();
