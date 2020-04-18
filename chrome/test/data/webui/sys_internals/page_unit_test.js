// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

var PageTest = PageTest || {};

PageTest.Unit = function() {
  suite('Page unit test', function() {
    test('getDiffPerSecAndUpdateCounter', function() {
      const getDiffAndUpdateCounter = SysInternals.getDiffAndUpdateCounter;
      assertEquals(getDiffAndUpdateCounter('test', 10, 1000), 0);
      assertEquals(getDiffAndUpdateCounter('test', 20, 2000), 10);
      assertEquals(getDiffAndUpdateCounter('test', 42, 3000), 22);

      const getDiffPerSecAndUpdateCounter =
          SysInternals.getDiffPerSecAndUpdateCounter;
      TestUtil.assertCloseTo(
          getDiffPerSecAndUpdateCounter('test2', 10, 1000), 0, 1e-2);
      TestUtil.assertCloseTo(
          getDiffPerSecAndUpdateCounter('test2', 20, 3000), 5, 1e-2);
      TestUtil.assertCloseTo(
          getDiffPerSecAndUpdateCounter('test2', 42, 3720), 30.555, 1e-2);
      TestUtil.assertCloseTo(
          getDiffPerSecAndUpdateCounter('test2', 59, 4999), 13.291, 1e-2);
    });

    test('toPercentageString', function() {
      const toPercentageString = SysInternals.toPercentageString;
      assertEquals(toPercentageString(0.12345, 2), '12.35%');
      assertEquals(toPercentageString(1.23456, 2), '123.46%');
      assertEquals(toPercentageString(0.424242, 2), '42.42%');
      assertEquals(toPercentageString(NaN, 2), 'NaN%');
    });

    test('getValueWithUnit', function() {
      const UNITS = ['B', 'KB', 'MB', 'GB'];
      const UNITBASE = 1024;
      const GB = TestUtil.MEMORY_UNITS.GB;
      const KB = TestUtil.MEMORY_UNITS.KB;
      const getValueWithUnit = SysInternals.getValueWithUnit;
      assertEquals(getValueWithUnit(60 * KB, UNITS, UNITBASE), '60.00 KB');
      assertEquals(getValueWithUnit(42 * GB, UNITS, UNITBASE), '42.00 GB');
      assertEquals(getValueWithUnit(73.2546 * KB, UNITS, UNITBASE), '73.25 KB');
    });
  });

  mocha.run();
};
