// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

(async function() {
  TestRunner.addResult(`Tests the tooltip for the image on hover.\n`);
  await TestRunner.loadModule('elements_test_runner');
  await TestRunner.showPanel('elements');
  await TestRunner.loadHTML(`
      <img id="image" src="data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAANcAAACuCAIAAAAqMg/rAAAAAXNSR0IArs4c6QAAAU9JREFUeNrt0jERAAAIxDDAv+dHAxNLIqHXTlLwaiTAheBCXAguxIXgQlwILsSF4EJcCC7EheBCXAguxIXgQlwILsSF4EJcCC7EheBCXAguxIXgQlwILsSF4EJcCC7EheBCXAguxIXgQlwILsSF4EJcCC7EheBCXAguxIXgQlwILsSF4EJcCC7EheBCXAguxIXgQlwILsSF4EJcCC7EheBCXAguxIXgQlwILsSF4EJcCC7EheBCXAguxIXgQlwILsSF4EJcCC7EheBCXAguxIXgQlwILsSF4EJcCC7EheBCXAguxIXgQlwILsSF4EJcCC7EheBCXAguxIXgQlwILsSFEuBCcCEuBBfiQnAhLgQX4kJwIS4EF+JCcCEuBBfiQnAhLgQX4kJwIS4EF+JCcCEuBBfiQnAhLgQX4kJwIS4EF+JCcCEuBBfiQnAhLoSDBZXqBFnkRyeqAAAAAElFTkSuQmCC">
    `);


  var treeElement;
  ElementsTestRunner.nodeWithId('image', step1);

  function step1(node) {
    ElementsTestRunner.firstElementsTreeOutline()._loadDimensionsForNode(node).then(step2);
  }

  function step2(dimensions) {
    const EXPECTED_WIDTH = 215;
    const EXPECTED_HEIGHT = 174;

    if (!dimensions)
      TestRunner.addResult('FAILED, no dimensions on treeElement.');
    else {
      if (dimensions.offsetWidth === dimensions.naturalWidth && dimensions.offsetHeight == dimensions.naturalHeight &&
          dimensions.offsetWidth === EXPECTED_WIDTH && dimensions.offsetHeight === EXPECTED_HEIGHT)
        TestRunner.addResult('PASSED, image dimensions for tooltip: ' + EXPECTED_WIDTH + 'x' + EXPECTED_HEIGHT + '.');
      else
        TestRunner.addResult(
            'FAILED, image dimensions for tooltip: ' + formatDimensions(dimensions) + ' (should be ' + EXPECTED_WIDTH +
            'x' + EXPECTED_HEIGHT + ').');
    }
    TestRunner.completeTest();
  }

  function formatDimensions(dimensions) {
    return dimensions.offsetWidth + 'x' + dimensions.offsetHeight + ' (natural: ' + dimensions.naturalWidth + 'x' +
        dimensions.naturalHeight + ')';
  }
})();
