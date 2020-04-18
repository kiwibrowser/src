// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

var pass = chrome.test.callbackPass;
var fail = chrome.test.callbackFail;

var tabId;
var debuggee;
var protocolVersion = "1.3";
var protocolPreviousVersion = "1.2";
var unsupportedMinorProtocolVersion = "1.5";
var unsupportedMajorProtocolVersion = "100.0";

var SILENT_FLAG_REQUIRED = "Cannot attach to this target unless " +
    "'silent-debugger-extension-api' flag is enabled.";
var DETACHED_WHILE_HANDLING = "Detached while handling command.";

chrome.test.runTests([

  function attachMalformedVersion() {
    chrome.tabs.getSelected(null, function(tab) {
      chrome.debugger.attach({tabId: tab.id}, "malformed-version", fail(
          "Requested protocol version is not supported: malformed-version."));
    });
  },

  function attachUnsupportedMinorVersion() {
    chrome.tabs.getSelected(null, function(tab) {
      chrome.debugger.attach({tabId: tab.id}, unsupportedMinorProtocolVersion,
          fail("Requested protocol version is not supported: " +
              unsupportedMinorProtocolVersion + "."));
    });
  },

  function attachUnsupportedVersion() {
    chrome.tabs.getSelected(null, function(tab) {
      chrome.debugger.attach({tabId: tab.id}, unsupportedMajorProtocolVersion,
          fail("Requested protocol version is not supported: " +
              unsupportedMajorProtocolVersion + "."));
    });
  },

  function attachPreviousVersion() {
    chrome.tabs.getSelected(null, function(tab) {
      debuggee = {tabId: tab.id};
      chrome.debugger.attach(debuggee, protocolPreviousVersion, function() {
        chrome.debugger.detach(debuggee, pass());
      });
    });
  },

  function attachLatestVersion() {
    chrome.tabs.getSelected(null, function(tab) {
      tabId = tab.id;
      debuggee = {tabId: tab.id};
      chrome.debugger.attach(debuggee, protocolVersion, pass());
    });
  },

  function attachAgain() {
    chrome.debugger.attach(debuggee, protocolVersion,
        fail("Another debugger is already attached to the tab with id: " +
            tabId + "."));
  },

  function sendCommand() {
    function onResponse() {
      if (chrome.runtime.lastError &&
          chrome.runtime.lastError.message.indexOf("invalidMethod") != -1)
        chrome.test.succeed();
      else
        chrome.test.fail();
    }
    chrome.debugger.sendCommand(debuggee,
                               "DOM.invalidMethod",
                               null,
                               onResponse);
  },

  function detach() {
    chrome.debugger.detach(debuggee, pass());
  },

  function sendCommandAfterDetach() {
    chrome.debugger.sendCommand(debuggee, "Foo", null,
        fail("Debugger is not attached to the tab with id: " + tabId + "."));
  },

  function detachAgain() {
    chrome.debugger.detach(debuggee,
        fail("Debugger is not attached to the tab with id: " + tabId + "."));
  },

  function closeTab() {
    chrome.tabs.create({url:"inspected.html"}, function(tab) {
      function onDetach(debuggee, reason) {
        chrome.test.assertEq(tab.id, debuggee.tabId);
        chrome.test.assertEq("target_closed", reason);
        chrome.debugger.onDetach.removeListener(onDetach);
        chrome.test.succeed();
      }

      var debuggee2 = {tabId: tab.id};
      chrome.debugger.attach(debuggee2, protocolVersion, function() {
        chrome.debugger.onDetach.addListener(onDetach);
        chrome.tabs.remove(tab.id);
      });
    });
  },

  function attachToWebUI() {
    chrome.tabs.create({url:"chrome://version"}, function(tab) {
      var debuggee = {tabId: tab.id};
      chrome.debugger.attach(debuggee, protocolVersion,
          fail("Cannot access a chrome:// URL"));
      chrome.tabs.remove(tab.id);
    });
  },

  function navigateToWebUI() {
    chrome.tabs.create({url:"inspected.html"}, function(tab) {
      var debuggee = {tabId: tab.id};
      chrome.debugger.attach(debuggee, protocolVersion, function() {
        var responded = false;

        function onResponse() {
          chrome.test.assertLastError(DETACHED_WHILE_HANDLING);
          responded = true;
        }

        function onDetach(from, reason) {
          chrome.debugger.onDetach.removeListener(onDetach);
          chrome.test.assertTrue(responded);
          chrome.test.assertEq(debuggee.tabId, from.tabId);
          chrome.test.assertEq("target_closed", reason);
          chrome.tabs.remove(tab.id, function() {
            chrome.test.assertNoLastError();
            chrome.test.succeed();
          });
        }

        chrome.test.assertNoLastError();
        chrome.debugger.onDetach.addListener(onDetach);
        chrome.debugger.sendCommand(
          debuggee, "Page.navigate", {url: "chrome://version"}, onResponse);
      });
    });
  },

  function detachDuringCommand() {
    chrome.tabs.create({url:"inspected.html"}, function(tab) {
      var debuggee = {tabId: tab.id};
      chrome.debugger.attach(debuggee, protocolVersion, function() {
        var responded = false;

        function onResponse() {
          chrome.test.assertLastError(DETACHED_WHILE_HANDLING);
          responded = true;
        }

        function onDetach() {
          chrome.debugger.onDetach.removeListener(onDetach);
          chrome.test.assertTrue(responded);
          chrome.tabs.remove(tab.id, function() {
            chrome.test.assertNoLastError();
            chrome.test.succeed();
          });
        }

        chrome.test.assertNoLastError();
        chrome.debugger.sendCommand(debuggee, "command", null, onResponse);
        chrome.debugger.detach(debuggee, onDetach);
      });
    });
  },

  function attachToMissing() {
    var missingDebuggee = {tabId: -1};
    chrome.debugger.attach(missingDebuggee, protocolVersion,
        fail("No tab with given id " + missingDebuggee.tabId + "."));
  },

  function attachToOwnBackgroundPageWithNoSilentFlag() {
    var ownExtensionId = chrome.extension.getURL('').split('/')[2];
    debuggee = {extensionId: ownExtensionId};
    chrome.debugger.attach(debuggee, protocolVersion, pass());
  },

  function discoverOwnBackgroundPageWithNoSilentFlag() {
    chrome.debugger.getTargets(function(targets) {
      var target = targets.filter(
          function(target) { return target.type == 'background_page'})[0];
      if (target) {
        chrome.debugger.attach({targetId: target.id}, protocolVersion, fail(
            "Another debugger is already attached to the target with id: " +
            target.id + "."));
      } else {
        chrome.test.succeed();
      }
    });
  },

  function detachFromOwnBackgroundPage() {
    chrome.debugger.detach(debuggee, pass());
  },

  function createAndDiscoverTab() {
    function onUpdated(tabId, changeInfo) {
      if (changeInfo.status == 'loading')
        return;
      chrome.tabs.onUpdated.removeListener(onUpdated);
      chrome.debugger.getTargets(function(targets) {
        var page = targets.filter(
            function(t) {
              return t.type == 'page' &&
                     t.tabId == tabId &&
                     t.title == 'Test page';
            })[0];
        if (page) {
          chrome.debugger.attach(
              {targetId: page.id}, protocolVersion, pass());
        } else {
          chrome.test.fail("Cannot discover a newly created tab");
        }
      });
    }
    chrome.tabs.onUpdated.addListener(onUpdated);
    chrome.tabs.create({url: "inspected.html"});
  },

  function discoverWorker() {
    var workerPort = new SharedWorker("worker.js").port;
    workerPort.onmessage = function() {
      chrome.debugger.getTargets(function(targets) {
        var page = targets.filter(
            function(t) { return t.type == 'worker' })[0];
        if (page) {
          debuggee = {targetId: page.id};
          chrome.debugger.attach(debuggee, protocolVersion, pass());
        } else {
          chrome.test.fail("Cannot discover a newly created worker");
        }
      });
    };
    workerPort.start();
  },

  function detachFromWorker() {
    chrome.debugger.detach(debuggee, pass());
  },

  function sendCommandDuringNavigation() {
    chrome.tabs.create({url:"inspected.html"}, function(tab) {
      var debuggee = {tabId: tab.id};

      function checkError() {
        if (chrome.runtime.lastError) {
          chrome.test.fail(chrome.runtime.lastError.message);
        } else {
          chrome.tabs.remove(tab.id);
          chrome.test.succeed();
        }
      }

      function onNavigateDone() {
        chrome.debugger.sendCommand(debuggee, "Page.disable", null, checkError);
      }

      function onAttach() {
        chrome.debugger.sendCommand(debuggee, "Page.enable");
        chrome.debugger.sendCommand(
            debuggee, "Page.navigate", {url:"about:blank"}, onNavigateDone);
      }

      chrome.debugger.attach(debuggee, protocolVersion, onAttach);
    });
  },

  function sendCommandToDataUri() {
    chrome.tabs.create({url:"data:text/html,<h1>hi</h1>"}, function(tab) {
      var debuggee = {tabId: tab.id};

      function checkError() {
        if (chrome.runtime.lastError) {
          chrome.test.fail(chrome.runtime.lastError.message);
        } else {
          chrome.tabs.remove(tab.id);
          chrome.test.succeed();
        }
      }

      function onAttach() {
        chrome.debugger.sendCommand(debuggee, "Page.enable", null, checkError);
      }

      chrome.debugger.attach(debuggee, protocolVersion, onAttach);
    });
  }
]);
