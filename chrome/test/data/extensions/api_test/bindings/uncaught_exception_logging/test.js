// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

function createTestFunction(expected_message) {
  return function(tab) {
    function onDebuggerEvent(debuggee, method, params) {
      if (debuggee.tabId == tab.id && method == 'Runtime.exceptionThrown') {
        var exception = params.exceptionDetails.exception;
        if (exception.value.indexOf(expected_message) > -1) {
          chrome.debugger.onEvent.removeListener(onDebuggerEvent);
          chrome.test.succeed();
        }
      }
    };
    chrome.debugger.onEvent.addListener(onDebuggerEvent);
    chrome.debugger.attach({ tabId: tab.id }, "1.1", function() {
      // Enabling console provides both stored and new messages via the
      // Console.messageAdded event.
      chrome.debugger.sendCommand({ tabId: tab.id }, "Runtime.enable");
    });
  }
}

chrome.test.runTests([
  function testExceptionInExtensionPage() {
    chrome.tabs.create(
        {url: chrome.runtime.getURL('extension_page.html')},
        createTestFunction('Exception thrown in extension page.'));
  },

  function testExceptionInInjectedScript() {
    function injectScriptAndSendMessage(tab) {
      chrome.tabs.executeScript(
          tab.id,
          { file: 'content_script.js' },
          function() {
            createTestFunction('Exception thrown in injected script.')(tab);
          });
    }

    chrome.test.getConfig(function(config) {
      var test_url =
          'http://localhost:PORT/extensions/test_file.html'
              .replace(/PORT/, config.testServer.port);

      chrome.tabs.create({ url: test_url }, injectScriptAndSendMessage);
    });
  }
]);
