// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

(async function() {
  TestRunner.addResult(
      `Tests that distributed nodes and their updates are correctly shown in different shadow host display modes.\n`);
  await TestRunner.loadModule('elements_test_runner');
  await TestRunner.showPanel('elements');
  await TestRunner.loadHTML(`
<template id="youngestShadowRootTemplate">
    <div class="youngestShadowMain">
        <shadow></shadow>
        <content select=".distributeMeToYoungest"><div id="fallbackYoungest"></div></content>
        <div class="innerShadowHost">
            <content in-youngest-shadow-root="" select=".distributeMeToInner"></content>
        </div>
    </div>
</template>
<template id="oldestShadowRootTemplate">
    <div class="oldestShadowMain">
        <content select=".distributeMeToOldest"><div id="fallbackOldest"></div></content>
    </div>
</template>
<template id="innerShadowRootTemplate">
    <div class="innerShadowMain">
        <content in-inner-shadow-root="" select=".distributeMeToInner"></content>
    </div>
</template>
<div id="shadowHost">
    <div class="distributeMeToYoungest original">
        youngest distributed text
    </div>
    <div class="distributeMeToOldest original">
        oldest distributed text
    </div>
    <div class="distributeMeToInner original">
        oldest distributed text
    </div>
    <div class="distributeMeToInner original2">
        oldest distributed text
    </div>
</div>
    `);
  await TestRunner.evaluateInPagePromise(`
      function createShadowRootFromTemplate(root, selector, templateId)
      {
          var shadowHost = root.querySelector(selector);
          var shadowRoot = shadowHost.createShadowRoot();
          var template = document.querySelector(templateId);
          var clone = document.importNode(template.content, true);
          shadowRoot.appendChild(clone);
          return shadowHost;
      }

      function initOldestShadowRoot()
      {
          createShadowRootFromTemplate(document, "#shadowHost", "#oldestShadowRootTemplate");
      }

      function initYoungestShadowRoot()
      {
          createShadowRootFromTemplate(document, "#shadowHost", "#youngestShadowRootTemplate");
      }

      function initInnerShadowRoot()
      {
          var shadowHost = document.querySelector("#shadowHost");
          var innerShadowHost = createShadowRootFromTemplate(shadowHost.shadowRoot, ".innerShadowHost", "#innerShadowRootTemplate");
          innerShadowHost.id = "innerShadowHost";
      }

      var lastDistributedNodeId = 0;

      function addDistributedNode(oldest)
      {
          var node = document.createElement("div");
          node.classList.add(oldest ? "distributeMeToOldest" : "distributeMeToYoungest");
          node.classList.add("distributeMeAsWell_" + (++lastDistributedNodeId));
          var shadowHost = document.querySelector("#shadowHost");
          shadowHost.appendChild(node);
      }

      function addDistributedNodeToOldest()
      {
          addDistributedNode(true);
      }
  `);

  var shadowHostNode;
  var treeOutline;
  var shadowHostTreeElement;
  var innerShadowHostNode;
  var innerShadowHostTreeElement;

  ElementsTestRunner.expandElementsTree(elementsTreeExpanded);

  function elementsTreeExpanded(node) {
    treeOutline = ElementsTestRunner.firstElementsTreeOutline();
    shadowHostNode = ElementsTestRunner.expandedNodeWithId('shadowHost');
    shadowHostTreeElement = treeOutline.findTreeElement(shadowHostNode);
    expandAndDumpShadowHostNode('========= Original ========', originalElementsTreeDumped);
  }

  function originalElementsTreeDumped(node) {
    TestRunner.evaluateInPage('initOldestShadowRoot()', onOldestShadowRootInitialized);
  }

  function onOldestShadowRootInitialized() {
    expandAndDumpShadowHostNode('========= After shadow root created ========', onOldestShadowRootDumped);
  }

  function onOldestShadowRootDumped() {
    waitForModifiedNodesUpdate('After adding distributed node', distributedNodeChangedAfterFirstAdding);
    TestRunner.evaluateInPage('addDistributedNodeToOldest()');
  }

  function distributedNodeChangedAfterFirstAdding() {
    waitForModifiedNodesUpdate('After adding another distributed node', distributedNodeChangedAfterSecondAdding);
    TestRunner.evaluateInPage('addDistributedNodeToOldest()');
  }

  function distributedNodeChangedAfterSecondAdding() {
    TestRunner.completeTest();
  }

  function waitForModifiedNodesUpdate(title, next) {
    TestRunner.addSniffer(Elements.ElementsTreeOutline.prototype, '_updateModifiedNodes', callback);

    function callback() {
      expandAndDumpShadowHostNode('========= ' + title + ' ========', next);
    }
  }

  function expandAndDumpShadowHostNode(title, next) {
    TestRunner.addResult(title);
    ElementsTestRunner.expandElementsTree(callback);

    function callback() {
      ElementsTestRunner.dumpElementsTree(shadowHostNode);
      next();
    }
  }
})();
