// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

goog.module('mr.Throttle.test');
goog.setTestOnly();

const Throttle = goog.require('mr.Throttle');
const UnitTestUtils = goog.require('mr.UnitTestUtils');


describe('mr.Timer', () => {
  let clock;

  beforeEach(() => {
    clock = UnitTestUtils.useMockClockAndPromises();
  });

  afterEach(() => {
    UnitTestUtils.restoreRealClockAndPromises();
  });

  it('works', () => {
    let callBackCount = 0;
    const callBackFunction = () => {
      callBackCount++;
    };

    const throttle = new Throttle(callBackFunction, 100);
    expect(callBackCount).toBe(0);
    throttle.fire();
    expect(callBackCount).toBe(1);
    throttle.fire();
    expect(callBackCount).toBe(1);
    throttle.fire();
    throttle.fire();
    expect(callBackCount).toBe(1);
    clock.tick(101);
    expect(callBackCount).toBe(2);
    clock.tick(101);
    expect(callBackCount).toBe(2);
  });

  it('binds scope correctly', () => {
    const interval = 500;
    const x = {'y': 0};
    new Throttle(function() {
      ++this['y'];
    }, interval, x).fire();
    expect(x['y']).toBe(1);
  });


  it('binds arguments correctly', () => {
    const interval = 500;
    let calls = 0;
    const throttle = new Throttle((a, b, c) => {
      ++calls;
      expect(a).toBe(3);
      expect(b).toBe('string');
      expect(c).toBe(false);
    }, interval);

    throttle.fire(3, 'string', false);
    expect(calls).toBe(1);

    // fire should always pass the last arguments passed to it into the
    // decorated function, even if called multiple times.
    throttle.fire();
    clock.tick(interval / 2);
    throttle.fire(8, null, true);
    throttle.fire(3, 'string', false);
    clock.tick(interval);
    expect(calls).toBe(2);
  });


  it('binds arguments and scope correctly', () => {
    const interval = 500;
    const x = {'calls': 0};
    const throttle = new Throttle(function(a, b, c) {
      ++this['calls'];
      expect(a).toBe(3);
      expect(b).toBe('string');
      expect(c).toBe(false);
    }, interval, x);

    throttle.fire(3, 'string', false);
    expect(x['calls']).toBe(1);

    // fire should always pass the last arguments passed to it into the
    // decorated function, even if called multiple times.
    throttle.fire();
    clock.tick(interval / 2);
    throttle.fire(8, null, true);
    throttle.fire(3, 'string', false);
    clock.tick(interval);
    expect(x['calls']).toBe(2);
  });
});
