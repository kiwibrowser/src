// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

goog.setTestOnly();
goog.require('mr.Analytics');
goog.require('mr.LongTiming');
goog.require('mr.MediumTiming');
goog.require('mr.MockClock');
goog.require('mr.Timing');

describe('Tests Analytics', function() {
  let mockClock;

  const TEN_SECONDS = 10 * 1000;
  const THREE_MINUTES = 3 * 60 * 1000;
  const ONE_HOUR = 60 * 60 * 1000;

  beforeEach(function() {
    mockClock = new mr.MockClock(true);
    chrome.metricsPrivate = {
      recordTime: jasmine.createSpy('recordTime'),
      recordMediumTime: jasmine.createSpy('recordMediumTime'),
      recordLongTime: jasmine.createSpy('recordLongTime'),
      recordUserAction: jasmine.createSpy('recordUserAction'),
      recordValue: jasmine.createSpy('recordValue'),
      recordSmallCount: jasmine.createSpy('recordSmallCount'),
    };
  });

  afterEach(function() {
    mockClock.uninstall();
  });

  describe('Test Timing Events', function() {
    describe('Test mr.Timing', function() {
      it('Should record the time passing', function() {
        const histogramName = 'Test';
        const timeToPass = 34;
        const timing = new mr.Timing(histogramName);
        mockClock.tick(timeToPass);
        timing.end();
        expect(chrome.metricsPrivate.recordTime.calls.count()).toBe(1);
        expect(chrome.metricsPrivate.recordTime)
            .toHaveBeenCalledWith(histogramName, timeToPass);
      });
      it('Should record the time passing with a suffix', function() {
        const histogramName = 'Test';
        const suffixName = 'Test';
        const expectedFinalName = histogramName + '_' + suffixName;
        const timeToPass = 34;
        const timing = new mr.Timing(histogramName);
        mockClock.tick(timeToPass);
        timing.end(suffixName);
        expect(chrome.metricsPrivate.recordTime.calls.count()).toBe(1);
        expect(chrome.metricsPrivate.recordTime)
            .toHaveBeenCalledWith(expectedFinalName, timeToPass);
      });
      it('Should record the max if duration exceeds ten seconds', function() {
        const histogramName = 'Test';
        const timeToPass = TEN_SECONDS + 1;
        const timing = new mr.Timing(histogramName);
        mockClock.tick(timeToPass);
        timing.end();
        expect(chrome.metricsPrivate.recordTime.calls.count()).toBe(1);
        expect(chrome.metricsPrivate.recordTime)
            .toHaveBeenCalledWith(histogramName, TEN_SECONDS);
      });
      it('Should record the minimum if duration is negative', function() {
        const histogramName = 'Test';
        const timeToPass = -1;
        const timing = new mr.Timing(histogramName);
        mockClock.tick(timeToPass);
        timing.end();
        expect(chrome.metricsPrivate.recordTime.calls.count()).toBe(1);
        expect(chrome.metricsPrivate.recordTime)
            .toHaveBeenCalledWith(histogramName, 0);
      });
    });
    describe('Test mr.MediumTiming', function() {
      it('Should record the time passing', function() {
        const histogramName = 'Test';
        const timeToPass = 34 * 1000;
        const timing = new mr.MediumTiming(histogramName);
        mockClock.tick(timeToPass);
        timing.end();
        expect(chrome.metricsPrivate.recordMediumTime.calls.count()).toBe(1);
        expect(chrome.metricsPrivate.recordMediumTime)
            .toHaveBeenCalledWith(histogramName, timeToPass);
      });
      it('Should record the time passing with a suffix', function() {
        const histogramName = 'Test';
        const suffixName = 'Test';
        const expectedFinalName = histogramName + '_' + suffixName;
        const timeToPass = 34 * 1000;
        const timing = new mr.MediumTiming(histogramName);
        mockClock.tick(timeToPass);
        timing.end(suffixName);
        expect(chrome.metricsPrivate.recordMediumTime.calls.count()).toBe(1);
        expect(chrome.metricsPrivate.recordMediumTime)
            .toHaveBeenCalledWith(expectedFinalName, timeToPass);
      });
      it('Should record ten seconds if duration is below ten seconds',
         function() {
           const histogramName = 'Test';
           const timeToPass = 34;
           const timing = new mr.MediumTiming(histogramName);
           mockClock.tick(timeToPass);
           timing.end();
           expect(chrome.metricsPrivate.recordMediumTime.calls.count()).toBe(1);
           expect(chrome.metricsPrivate.recordMediumTime)
               .toHaveBeenCalledWith(histogramName, TEN_SECONDS);
         });
      it('Should record three minutes if duration exceeds three minutes',
         function() {
           const histogramName = 'Test';
           const timeToPass = THREE_MINUTES + 1;
           const timing = new mr.MediumTiming(histogramName);
           mockClock.tick(timeToPass);
           timing.end();
           expect(chrome.metricsPrivate.recordMediumTime.calls.count()).toBe(1);
           expect(chrome.metricsPrivate.recordMediumTime)
               .toHaveBeenCalledWith(histogramName, THREE_MINUTES);
         });
    });
    describe('Test mr.LongTiming', function() {
      it('Should record the time passing', function() {
        const histogramName = 'Test';
        const timeToPass = 34 * 60 * 1000;
        const timing = new mr.LongTiming(histogramName);
        mockClock.tick(timeToPass);
        timing.end();
        expect(chrome.metricsPrivate.recordLongTime.calls.count()).toBe(1);
        expect(chrome.metricsPrivate.recordLongTime)
            .toHaveBeenCalledWith(histogramName, timeToPass);
      });
      it('Should record the time passing with a suffix', function() {
        const histogramName = 'Test';
        const suffixName = 'Test';
        const expectedFinalName = histogramName + '_' + suffixName;
        const timeToPass = 34 * 60 * 1000;
        const timing = new mr.LongTiming(histogramName);
        mockClock.tick(timeToPass);
        timing.end(suffixName);
        expect(chrome.metricsPrivate.recordLongTime.calls.count()).toBe(1);
        expect(chrome.metricsPrivate.recordLongTime)
            .toHaveBeenCalledWith(expectedFinalName, timeToPass);
      });
      it('Should record three minutes if duration is below three minutes',
         function() {
           const histogramName = 'Test';
           const timeToPass = 2 * 60 * 1000;
           const timing = new mr.LongTiming(histogramName);
           mockClock.tick(timeToPass);
           timing.end();
           expect(chrome.metricsPrivate.recordLongTime.calls.count()).toBe(1);
           expect(chrome.metricsPrivate.recordLongTime)
               .toHaveBeenCalledWith(histogramName, THREE_MINUTES);
         });
      it('Should record one hour if duration exceeds one hour', function() {
        const histogramName = 'Test';
        const timeToPass = ONE_HOUR + 1;
        const timing = new mr.LongTiming(histogramName);
        mockClock.tick(timeToPass);
        timing.end();
        expect(chrome.metricsPrivate.recordLongTime.calls.count()).toBe(1);
        expect(chrome.metricsPrivate.recordLongTime)
            .toHaveBeenCalledWith(histogramName, ONE_HOUR);
      });
      it('Should not record the time if it we went back in time', function() {
        const histogramName = 'Test';
        const timeToPass = -1;
        const timing = new mr.LongTiming(histogramName);
        mockClock.tick(timeToPass);
        timing.end();
        expect(chrome.metricsPrivate.recordLongTime.calls.count()).toBe(0);
      });
    });
  });
  describe('Test recordEvent', function() {
    it('Should record an event', function() {
      const eventName = 'Test';
      mr.Analytics.recordEvent(eventName);
      expect(chrome.metricsPrivate.recordUserAction.calls.count()).toBe(1);
      expect(chrome.metricsPrivate.recordUserAction)
          .toHaveBeenCalledWith(eventName);
    });
  });
  describe('Test recordEnum', function() {
    const testHistogram = 'Test';
    const testValues = {TEST1: 0, TEST2: 1, TEST3: 2};
    const testConfig = {
      'metricName': testHistogram,
      'type': 'histogram-linear',
      'min': 1,
      'max': 3,
      'buckets': 4
    };
    it('Should record an event with corrct index of 0', function() {
      mr.Analytics.recordEnum(testHistogram, testValues.TEST1, testValues);
      expect(chrome.metricsPrivate.recordValue.calls.count()).toBe(1);
      expect(chrome.metricsPrivate.recordValue)
          .toHaveBeenCalledWith(testConfig, 0);
    });
    it('Should record an event with corrct index of 1', function() {
      mr.Analytics.recordEnum(testHistogram, testValues.TEST2, testValues);
      expect(chrome.metricsPrivate.recordValue.calls.count()).toBe(1);
      expect(chrome.metricsPrivate.recordValue)
          .toHaveBeenCalledWith(testConfig, 1);
    });
    it('Should record an event with correct index of 2', function() {
      mr.Analytics.recordEnum(testHistogram, testValues.TEST3, testValues);
      expect(chrome.metricsPrivate.recordValue.calls.count()).toBe(1);
      expect(chrome.metricsPrivate.recordValue)
          .toHaveBeenCalledWith(testConfig, 2);
    });
    it('Should not record an event with an unknown value', function() {
      mr.Analytics.recordEnum(testHistogram, 3, testValues);
      expect(chrome.metricsPrivate.recordValue.calls.count()).toBe(0);
    });
  });
  describe('Test recordSmallCount', () => {
    it('Record 0 count succeeds', () => {
      mr.Analytics.recordSmallCount('smallCount', 0);
      expect(chrome.metricsPrivate.recordSmallCount.calls.count()).toBe(1);
      expect(chrome.metricsPrivate.recordSmallCount)
          .toHaveBeenCalledWith('smallCount', 0);
    });
    it('Record negative count fails', () => {
      mr.Analytics.recordSmallCount('smallCount', -1);
      expect(chrome.metricsPrivate.recordSmallCount.calls.count()).toBe(0);
    });
    it('Record large count succeeds', () => {
      mr.Analytics.recordSmallCount('smallCount', 200);
      expect(chrome.metricsPrivate.recordSmallCount.calls.count()).toBe(1);
      expect(chrome.metricsPrivate.recordSmallCount)
          .toHaveBeenCalledWith('smallCount', 200);
    });
    it('Record regular count succeeds', () => {
      mr.Analytics.recordSmallCount('smallCount', 1);
      mr.Analytics.recordSmallCount('smallCount', 50);
      mr.Analytics.recordSmallCount('smallCount', 100);
      expect(chrome.metricsPrivate.recordSmallCount.calls.count()).toBe(3);
    });
  });
});
