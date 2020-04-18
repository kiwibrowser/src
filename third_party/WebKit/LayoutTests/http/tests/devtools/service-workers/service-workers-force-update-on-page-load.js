// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

(async function() {
  TestRunner.addResult(`Tests "Force update on page load" checkbox\n`);
  await TestRunner.loadModule('application_test_runner');
  // Note: every test that uses a storage API must manually clean-up state from previous tests.
  await ApplicationTestRunner.resetState();

  await TestRunner.showPanel('resources');

  const scriptURL = 'http://127.0.0.1:8000/devtools/service-workers/resources/force-update-on-page-load-worker.php';
  const scope = 'http://127.0.0.1:8000/devtools/service-workers/resources/service-worker-force-update-on-page-load/';

  function waitForWorkerActivated(scope) {
    return new Promise(function(resolve) {
      TestRunner.addSniffer(Resources.ServiceWorkersView.prototype, '_updateRegistration', updateRegistration, false);
      function updateRegistration(registration) {
        if (registration.scopeURL == scope) {
          for (var version of registration.versions.values()) {
            if (version.isRunning() && version.isActivated()) {
              resolve();
              return;
            }
          }
        }
        TestRunner.addSniffer(Resources.ServiceWorkersView.prototype, '_updateRegistration', updateRegistration, false);
      }
    });
  }
  function installNewWorkerDetector(scope) {
    var workerIdSet = {};
    TestRunner.addSniffer(Resources.ServiceWorkersView.prototype, '_updateRegistration', updateRegistration, true);
    function updateRegistration(registration) {
      if (registration.scopeURL == scope) {
        for (var version of registration.versions.values()) {
          if (!workerIdSet[version.id] && version.isRunning() && version.isActivated()) {
            workerIdSet[version.id] = true;
            TestRunner.addResult('A new ServiceWorker is activated.');
          }
        }
      }
    }
  }
  installNewWorkerDetector(scope);
  UI.inspectorView.showPanel('sources')
      .then(() => waitForWorkerActivated(scope))
      .then(function() {
        TestRunner.addResult('The first ServiceWorker is activated.');
        return TestRunner.addIframe(scope);
      })
      .then(function() {
        TestRunner.addResult('The first frame loaded.');
        return TestRunner.addIframe(scope);
      })
      .then(function() {
        TestRunner.addResult('The second frame loaded.');
        TestRunner.addResult('Check "Force update on page load" check box');
        Common.settings.settingForTest('serviceWorkerUpdateOnReload').set(true);
        return TestRunner.addIframe(scope);
      })
      .then(function() {
        TestRunner.addResult('The third frame loaded. The second worker must be activated before here.');
        return TestRunner.addIframe(scope);
      })
      .then(function() {
        TestRunner.addResult('The fourth frame loaded.  The third worker must be activated before here.');
        TestRunner.addResult('Uncheck "Force update on page load" check box');
        Common.settings.settingForTest('serviceWorkerUpdateOnReload').set(false);
        return TestRunner.addIframe(scope);
      })
      .then(function() {
        TestRunner.addResult('The fifth frame loaded.');
        ApplicationTestRunner.deleteServiceWorkerRegistration(scope);
        TestRunner.completeTest();
      })
      .catch(function(exception) {
        TestRunner.addResult('Error');
        TestRunner.addResult(exception);
        ApplicationTestRunner.deleteServiceWorkerRegistration(scope);
        TestRunner.completeTest();
      });
  UI.panels.resources._sidebar.serviceWorkersTreeElement.select();
  ApplicationTestRunner.registerServiceWorker(scriptURL, scope);
})();
