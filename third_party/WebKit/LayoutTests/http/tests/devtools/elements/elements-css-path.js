// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

(async function() {
  TestRunner.addResult(`Tests DOMNode.cssPath()\n`);
  await TestRunner.loadModule('elements_test_runner');
  await TestRunner.showPanel('elements');
  await TestRunner.loadHTML(`
      <!DOCTYPE html>
      <article></article>
      <article></article>
      <input type="number">

      <!-- Comment node -->

      <div id="ids">
          <!-- Comment node -->

          <div></div>
          <div id="inner-id"></div>
          <div id="__proto__"></div>
          <div id="#&quot;ridiculous&quot;.id"></div>
          <div id="'quoted.value'"></div>
          <div id=".foo.bar"></div>
          <div id="-"></div>
          <div id="-a"></div>
          <div id="-0"></div>
          <div id="7"></div>
          <div id="&#x438;&#x434;">&#x438;&#x434;</div>
          <input type="text" id="input-id">
          <input type="text">
          <p></p>
      </div>

      <div id="classes">
          <!-- Comment node 1 -->
          <div class="foo bar"></div>
          <!-- Comment node 2 -->
          <div class=" foo foo "></div>
          <div class=".foo"></div>
          <div class=".foo.bar"></div>
          <div class="-"></div>
          <div class="-a"></div>
          <div class="-0"></div>
          <div class="--a"></div>
          <div class="---a"></div>
          <div class="7"></div>
          <div class="&#x43A;&#x43B;&#x430;&#x441;&#x441;">&#x43A;&#x43B;&#x430;&#x441;&#x441;</div>
          <div class="__proto__"></div>
          <div class="__proto__ foo"></div>
          <span class="bar"></span>
          <div id="id-with-class" class="moo"></div>
          <input type="text" class="input-class-one">
          <input type="text" class="input-class-two">
          <!-- Comment node 3 -->
      </div>

      <div id="non-unique-classes">
        <!-- Comment node 1 -->
        <span class="c1"></span>
        <!-- Comment node 2 -->
        <span class="c1"></span>
        <!-- Comment node 3 -->
        <span class="c1 c2"></span>
        <!-- Comment node 4 -->
        <span class="c1 c2 c3"></span>
        <!-- Comment node 5 -->
        <span></span>
        <!-- Comment node 6 -->
        <div class="c1"></div>
        <!-- Comment node 7 -->
        <div class="c1 c2"></div>
        <!-- Comment node 8 -->
        <div class="c3 c2"></div>
        <!-- Comment node 9 -->
        <div class="c3 c4"></div>
        <!-- Comment node 10 -->
        <div class="c1 c4"></div>
        <!-- Comment node 11 -->
        <input type="text" class="input-class">
        <input type="text" class="input-class">
        <div></div>
      </div>
    `);
  await TestRunner.evaluateInPagePromise(`
      function matchingElements(selector)
      {
          return document.querySelectorAll(selector).length;
      }
  `);

  var nodeQueue = [];
  ElementsTestRunner.expandElementsTree(enqueueNodes);

  function enqueueNodes() {
    enqueueNode('', getDocumentElement());
    dumpNodeData();
  }

  function dumpNodeData() {
    var entry = nodeQueue.shift();
    if (!entry) {
      TestRunner.completeTest();
      return;
    }
    var cssPath = Elements.DOMPath.cssPath(entry.node, true);
    var result = entry.prefix + cssPath;
    TestRunner.addResult(result.replace(/\n/g, '\\n'));
    var escapedPath = cssPath.replace(/\\/g, '\\\\');
    TestRunner.evaluateInPage('matchingElements(\'' + escapedPath + '\')', callback);

    function callback(result) {
      TestRunner.assertEquals(1, result);
      dumpNodeData();
    }
  }

  function getDocumentElement() {
    var map = TestRunner.domModel._idToDOMNode;
    for (var id in map) {
      if (map[id].nodeName() === '#document')
        return map[id];
    }

    return null;
  }

  function enqueueNode(prefix, node) {
    if (node.nodeType() === Node.ELEMENT_NODE)
      nodeQueue.push({prefix: prefix, node: node});
    var children = node.children();
    for (var i = 0; children && i < children.length; ++i)
      enqueueNode(prefix + '  ', children[i]);
  }
})();
