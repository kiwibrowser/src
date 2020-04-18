// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

(async function() {
  TestRunner.addResult(`Tests layer compositing reasons in Layers Panel`);
  await TestRunner.loadModule('layers_test_runner');
  await TestRunner.navigatePromise(TestRunner.url('resources/compositing-reasons.html'));

  await TestRunner.evaluateInPageAsync(`
    (function() {
      return new Promise(fulfill => {
        var iframe = document.getElementById('iframe');
        iframe.onload = fulfill;
        iframe.src = "composited-iframe.html";
      });
    })()`);

  async function dumpCompositingReasons(layer) {
    var reasons = await layer.requestCompositingReasons();
    var node = layer.nodeForSelfOrAncestor();
    var label = Elements.DOMPath.fullQualifiedSelector(node, false);
    TestRunner.addResult(`Compositing reasons for ${label}: ` + reasons.sort().join(','));
  }

  var idsToTest = [
    'transform3d', 'transform3d-individual', 'iframe', 'backface-visibility', 'animation', 'animation-individual',
    'transformWithCompositedDescendants', 'transformWithCompositedDescendants-individual',
    'opacityWithCompositedDescendants', 'reflectionWithCompositedDescendants', 'perspective', 'preserve3d'
  ];

  await LayersTestRunner.requestLayers();
  dumpCompositingReasons(LayersTestRunner.layerTreeModel().layerTree().contentRoot());
  for (var i = 0; i < idsToTest.length - 1; ++i)
    dumpCompositingReasons(LayersTestRunner.findLayerByNodeIdAttribute(idsToTest[i]));

  await dumpCompositingReasons(LayersTestRunner.findLayerByNodeIdAttribute(idsToTest[idsToTest.length - 1]));
  TestRunner.completeTest();
})();
