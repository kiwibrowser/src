// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

(async function() {
  TestRunner.addResult(`Tests that emulated CSS media is reflected in the Styles pane.\n`);
  await TestRunner.loadModule('elements_test_runner');
  await TestRunner.showPanel('elements');
  await TestRunner.loadHTML(`
      <style>
      #main { color: red; }

      @media print {
      #main { color: black; }
      }

      @media tty {
      #main { color: green; }
      }
      </style>
      <div id="main"></div>
    `);

  ElementsTestRunner.selectNodeAndWaitForStyles('main', step0);

  function step0() {
    TestRunner.addResult('Original style:');
    ElementsTestRunner.dumpSelectedElementStyles(true);
    applyEmulatedMedia('print');
    ElementsTestRunner.waitForStyles('main', step1);
  }

  function step1() {
    TestRunner.addResult('print media emulated:');
    ElementsTestRunner.dumpSelectedElementStyles(true);
    applyEmulatedMedia('tty');
    ElementsTestRunner.waitForStyles('main', step2);
  }

  function step2() {
    TestRunner.addResult('tty media emulated:');
    ElementsTestRunner.dumpSelectedElementStyles(true);
    applyEmulatedMedia('');
    ElementsTestRunner.waitForStyles('main', step3);
  }

  function step3() {
    TestRunner.addResult('No media emulated:');
    ElementsTestRunner.dumpSelectedElementStyles(true);
    TestRunner.completeTest();
  }

  function applyEmulatedMedia(media) {
    TestRunner.EmulationAgent.setEmulatedMedia(media);
    TestRunner.cssModel.mediaQueryResultChanged();
  }
})();
