// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

(async function() {
  TestRunner.addResult(`Tests object revelation in the UI.\n`);
  await TestRunner.loadModule('elements_test_runner');
  await TestRunner.loadModule('network_test_runner');
  await TestRunner.showPanel('elements');
  await TestRunner.showPanel('sources');
  await TestRunner.showPanel('resources');
  await TestRunner.showPanel('network');
  await TestRunner.loadHTML(`
      <div id="div">
      </div>
    `);
  await TestRunner.evaluateInPagePromise(`
      function loadResource(url)
      {
          var xhr = new XMLHttpRequest();
          xhr.open("GET", url, false);
          xhr.send();
      }
  `);
  await TestRunner.addScriptTag('resources/bar.js');

  var node;
  var resource;
  var uiLocation;
  var requestWithResource;
  var requestWithoutResource;

  TestRunner.runTestSuite([
    function init(next) {
      installHooks();

      TestRunner.resourceTreeModel.forAllResources(function(r) {
        if (r.url.indexOf('bar.js') !== -1) {
          resource = r;
          return true;
        }
      });
      uiLocation = Workspace.workspace.uiSourceCodeForURL(resource.url).uiLocation(2, 1);

      ElementsTestRunner.nodeWithId('div', nodeCallback);

      function nodeCallback(foundNode) {
        node = foundNode;
        NetworkTestRunner.recordNetwork();
        var url = TestRunner.url('bar.js');
        TestRunner.evaluateInPage(`loadResource('${url}')`, firstXhrCallback);
      }

      function firstXhrCallback() {
        requestWithResource = BrowserSDK.networkLog.requestForURL(resource.url);
        TestRunner.evaluateInPage('loadResource(\'missing.js\')', secondXhrCallback);
      }

      function secondXhrCallback() {
        var requests = NetworkTestRunner.networkRequests();
        for (var i = 0; i < requests.length; ++i) {
          if (requests[i].url().indexOf('missing.js') !== -1) {
            requestWithoutResource = requests[i];
            break;
          }
        }
        next();
      }
    },

    function revealNode(next) {
      Common.Revealer.reveal(node).then(next);
    },

    function revealUILocation(next) {
      Common.Revealer.reveal(uiLocation).then(next);
    },

    function revealResource(next) {
      Common.Revealer.reveal(resource).then(next);
    },

    function revealRequestWithResource(next) {
      Common.Revealer.reveal(requestWithResource).then(next);
    },

    function revealRequestWithoutResource(next) {
      Common.Revealer.reveal(requestWithoutResource).then(next);
    }
  ]);

  function installHooks() {
    TestRunner.addSniffer(Elements.ElementsPanel.prototype, 'revealAndSelectNode', nodeRevealed, true);
    TestRunner.addSniffer(Sources.SourcesPanel.prototype, 'showUILocation', uiLocationRevealed, true);
    TestRunner.addSniffer(Resources.ApplicationPanelSidebar.prototype, 'showResource', resourceRevealed, true);
    TestRunner.addSniffer(Network.NetworkPanel.prototype, 'revealAndHighlightRequest', revealed, true);
  }

  function nodeRevealed(node) {
    TestRunner.addResult('Node revealed in the Elements panel');
  }

  function uiLocationRevealed(uiLocation) {
    TestRunner.addResult(
        'UILocation ' + uiLocation.uiSourceCode.name() + ':' + uiLocation.lineNumber + ':' + uiLocation.columnNumber +
        ' revealed in the Sources panel');
  }

  function resourceRevealed(resource, lineNumber) {
    TestRunner.addResult('Resource ' + resource.displayName + ' revealed in the Resources panel');
  }

  function revealed(request) {
    TestRunner.addResult(
        'Request ' + new Common.ParsedURL(request.url()).lastPathComponent + ' revealed in the Network panel');
  }
})();
