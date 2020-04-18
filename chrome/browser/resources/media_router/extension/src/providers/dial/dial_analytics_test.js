// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

goog.setTestOnly();
goog.require('mr.DialAnalytics');

describe('Dial Analytics', function() {

  beforeEach(function() {
    chrome.metricsPrivate = jasmine.createSpyObj(
        ['recordSmallCount', 'recordUserAction', 'recordValue']);
  });

  it('should record a Dial.Create.Route result', function() {
    const testConfig = {
      'metricName': 'MediaRouter.Dial.Create.Route',
      'type': 'histogram-linear',
      'min': 1,
      'max': 4,
      'buckets': 5
    };
    let numCalls = 0;
    for (key in mr.DialAnalytics.DialRouteCreation) {
      const value = mr.DialAnalytics.DialRouteCreation[key];
      mr.DialAnalytics.recordCreateRoute(value);
      expect(chrome.metricsPrivate.recordValue.calls.count()).toBe(++numCalls);
      expect(chrome.metricsPrivate.recordValue)
          .toHaveBeenCalledWith(testConfig, value);
    }
  });

  it('should not record an unknown Dial.Create.Route result', function() {
    mr.DialAnalytics.recordCreateRoute('test');
    expect(chrome.metricsPrivate.recordValue).not.toHaveBeenCalled();
  });

  it('should record a Dial.Device.Description.Failure result', function() {
    const testConfig = {
      'metricName': 'MediaRouter.Dial.Device.Description.Failure',
      'type': 'histogram-linear',
      'min': 1,
      'max': 3,
      'buckets': 4
    };
    let numCalls = 0;
    for (key in mr.DialAnalytics.DeviceDescriptionFailures) {
      const value = mr.DialAnalytics.DeviceDescriptionFailures[key];
      mr.DialAnalytics.recordDeviceDescriptionFailure(value);
      expect(chrome.metricsPrivate.recordValue.calls.count()).toBe(++numCalls);
      expect(chrome.metricsPrivate.recordValue)
          .toHaveBeenCalledWith(testConfig, value);
    }
  });

  it('should not record an unknown Dial.Device.Description.Failure',
     function() {
       mr.DialAnalytics.recordDeviceDescriptionFailure('test');
       expect(chrome.metricsPrivate.recordValue).not.toHaveBeenCalled();
     });

  it('should record a Device Description From Cache action', function() {
    mr.DialAnalytics.recordDeviceDescriptionFromCache();
    expect(chrome.metricsPrivate.recordUserAction.calls.count()).toBe(1);
    expect(chrome.metricsPrivate.recordUserAction)
        .toHaveBeenCalledWith('MediaRouter.Dial.Device.Description.Cached');
  });

  it('should record a non-Cast Sink Discovery action', function() {
    mr.DialAnalytics.recordNonCastDiscovery();
    expect(chrome.metricsPrivate.recordUserAction.calls.count()).toBe(1);
    expect(chrome.metricsPrivate.recordUserAction)
        .toHaveBeenCalledWith('MediaRouter.Dial.Sink.Discovered.NonCast');
  });

  it('DeviceCounts is recorded with recordSmallCount', () => {
    const deviceCounts = {availableDeviceCount: 5, knownDeviceCount: 8};
    mr.DialAnalytics.recordDeviceCounts(deviceCounts);
    expect(chrome.metricsPrivate.recordSmallCount.calls.count()).toBe(2);
    let args = chrome.metricsPrivate.recordSmallCount.calls.argsFor(0);
    expect(args[0]).toBe(mr.DialAnalytics.AVAILABLE_DEVICES_COUNT_);
    expect(args[1]).toBe(deviceCounts.availableDeviceCount);

    args = chrome.metricsPrivate.recordSmallCount.calls.argsFor(1);
    expect(args[0]).toBe(mr.DialAnalytics.KNOWN_DEVICES_COUNT_);
    expect(args[1]).toBe(deviceCounts.knownDeviceCount);
  });
});
