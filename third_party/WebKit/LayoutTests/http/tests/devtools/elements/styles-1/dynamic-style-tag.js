// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

(async function() {
  TestRunner.addResult(
      `Tests that different types of inline styles are correctly disambiguated and their sourceURL is correct.\n`);
  await TestRunner.loadModule('elements_test_runner');
  await TestRunner.showPanel('elements');
  await TestRunner.navigatePromise('resources/dynamic-style-tag.html');

  ElementsTestRunner.selectNodeAndWaitForStyles('inspected', step1);

  async function step1() {
    var styleSheets = TestRunner.cssModel.allStyleSheets();
    styleSheets.sort();
    for (var header of styleSheets) {
      var content = await TestRunner.CSSAgent.getStyleSheetText(header.id);

      TestRunner.addResult('Stylesheet added:');
      TestRunner.addResult('  - isInline: ' + header.isInline);
      TestRunner.addResult('  - sourceURL: ' + header.sourceURL.substring(header.sourceURL.lastIndexOf('/') + 1));
      TestRunner.addResult('  - hasSourceURL: ' + header.hasSourceURL);
      TestRunner.addResult('  - contents: ' + content);
    }
    ElementsTestRunner.dumpSelectedElementStyles(true, false, true);
    TestRunner.completeTest();
  }
})();
