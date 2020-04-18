// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

var LineChartTest = LineChartTest || {};

LineChartTest.Scrollbar = function() {
  test('Scrollbar integration test', function() {
    const scrollbar = new LineChart.Scrollbar(function() {});
    scrollbar.resize(100);
    scrollbar.setRange(1000);

    /* See |LineChart.Scrollbar.isScrolledToRightEdge()|. */
    const scrollError = 2;
    assertFalse(scrollbar.isScrolledToRightEdge());
    TestUtil.assertCloseTo(scrollbar.getPosition(), 0, scrollError);
    scrollbar.scrollToRightEdge();
    assertTrue(scrollbar.isScrolledToRightEdge());
    TestUtil.assertCloseTo(scrollbar.getPosition(), 1000, scrollError);
    scrollbar.setPosition(500);
    assertFalse(scrollbar.isScrolledToRightEdge());
    TestUtil.assertCloseTo(scrollbar.getPosition(), 500, scrollError);
    scrollbar.setRange(100);
    assertTrue(scrollbar.isScrolledToRightEdge());
    TestUtil.assertCloseTo(scrollbar.getPosition(), 100, scrollError);
  });

  mocha.run();
};
