// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

(async function() {
  TestRunner.addResult(`Tests that forced element state is reflected in the DOM tree and Styles pane.\n`);
  await TestRunner.loadModule('elements_test_runner');
  await TestRunner.showPanel('elements');
  await TestRunner.loadHTML(`
      <head>
        <style>
        div:hover, a:hover {
            color: red;
        }

        div:focus, a:focus {
            border: 1px solid green;
        }

        div:active, a:active {
            font-weight: bold;
        }

        </style>
      </head>
      <body id="mainBody" class="main1 main2 mainpage" style="font-weight: normal; width: 85%; background-image: url(bar.png)">
        <div id="div">Test text</div>
      </body>
    `);

  ElementsTestRunner.nodeWithId('div', foundDiv);

  var divNode;

  function dumpData() {
    ElementsTestRunner.dumpSelectedElementStyles(true);
    ElementsTestRunner.dumpElementsTree();
  }

  function foundDiv(node) {
    divNode = node;
    TestRunner.cssModel.forcePseudoState(divNode, 'hover', true);
    TestRunner.cssModel.forcePseudoState(divNode, 'active', true);
    ElementsTestRunner.selectNodeAndWaitForStyles('div', divSelected1);
  }

  function divSelected1() {
    TestRunner.addResult('');
    TestRunner.addResult('DIV with :hover and :active');
    dumpData();
    ElementsTestRunner.waitForStyles('div', hoverCallback, true);
    TestRunner.cssModel.forcePseudoState(divNode, 'hover', false);

    function hoverCallback() {
      ElementsTestRunner.waitForStyles('div', divSelected2, true);
      TestRunner.cssModel.forcePseudoState(divNode, 'focus', true);
    }
  }

  function divSelected2() {
    TestRunner.addResult('');
    TestRunner.addResult('DIV with :active and :focus');
    dumpData();
    ElementsTestRunner.waitForStyles('div', focusCallback, true);
    TestRunner.cssModel.forcePseudoState(divNode, 'focus', false);

    function focusCallback() {
      ElementsTestRunner.waitForStyles('div', divSelected3, true);
      TestRunner.cssModel.forcePseudoState(divNode, 'active', false);
    }
  }

  function divSelected3(node) {
    TestRunner.addResult('');
    TestRunner.addResult('DIV with no forced state');
    dumpData();
    TestRunner.completeTest();
  }
})();
