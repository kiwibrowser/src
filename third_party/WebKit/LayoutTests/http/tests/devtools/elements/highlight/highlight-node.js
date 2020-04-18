// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

(async function() {
  TestRunner.addResult(
      `This test verifies the position and size of the highlight rectangles overlayed on an inspected div.\n`);
  await TestRunner.loadModule('elements_test_runner');
  await TestRunner.showPanel('elements');
  await TestRunner.loadHTML(`
      <style>

      body {
          margin: 0;
      }
      #container {
          width: 400px;
          height: 400px;
          background-color: grey;
      }
      #inspectedElement {
          margin: 5px;
          border: solid 10px aqua;
          padding: 15px;
          width: 200px;
          height: 200px;
          background-color: blue;
          float: left;
      }
      #description {
          clear: both;
      }

      </style>
      <div id="inspectedElement"></div>
      <p id="description"></p>
    `);

  ElementsTestRunner.dumpInspectorHighlightJSON('inspectedElement', TestRunner.completeTest.bind(TestRunner));
})();
