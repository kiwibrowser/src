// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

goog.setTestOnly();
goog.require('mr.MirrorAnalytics');

describe('Tests Analytics', function() {
  const metricName = 'MediaRouter.Fake.Start.Failure';

  beforeEach(function() {
    chrome.metricsPrivate = {
      recordTime: jasmine.createSpy('recordTime'),
      recordMediumTime: jasmine.createSpy('recordMediumTime'),
      recordLongTime: jasmine.createSpy('recordLongTime'),
      recordUserAction: jasmine.createSpy('recordUserAction'),
      recordValue: jasmine.createSpy('recordValue'),
    };
  });

  describe('Test Mirror Analytics', function() {
    describe('Test recordCapturingFailure', function() {
      const testConfig = {
        'metricName': metricName,
        'type': 'histogram-linear',
        'min': 1,
        'max': 10,
        'buckets': 11
      };
      it('Should record an empty stream error', function() {
        mr.MirrorAnalytics.recordCapturingFailureWithName(
            mr.MirrorAnalytics.CapturingFailure.CAPTURE_TAB_FAIL_EMPTY_STREAM,
            metricName);
        expect(chrome.metricsPrivate.recordValue.calls.count()).toBe(1);
        expect(chrome.metricsPrivate.recordValue)
            .toHaveBeenCalledWith(testConfig, 0);
      });
      it('Should record a desktop timeout error', function() {
        mr.MirrorAnalytics.recordCapturingFailureWithName(
            mr.MirrorAnalytics.CapturingFailure
                .CAPTURE_DESKTOP_FAIL_ERROR_TIMEOUT,
            metricName);
        expect(chrome.metricsPrivate.recordValue.calls.count()).toBe(1);
        expect(chrome.metricsPrivate.recordValue)
            .toHaveBeenCalledWith(testConfig, 1);
      });
      it('Should record a tab timeout error', function() {
        mr.MirrorAnalytics.recordCapturingFailureWithName(
            mr.MirrorAnalytics.CapturingFailure.CAPTURE_TAB_TIMEOUT,
            metricName);
        expect(chrome.metricsPrivate.recordValue.calls.count()).toBe(1);
        expect(chrome.metricsPrivate.recordValue)
            .toHaveBeenCalledWith(testConfig, 2);
      });
      it('Should record an user cancel error', function() {
        mr.MirrorAnalytics.recordCapturingFailureWithName(
            mr.MirrorAnalytics.CapturingFailure
                .CAPTURE_DESKTOP_FAIL_ERROR_USER_CANCEL,
            metricName);
        expect(chrome.metricsPrivate.recordValue.calls.count()).toBe(1);
        expect(chrome.metricsPrivate.recordValue)
            .toHaveBeenCalledWith(testConfig, 3);
      });
      it('Should record an answer not received error', function() {
        mr.MirrorAnalytics.recordCapturingFailureWithName(
            mr.MirrorAnalytics.CapturingFailure.ANSWER_NOT_RECEIVED,
            metricName);
        expect(chrome.metricsPrivate.recordValue.calls.count()).toBe(1);
        expect(chrome.metricsPrivate.recordValue)
            .toHaveBeenCalledWith(testConfig, 4);
      });
      it('Should record a tab fail error', function() {
        mr.MirrorAnalytics.recordCapturingFailureWithName(
            mr.MirrorAnalytics.CapturingFailure.CAPTURE_TAB_FAIL_ERROR_TIMEOUT,
            metricName);
        expect(chrome.metricsPrivate.recordValue.calls.count()).toBe(1);
        expect(chrome.metricsPrivate.recordValue)
            .toHaveBeenCalledWith(testConfig, 5);
      });
      it('Should record an ice connection closed error', function() {
        mr.MirrorAnalytics.recordCapturingFailureWithName(
            mr.MirrorAnalytics.CapturingFailure.ICE_CONNECTION_CLOSED,
            metricName);
        expect(chrome.metricsPrivate.recordValue.calls.count()).toBe(1);
        expect(chrome.metricsPrivate.recordValue)
            .toHaveBeenCalledWith(testConfig, 6);
      });
      it('Should record a tab failure', function() {
        mr.MirrorAnalytics.recordCapturingFailureWithName(
            mr.MirrorAnalytics.CapturingFailure.TAB_FAIL, metricName);
        expect(chrome.metricsPrivate.recordValue.calls.count()).toBe(1);
        expect(chrome.metricsPrivate.recordValue)
            .toHaveBeenCalledWith(testConfig, 7);
      });
      it('Should record a desktop failure', function() {
        mr.MirrorAnalytics.recordCapturingFailureWithName(
            mr.MirrorAnalytics.CapturingFailure.DESKTOP_FAIL, metricName);
        expect(chrome.metricsPrivate.recordValue.calls.count()).toBe(1);
        expect(chrome.metricsPrivate.recordValue)
            .toHaveBeenCalledWith(testConfig, 8);
      });
      it('Should record an unknown error', function() {
        mr.MirrorAnalytics.recordCapturingFailureWithName(
            mr.MirrorAnalytics.CapturingFailure.UNKNOWN, metricName);
        expect(chrome.metricsPrivate.recordValue.calls.count()).toBe(1);
        expect(chrome.metricsPrivate.recordValue)
            .toHaveBeenCalledWith(testConfig, 9);
      });
    });
  });
});
