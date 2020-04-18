// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

(async function() {
  TestRunner.addResult(`Tests that pseudo elements and their styles are handled properly.\n`);
  await TestRunner.loadModule('elements_test_runner');
  await TestRunner.showPanel('elements');
  await TestRunner.loadHTML(`
      <style>
      #inspected:before, .some-other-selector {
        content: "BEFORE";
      }

      #inspected:after {
        content: "AFTER";
      }
      </style>
      <style>
      #empty::before {
        content: "EmptyBefore";
      }

      #empty::after {
        content: "EmptyAfter";
      }
      </style>
      <div id="container">
          <div id="inspected">Text</div>
          <div id="empty"></div>
      </div>
    `);
  await TestRunner.evaluateInPagePromise(`
      function removeLastRule()
      {
          document.styleSheets[0].deleteRule(document.styleSheets[0].cssRules.length - 1);
      }

      function addAfterRule()
      {
          document.styleSheets[0].addRule("#inspected:after", "content: \\"AFTER\\"");
      }

      function addBeforeRule()
      {
          document.styleSheets[0].addRule("#inspected:before", "content: \\"BEFORE\\"");
      }

      function modifyTextContent()
      {
          document.getElementById("inspected").textContent = "bar";
      }

      function clearTextContent()
      {
          document.getElementById("inspected").textContent = "";
      }

      function removeNode()
      {
          document.getElementById("inspected").remove();
      }
  `);

  var containerNode;
  var inspectedNode;

  TestRunner.runTestSuite([
    function dumpOriginalTree(next) {
      ElementsTestRunner.expandElementsTree(callback);
      function callback() {
        containerNode = ElementsTestRunner.expandedNodeWithId('container');
        inspectedNode = ElementsTestRunner.expandedNodeWithId('inspected');
        TestRunner.addResult('Original elements tree:');
        ElementsTestRunner.dumpElementsTree(containerNode);
        next();
      }
    },

    function dumpNormalNodeStyles(next) {
      selectNodeAndDumpStyles('inspected', '', next);
    },

    function dumpBeforeStyles(next) {
      selectNodeAndDumpStyles('inspected', 'before', next);
    },

    function dumpAfterStyles(next) {
      selectNodeAndDumpStyles('inspected', 'after', next);
    },

    function removeAfter(next) {
      executeAndDumpTree('removeLastRule()', SDK.DOMModel.Events.NodeRemoved, next);
    },

    function removeBefore(next) {
      executeAndDumpTree('removeLastRule()', SDK.DOMModel.Events.NodeRemoved, next);
    },

    function addAfter(next) {
      executeAndDumpTree('addAfterRule()', SDK.DOMModel.Events.NodeInserted, expandAndDumpTree.bind(this, next));
    },

    function addBefore(next) {
      executeAndDumpTree('addBeforeRule()', SDK.DOMModel.Events.NodeInserted, next);
    },

    function modifyTextContent(next) {
      executeAndDumpTree('modifyTextContent()', SDK.DOMModel.Events.NodeInserted, next);
    },

    function clearTextContent(next) {
      executeAndDumpTree('clearTextContent()', SDK.DOMModel.Events.NodeRemoved, next);
    },

    function removeNodeAndCheckPseudoElementsUnbound(next) {
      var inspectedBefore = inspectedNode.beforePseudoElement();
      var inspectedAfter = inspectedNode.afterPseudoElement();

      executeAndDumpTree('removeNode()', SDK.DOMModel.Events.NodeRemoved, callback);
      function callback() {
        TestRunner.addResult(
            'inspected:before DOMNode in DOMAgent: ' + !!(TestRunner.domModel.nodeForId(inspectedBefore.id)));
        TestRunner.addResult(
            'inspected:after DOMNode in DOMAgent: ' + !!(TestRunner.domModel.nodeForId(inspectedAfter.id)));
        next();
      }
    }
  ]);

  function executeAndDumpTree(pageFunction, eventName, next) {
    TestRunner.domModel.addEventListener(eventName, domCallback, this);
    TestRunner.evaluateInPage(pageFunction);

    function domCallback() {
      TestRunner.domModel.removeEventListener(eventName, domCallback, this);
      ElementsTestRunner.firstElementsTreeOutline().addEventListener(
          Elements.ElementsTreeOutline.Events.ElementsTreeUpdated, treeCallback, this);
    }

    function treeCallback() {
      ElementsTestRunner.firstElementsTreeOutline().removeEventListener(
          Elements.ElementsTreeOutline.Events.ElementsTreeUpdated, treeCallback, this);
      ElementsTestRunner.dumpElementsTree(containerNode);
      next();
    }
  }

  function expandAndDumpTree(next) {
    TestRunner.addResult('== Expanding: ==');
    ElementsTestRunner.expandElementsTree(callback);
    function callback() {
      ElementsTestRunner.dumpElementsTree(containerNode);
      next();
    }
  }

  function selectNodeAndDumpStyles(id, pseudoTypeName, callback) {
    if (pseudoTypeName)
      ElementsTestRunner.selectPseudoElementAndWaitForStyles('inspected', pseudoTypeName, stylesCallback);
    else
      ElementsTestRunner.selectNodeAndWaitForStyles('inspected', stylesCallback);

    function stylesCallback() {
      ElementsTestRunner.dumpSelectedElementStyles(true, false, false, true);
      callback();
    }
  }
})();
