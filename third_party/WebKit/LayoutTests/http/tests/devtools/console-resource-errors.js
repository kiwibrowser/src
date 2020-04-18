// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

(async function() {
  TestRunner.addResult(`Tests that errors to load a resource cause error messages to be logged to console.\n`);
  await TestRunner.loadModule('console_test_runner');
  await TestRunner.showPanel('console');
  await TestRunner.evaluateInPagePromise(`
      function performActions()
      {
          loadXHR();
          loadIframe();
      }

      function loadXHR()
      {
          var xhr = new XMLHttpRequest();
          xhr.open("GET","non-existent-xhr", false);
          xhr.send(null);
      }

      function loadIframe()
      {
          var iframe = document.createElement("iframe");
          iframe.src = "resources/console-resource-errors-iframe.html";
          document.body.appendChild(iframe);
      }
  `);

  ConsoleTestRunner.addConsoleViewSniffer(addMessage, true);
  async function addMessage(uiMessage) {
    var element = await uiMessage.completeElementForTest();
    // There will be only one such message.
    if (element.deepTextContent().indexOf('non-existent-iframe') !== -1)
      ConsoleTestRunner.expandConsoleMessages(onExpandedMessages);
  }

  function onExpandedMessages() {
    ConsoleTestRunner.dumpConsoleMessagesWithClasses(true);
    TestRunner.completeTest();
  }

  TestRunner.evaluateInPage('performActions()');
})();
