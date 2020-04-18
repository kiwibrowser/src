// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

(async function() {
  TestRunner.addResult(`This test checks dom extensions.\n`);


  TestRunner.runTestSuite([
    function traverseNextNodeInShadowDom(next) {
      function createContent(parent, selection) {
        var content = parent.createChild('content');
        content.setAttribute('select', selection);
      }

      var component1 = createElementWithClass('div', 'component1');
      var shadow1 = component1.createShadowRoot();
      component1.createChild('div', 'component1-content').textContent = 'text 1';
      component1.createChild('div', 'component2-content').textContent = 'text 2';
      component1.createChild('span').textContent = 'text 3';
      component1.createChild('span', 'component1-content').textContent = 'text 4';

      var shadow1Content = createElementWithClass('div', 'shadow-component1');
      shadow1.appendChild(shadow1Content);
      createContent(shadow1Content, '.component1-content');
      createContent(shadow1Content, 'span');

      var component2 = shadow1Content.createChild('div', 'component2');
      var shadow2 = component2.createShadowRoot();
      createContent(component2, '.component2-content');
      component2.createChild('div', 'component2-content').textContent = 'component2 light dom text';

      var shadow2Content = createElementWithClass('div', 'shadow-component1');
      shadow2.appendChild(shadow2Content);
      var midDiv = shadow2Content.createChild('div', 'mid-div');
      midDiv.createChild('div').textContent = 'component2-text';
      createContent(midDiv, '.component2-content');

      var node = component1;
      while ((node = node.traverseNextNode(component1))) {
        if (node.nodeType === Node.TEXT_NODE)
          TestRunner.addResult(node.nodeValue);
        else
          TestRunner.addResult(node.nodeName + (node.className ? '.' + node.className : ''));
      }
      next();
    },
  ]);
})();
