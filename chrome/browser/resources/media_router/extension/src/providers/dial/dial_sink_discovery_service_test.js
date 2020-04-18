// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

goog.module('mr.dial.SinkDiscoveryServiceTest');
goog.setTestOnly('mr.dial.SinkDiscoveryServiceTest');

const DialAnalytics = goog.require('mr.DialAnalytics');
const PersistentDataManager = goog.require('mr.PersistentDataManager');
const SinkAppStatus = goog.require('mr.dial.SinkAppStatus');
const SinkDiscoveryService = goog.require('mr.dial.SinkDiscoveryService');
const UnitTestUtils = goog.require('mr.UnitTestUtils');

describe('DIAL SinkDiscoveryService Tests', function() {
  let service;
  let mockClock;
  let mockSinkCallbacks;

  beforeEach(function() {
    mockClock = UnitTestUtils.useMockClockAndPromises();
    mockSinkCallbacks = jasmine.createSpyObj(
        'SinkCallbacks', ['onSinkAdded', 'onSinksRemoved', 'onSinkUpdated']);

    chrome.metricsPrivate = {
      recordTime: jasmine.createSpy('recordTime'),
      recordMediumTime: jasmine.createSpy('recordMediumTime'),
      recordLongTime: jasmine.createSpy('recordLongTime'),
      recordUserAction: jasmine.createSpy('recordUserAction')
    };

    service = new SinkDiscoveryService(mockSinkCallbacks);
    spyOn(DialAnalytics, 'recordDeviceCounts');
  });

  afterEach(function() {
    UnitTestUtils.restoreRealClockAndPromises();
    PersistentDataManager.clear();
  });


  /**
   * Creates mojo sink instances.
   * @param {number} numSinks The number of mojo sinks to create.
   * @return {!Array<!mojo.Sink>} The mojo sinks.
   */
  function createMojoSinks(numSinks) {
    const mojoSinks = [];
    for (var i = 1; i <= numSinks; i++) {
      const dialMediaSink = {
        ip_address: {address_bytes: [127, 0, 0, i]},
        model_name: 'Eureka Dongle',
        app_url: {url: 'http://127.0.0.' + i + ':8008/apps'}
      };

      mojoSinks.push({
        sink_id: 'sinkId ' + i,
        name: 'TV ' + i,
        extra_data: {dial_media_sink: dialMediaSink}
      });
    }
    return mojoSinks;
  }

  describe('addSinks tests', function() {
    beforeEach(function() {
      service.init();
    });

    it('add mojo sinks to sink map', function() {
      expect(service.getSinks().length).toBe(0);
      const mojoSinks = createMojoSinks(1);
      service.addSinks(mojoSinks);

      // sinks were added
      const actualSinks = service.getSinks();
      expect(actualSinks.length).toBe(1);

      const actualSink = actualSinks[0];
      const mojoSink = mojoSinks[0];
      const extraData = mojoSink.extra_data.dial_media_sink;
      expect(actualSink.getFriendlyName()).toEqual(mojoSink.name);
      expect(actualSink.getIpAddress())
          .toEqual(extraData.ip_address.address_bytes.join('.'));
      expect(actualSink.getDialAppUrl()).toEqual(extraData.app_url.url);
      expect(actualSink.getModelName()).toEqual(extraData.model_name);
      expect(actualSink.supportsAppAvailability()).toEqual(false);

      // add-sink-events were fired.
      expect(mockSinkCallbacks.onSinkAdded.calls.count()).toBe(1);
      expect(mockSinkCallbacks.onSinksRemoved.calls.count()).toBe(0);
    });

    it('remove outdated sinks', function() {
      expect(service.getSinks().length).toBe(0);
      const mojoSinks = createMojoSinks(3);
      // First round discover sink 1, 2, 3
      service.addSinks(mojoSinks);

      // Second round discover sink 1
      const mojoSinks2 = createMojoSinks(1);
      service.addSinks(mojoSinks2);
      expect(mockSinkCallbacks.onSinkAdded.calls.count()).toBe(3);

      // 2 devices were removed
      expect(mockSinkCallbacks.onSinksRemoved.calls.count()).toBe(1);
      const sinks = mockSinkCallbacks.onSinksRemoved.calls.argsFor(0)[0];
      expect(sinks.length).toBe(2);
      expect(sinks[0].getFriendlyName()).toEqual(mojoSinks[1].name);
      expect(sinks[1].getFriendlyName()).toEqual(mojoSinks[2].name);

      expect(mockSinkCallbacks.onSinkUpdated.calls.count()).toBe(0);
    });

    it('Gets sinks by app name', function() {
      const mojoSinks = createMojoSinks(3);
      service.addSinks(mojoSinks);
      service.getSinkById(mojoSinks[0].sink_id)
          .setAppStatus('YouTube', SinkAppStatus.AVAILABLE);
      service.getSinkById(mojoSinks[1].sink_id)
          .setAppStatus('Netflix', SinkAppStatus.AVAILABLE);
      service.getSinkById(mojoSinks[1].sink_id)
          .setAppStatus('YouTube', SinkAppStatus.AVAILABLE);
      service.getSinkById(mojoSinks[2].sink_id)
          .setAppStatus('Pandora', SinkAppStatus.UNAVAILABLE);
      expect(service.getSinksByAppName('YouTube').sinks.length).toBe(2);
      expect(service.getSinksByAppName('Netflix').sinks.length).toBe(1);
      expect(service.getSinksByAppName('Netflix').sinks[0].id)
          .toEqual(mojoSinks[1].sink_id);
      expect(service.getSinksByAppName('Pandora').sinks.length).toBe(0);
    });

  });

  it('Saves PersistentData without any data', function() {
    service.init();
    expect(service.getSinks()).toEqual([]);
    PersistentDataManager.suspendForTest();
    service = new SinkDiscoveryService(mockSinkCallbacks);
    service.loadSavedData();
    expect(service.getSinks()).toEqual([]);
  });

  it('Saves PersistentData with data', function() {
    service.init();
    service.addSinks(createMojoSinks(3));
    mockClock.tick(1);
    const sinks = service.getSinks();
    expect(sinks.length).toBe(3);
    const expectedDeviceCounts = {availableDeviceCount: 3, knownDeviceCount: 3};
    expect(service.getDeviceCounts()).toEqual(expectedDeviceCounts);

    PersistentDataManager.suspendForTest();
    service = new SinkDiscoveryService(mockSinkCallbacks);
    service.loadSavedData();
    const restoredSinks = service.getSinks();
    expect(restoredSinks.length).toBe(3);
    expect(service.getDeviceCounts()).toEqual(expectedDeviceCounts);
    for (let index = 0; index < 3; index++) {
      expect(sinks[0].getId()).toEqual(restoredSinks[0].getId());
    }
  });

});
