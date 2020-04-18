// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

(async function() {
  TestRunner.addResult(`Tests sticky position constraints in Layers panel\n`);
  await TestRunner.loadModule('layers_test_runner');
  await TestRunner.loadHTML(`
      <style>
      .scroller {
        height: 100px;
        width: 100px;
        overflow-y: scroll;
        overflow-x: hidden;
      }

      .composited {
        will-change: transform;
      }

      .inline {
        display: inline;
      }

      .sticky {
        position: sticky;
        top: 10px;
      }

      .large-box {
        height: 50px;
        width: 50px;
        background-color: green;
      }

      .small-box {
        height: 25px;
        width: 25px;
        background-color: blue;
      }

      .padding {
        height: 350px;
        width: 100px;
      }
      </style>
      <!-- General stickyPositionConstraint test. -->
      <div class="composited scroller">
        <div class="composited sticky large-box">
          <div class="composited sticky small-box" style="top: 20px;"></div>
        </div>
        <div class="padding"></div>
      </div>

      <!-- Test _nearestLayerShiftingStickyBox is filled correctly. -->
      <div class="composited scroller">
        <div class="composited inline sticky">
          <div class="composited inline sticky" style="top: 20px;">
          </div>
        </div>
        <div class="padding"></div>
      </div>
    `);

  await LayersTestRunner.requestLayers();
  TestRunner.addResult('Sticky position constraint');

  var stickyFormatters = {
    '_nearestLayerShiftingContainingBlock': 'formatAsTypeNameOrNull',
    '_nearestLayerShiftingStickyBox': 'formatAsTypeNameOrNull'
  };

  LayersTestRunner.layerTreeModel().layerTree().forEachLayer(layer => {
    if (layer._stickyPositionConstraint)
      TestRunner.addObject(layer._stickyPositionConstraint, stickyFormatters);
  });

  TestRunner.completeTest();
})();
