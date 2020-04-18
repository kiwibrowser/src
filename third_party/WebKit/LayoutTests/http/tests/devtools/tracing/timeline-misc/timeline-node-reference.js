// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

(async function() {
  TestRunner.addResult(`Tests the Timeline API instrumentation of a Layout event\n`);
  await TestRunner.loadModule('elements_test_runner');
  await TestRunner.loadModule('performance_test_runner');
  await TestRunner.showPanel('timeline');
  await TestRunner.loadHTML(`
      <style>
      .relayout-boundary {
          overflow: hidden;
          width: 100px;
          height: 100px;
      }
      </style>
      <div id="boundary" class="relayout-boundary">
          <div>
              <div id="invalidate1"><div>text</div></div>
          </div>
      </div>
    `);
  await TestRunner.evaluateInPagePromise(`
      function performActions()
      {
          var element = document.getElementById("invalidate1");
          element.style.marginTop = "10px";
          var unused = element.offsetHeight;
      }
  `);

  TestRunner.evaluateInPage('var unused = document.body.offsetWidth;', async function() {
    const records = await PerformanceTestRunner.evaluateWithTimeline('performActions()');
    const layoutEvent = PerformanceTestRunner.findTimelineEvent(TimelineModel.TimelineModel.RecordType.Layout);
    UI.context.addFlavorChangeListener(SDK.DOMNode, onSelectedNodeChanged);
    clickValueLink(layoutEvent, 'Layout root');
  });

  async function clickValueLink(event, row) {
    var model = UI.panels.timeline._performanceModel.timelineModel();
    var element = await Timeline.TimelineUIUtils.buildTraceEventDetails(event, model, new Components.Linkifier(), true);
    var rows = element.querySelectorAll('.timeline-details-view-row');
    for (var i = 0; i < rows.length; ++i) {
      if (rows[i].firstChild.textContent.indexOf(row) !== -1) {
        rows[i].lastChild.firstChild.shadowRoot.lastChild.click();
        return;
      }
    }
  }

  function onSelectedNodeChanged() {
    var node = UI.panels.elements.selectedDOMNode();
    // We may first get an old selected node while switching to the Elements panel.
    if (node.nodeName() === 'BODY')
      return;
    UI.context.removeFlavorChangeListener(SDK.DOMNode, onSelectedNodeChanged);
    TestRunner.addResult('Layout root node id: ' + node.getAttribute('id'));
    TestRunner.completeTest();
  }
})();
