// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

(async function() {
  TestRunner.addResult(`Tests grouped invalidations on the timeline.\n`);
  await TestRunner.loadModule('performance_test_runner');
  await TestRunner.showPanel('timeline');
  await TestRunner.loadHTML(`
    <!DOCTYPE HTML>
    <div class="testElement">P</div><div class="testElement">A</div>
    <div class="testElement">S</div><div class="testElement">S</div>
  `);
  await TestRunner.evaluateInPagePromise(`
    function display()
    {
      return new Promise(resolve => {
        requestAnimationFrame(function() {
          var testElements = document.body.getElementsByClassName("testElement");
          for (var i = 0; i < testElements.length; i++) {
            testElements[i].style.color = "red";
            testElements[i].style.backgroundColor = "blue";
          }
          testRunner.layoutAndPaintAsyncThen(resolve);
        });
      });
    }
  `);

  Runtime.experiments.enableForTest('timelineInvalidationTracking');

  await PerformanceTestRunner.invokeAsyncWithTimeline('display');
  var event = PerformanceTestRunner.findTimelineEvent(TimelineModel.TimelineModel.RecordType.Paint);
  TestRunner.addArray(
      TimelineModel.InvalidationTracker.invalidationEventsFor(event), PerformanceTestRunner.InvalidationFormatters,
      '', 'paint invalidations');

  var linkifier = new Components.Linkifier();
  var target = PerformanceTestRunner.timelineModel().targetByEvent(event);
  var contentHelper = new Timeline.TimelineDetailsContentHelper(target, linkifier, true);
  Timeline.TimelineUIUtils._generateCauses(event, target, null, contentHelper);
  var invalidationsTree = contentHelper.element.getElementsByClassName('invalidations-tree')[0];
  var invalidations = invalidationsTree.shadowRoot.textContent;
  checkStringContains(
      invalidations,
      `Inline CSS style declaration was mutated for [ DIV class='testElement' ], [ DIV class='testElement' ], and 2 others. (anonymous) @ timeline-grouped-invalidations.js:21`);
  checkStringContains(
      invalidations,
      `Inline CSS style declaration was mutated for [ DIV class='testElement' ], [ DIV class='testElement' ], and 2 others. (anonymous) @ timeline-grouped-invalidations.js:22`);
  TestRunner.completeTest();

  function checkStringContains(string, contains) {
    var doesContain = string.indexOf(contains) >= 0;
    TestRunner.check(doesContain, contains + ' should be present in ' + string);
    TestRunner.addResult('PASS - record contained ' + contains);
  }
})();
