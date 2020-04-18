// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

(async function() {
  // about:blank never fires a load event so just wait until we see the URL change
  function navigateToAboutBlankAndWait() {
    var listenerPromise = new Promise(resolve => {
      SDK.targetManager.addEventListener(SDK.TargetManager.Events.InspectedURLChanged, resolve);
    });

    TestRunner.navigate('about:blank');
    return listenerPromise;
  }

  TestRunner.addResult('Tests that audits panel prevents run of unauditable pages.\n');

  await TestRunner.loadModule('audits2_test_runner');
  await TestRunner.showPanel('audits2');

  TestRunner.addResult('**Prevents audit with no categories**');
  Audits2TestRunner.openStartAudit();
  var containerElement = Audits2TestRunner.getContainerElement();
  var checkboxes = containerElement.querySelectorAll('.checkbox');
  checkboxes.forEach(checkbox => checkbox.checkboxElement.click());
  Audits2TestRunner.dumpStartAuditState();

  TestRunner.addResult('**Allows audit with a single category**');
  checkboxes[0].checkboxElement.click();
  Audits2TestRunner.dumpStartAuditState();

  TestRunner.addResult('**Prevents audit on undockable page**');
  // The content shell of the test runner is an undockable page that would normally always show the error
  // Temporarily fool the audits panel into thinking we're not under test so the validation logic will be triggered.
  Host.isUnderTest = () => false;
  Audits2TestRunner.forcePageAuditabilityCheck();
  Audits2TestRunner.dumpStartAuditState();
  // Reset our test state
  Host.isUnderTest = () => true;
  Audits2TestRunner.forcePageAuditabilityCheck();

  TestRunner.addResult('**Prevents audit on internal page**');
  await navigateToAboutBlankAndWait();
  TestRunner.addResult(`URL: ${TestRunner.mainTarget.inspectedURL()}`);
  Audits2TestRunner.dumpStartAuditState();

  TestRunner.completeTest();
})();
