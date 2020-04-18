// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

(async function() {
  TestRunner.addResult(`Checks the Product property in details pane for a node with URL.\n`);
  await TestRunner.loadModule('performance_test_runner');
  await TestRunner.loadModule('network_test_runner');
  await TestRunner.showPanel('timeline');

  await ProductRegistry.instance();
  NetworkTestRunner.resetProductRegistry();
  NetworkTestRunner.addProductRegistryEntry('*.google.com', 'Google');
  TestRunner.addResult('');

  var sessionId = '6.23';
  var rawTraceEvents = [
    {
      'args': {'name': 'Renderer'},
      'cat': '__metadata',
      'name': 'process_name',
      'ph': 'M',
      'pid': 17851,
      'tid': 23,
      'ts': 0
    },
    {
      'args': {'name': 'CrRendererMain'},
      'cat': '__metadata',
      'name': 'thread_name',
      'ph': 'M',
      'pid': 17851,
      'tid': 23,
      'ts': 0
    },
    {
      'args': {'data': {'sessionId': sessionId, 'frames': [
        {'frame': 'frame1', 'url': 'frameurl', 'name': 'frame-name'}
      ]}},
      'cat': 'disabled-by-default-devtools.timeline',
      'name': 'TracingStartedInPage',
      'ph': 'I',
      'pid': 17851,
      'tid': 23,
      'ts': 100000,
      'tts': 606543
    },
    {
      'cat': 'disabled-by-default-devtools.timeline',
      'name': 'EvaluateScript',
      'ph': 'X',
      'pid': 17851,
      'tid': 23,
      'ts': 101000,
      'dur': 10000,
      'args': {'data': {'url': 'https://www.google.com', 'lineNumber': 1337}}
    }
  ];

  var badgeRendered = Promise.resolve();
  TestRunner.addSniffer(
      ProductRegistry.BadgePool.prototype, '_renderBadge', (arg, result) => badgeRendered = result, true);
  Common.settings.moduleSetting('product_registry.badges-visible').set(true);
  var model = PerformanceTestRunner.createPerformanceModelWithEvents(rawTraceEvents).timelineModel();
  var linkifier = new Components.Linkifier();
  var badgePool = new ProductRegistry.BadgePool();
  for (var event of PerformanceTestRunner.mainTrackEvents()) {
    var node = await Timeline.TimelineUIUtils.buildTraceEventDetails(event, model, linkifier, badgePool);
    await badgeRendered;
    for (var child of node.querySelectorAll('.timeline-details-view-row'))
      TestRunner.addResult(
          TestRunner.deepTextContent(child.firstChild) + ': ' + TestRunner.deepTextContent(child.lastChild));
  }
  TestRunner.completeTest();
})();
