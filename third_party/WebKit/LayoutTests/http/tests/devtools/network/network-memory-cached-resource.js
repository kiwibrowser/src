// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

(async function() {
  TestRunner.addResult(`Tests that memory-cached resources are correctly reported.\n`);
  await TestRunner.loadModule('network_test_runner');
  await TestRunner.showPanel('network');
  await TestRunner.navigatePromise(`resources/memory-cached-resource.html`);

  var finished = false;
  TestRunner.NetworkAgent.setCacheDisabled(true).then(step1);

  function findResource(url, status, cached) {
    return NetworkTestRunner.networkRequests().find(
        request => url.test(request.url()) && (status === request.statusCode) && (cached === request.cached()));
  }

  function step1() {
    TestRunner.networkManager.addEventListener(SDK.NetworkManager.Events.RequestFinished, onRequest);
    TestRunner.reloadPage(step2);
  }

  function step2() {
    TestRunner.addIframe('memory-cached-resource.html');
  }

  function onRequest() {
    if (!finished && findResource(/abe\.png/, 200, false) && findResource(/abe\.png/, 200, true)) {
      finished = true;
      TestRunner.addResult('Memory-cached resource found.');
      step3();
    }
  }

  function step3() {
    TestRunner.NetworkAgent.setCacheDisabled(false).then(step4);
  }

  function step4() {
    TestRunner.completeTest();
  }
})();
