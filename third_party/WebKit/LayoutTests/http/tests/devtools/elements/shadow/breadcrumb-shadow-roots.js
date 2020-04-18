// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

(async function() {
  TestRunner.addResult(`Tests that shadow roots are displayed correctly in breadcrumbs.\n`);
  await TestRunner.loadModule('elements_test_runner');
  await TestRunner.showPanel('elements');
  await TestRunner.loadHTML(`
      <!DOCTYPE HTML PUBLIC "-//W3C//DTD HTML 4.01 Transitional//EN" "http://www.w3.org/TR/html4/loose.dtd">
      <input type="text">
      <div id="host"></div>
      <div id="hostClosed"></div>
      <template id="tmpl">
          <style>.red { color: red; }</style>
          <div id="inner" class="red">inner</div>
      </template>
    `);
  await TestRunner.evaluateInPagePromise(`
      var template = document.querySelector("#tmpl");
      var root = document.querySelector("#host").createShadowRoot();
      root.appendChild(template.content.cloneNode(true));
      var rootClosed = document.querySelector("#hostClosed").attachShadow({mode: 'closed'});
      rootClosed.appendChild(template.content.cloneNode(true));
  `);

  Common.settingForTest('showUAShadowDOM').set(true);
  ElementsTestRunner.expandElementsTree(step0);

  function step0() {
    selectNode(matchUserAgentShadowRoot, step1);
  }

  function step1() {
    ElementsTestRunner.dumpBreadcrumb('User-agent shadow root breadcrumb');
    selectNode(matchOpenShadowRoot, step2);
  }

  function step2() {
    ElementsTestRunner.dumpBreadcrumb('Open shadow root breadcrumb');
    selectNode(matchClosedShadowRoot, step3);
  }

  function step3() {
    ElementsTestRunner.dumpBreadcrumb('Closed shadow root breadcrumb');
    TestRunner.completeTest();
  }

  function selectNode(matchFunction, next) {
    ElementsTestRunner.findNode(matchFunction, callback);
    function callback(node) {
      TestRunner.addSniffer(Elements.ElementsBreadcrumbs.prototype, 'update', next);
      Common.Revealer.reveal(node);
    }
  }

  function matchUserAgentShadowRoot(node) {
    return node.shadowRootType() === SDK.DOMNode.ShadowRootTypes.UserAgent;
  }

  function matchOpenShadowRoot(node) {
    return node.shadowRootType() === SDK.DOMNode.ShadowRootTypes.Open;
  }

  function matchClosedShadowRoot(node) {
    return node.shadowRootType() === SDK.DOMNode.ShadowRootTypes.Closed;
  }
})();
