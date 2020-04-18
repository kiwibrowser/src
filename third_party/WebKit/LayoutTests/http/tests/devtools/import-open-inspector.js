// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

(async function() {
  TestRunner.addResult(
      `This tests that reloading a page with the inspector opened does not crash (rewritten test from r156199).\n`);

  await TestRunner.evaluateInPageAsync(`
    (function(){
      var link = document.createElement('link');
      link.rel = 'import';
      link.href = 'resources/import-open-inspector-linked.html';
      document.head.append(link);
      return new Promise(f => link.onload = f);
    })();
  `);

  await TestRunner.evaluateInPagePromise(`
      function getGreeting()
      {
          return window.greeting;
      }
  `);

  TestRunner.runTestSuite([
    function checkGreetingSet(next) {
      TestRunner.evaluateInPage('getGreeting()', callback);
      function callback(result) {
        TestRunner.addResult('Received: ' + result);
        next();
      }
    },

    function reloadPage(next) {
      TestRunner.reloadPage(next);
    },

    function checkReloaded(next) {
      TestRunner.addResult('Page successfully reloaded');
      next();
    }
  ]);
})();
