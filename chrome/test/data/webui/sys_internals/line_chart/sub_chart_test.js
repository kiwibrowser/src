// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

var LineChartTest = LineChartTest || {};

LineChartTest.SubChart = function() {
  test('SubChart integration test', function() {
    const data1 = new LineChart.DataSeries('test1', '#aabbcc');
    data1.addDataPoint(100, 1504764694799);
    data1.addDataPoint(100, 1504764695799);
    data1.addDataPoint(100, 1504764696799);
    const data2 = new LineChart.DataSeries('test2', '#aabbcc');
    data2.addDataPoint(40, 1504764694799);
    data2.addDataPoint(42, 1504764695799);
    data2.addDataPoint(40, 1504764696799);
    const data3 = new LineChart.DataSeries('test3', '#aabbcc');
    data3.addDataPoint(1024, 1504764694799);
    data3.addDataPoint(2048, 1504764695799);
    data3.addDataPoint(4096, 1504764696799);

    const label = new LineChart.UnitLabel(['/s', 'K/s', 'M/s'], 1000);
    const subChart =
        new LineChart.SubChart(label, LineChart.UnitLabelAlign.RIGHT);
    assertFalse(subChart.shouldRender());
    subChart.addDataSeries(data1);
    subChart.addDataSeries(data2);
    subChart.addDataSeries(data3);
    assertEquals(subChart.getDataSeriesList().length, 3);
    assertTrue(subChart.shouldRender());

    subChart.setLayout(1920, 1080, 14, 1504764695799, 150, 8);
    TestUtil.assertCloseTo(subChart.label_.maxValueCache_, 2389.333, 1e-2);
    subChart.setMaxValue(424242);
    TestUtil.assertCloseTo(subChart.label_.maxValueCache_, 424242, 1e-2);

    subChart.setLayout(1920, 1080, 14, 1504764695799, 10, 8);
    TestUtil.assertCloseTo(subChart.label_.maxValueCache_, 424242, 1e-2);
    subChart.setMaxValue(null);
    TestUtil.assertCloseTo(subChart.label_.maxValueCache_, 4096, 1e-2);

    subChart.setLayout(150, 100, 14, 1504764685799, 100, 8);
    TestUtil.assertCloseTo(subChart.label_.maxValueCache_, 3072, 1e-2);
  });

  mocha.run();
};
