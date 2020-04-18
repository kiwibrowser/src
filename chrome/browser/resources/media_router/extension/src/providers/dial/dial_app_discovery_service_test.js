// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

goog.module('mr.dial.AppDiscoveryServiceTest');
goog.setTestOnly('mr.dial.AppDiscoveryServiceTest');

const Activity = goog.require('mr.dial.Activity');
const ActivityRecords = goog.require('mr.dial.ActivityRecords');
const AppDiscoveryService = goog.require('mr.dial.AppDiscoveryService');
const DialClient = goog.require('mr.dial.Client');
const DialSink = goog.require('mr.dial.Sink');
const DialSinkAppStatus = goog.require('mr.dial.SinkAppStatus');
const PersistentDataManager = goog.require('mr.PersistentDataManager');
const Route = goog.require('mr.Route');
const UnitTestUtils = goog.require('mr.UnitTestUtils');

describe('DIAL AppDiscoveryService Tests', function() {
  let activityRecords;
  let service;
  let mockClock;
  let mockDiscoveryService;
  let mockActivityCallbacks;
  let sink1;
  let sink2;
  let sink3;
  let mockDialClient;
  let stoppedAppInfo;
  let runningAppInfo;
  let installableAppInfo;
  let stoppedAppInfoWithWebsocket;

  const setUpGetAppInfoResponse = function(appInfo) {
    mockDialClient.getAppInfo.and.callFake(() => Promise.resolve(appInfo));
  };

  const setUpGetAppInfoError = function() {
    mockDialClient.getAppInfo.and.callFake(
        () => Promise.reject(new Error('getAppInfo failed')));
  };

  const setUpGetAppInfoNotFoundError = function() {
    mockDialClient.getAppInfo.and.callFake(
        () => Promise.reject(new DialClient.AppInfoNotFoundError()));
  };

  beforeEach(function() {
    mockClock = UnitTestUtils.useMockClockAndPromises();

    mockDiscoveryService = jasmine.createSpyObj(
        'discoveryService',
        ['getSinks', 'getSinkById', 'getSinkCount', 'onAppStatusChanged']);
    mockActivityCallbacks = jasmine.createSpyObj(
        'activityCallbacks',
        ['onActivityAdded', 'onActivityRemoved', 'onActivityUpdated']);
    activityRecords = new ActivityRecords(mockActivityCallbacks);
    service = new AppDiscoveryService(mockDiscoveryService, activityRecords);

    sink1 = new DialSink('sink1', '1').setSupportsAppAvailability(true);
    sink2 = new DialSink('sink2', '2').setSupportsAppAvailability(true);
    sink3 = new DialSink('sink3', '3').setSupportsAppAvailability(false);
    mockDialClient = jasmine.createSpyObj('dialClient', ['getAppInfo']);
    spyOn(AppDiscoveryService.prototype, 'getDialClient_')
        .and.returnValue(mockDialClient);
    stoppedAppInfo = {'state': DialClient.DialAppState.STOPPED};
    runningAppInfo = {'state': DialClient.DialAppState.RUNNING};
    installableAppInfo = {'state': DialClient.DialAppState.INSTALLABLE};
    stoppedAppInfoWithWebsocket = {
      'name': 'Netflix',
      'state': DialClient.DialAppState.STOPPED,
      'extraData': {'capabilities': 'websocket'}
    };

    chrome.runtime = {
      id: 'fakeId',
      getManifest: function() {
        return {version: 'fakeVersion'};
      }
    };
  });

  afterEach(function() {
    service.stop();
    UnitTestUtils.restoreRealClockAndPromises();
    PersistentDataManager.clear();
  });

  describe('Tests registerApp', function() {
    beforeEach(function() {
      mockDiscoveryService.getSinks.and.returnValue([sink1, sink2, sink3]);
      mockDiscoveryService.getSinkCount.and.returnValue(3);
    });

    const expectAppStatus = function(expectedAppStatus, appName) {
      service.init();

      expect(sink1.getAppStatus(appName)).toEqual(DialSinkAppStatus.UNKNOWN);
      expect(sink2.getAppStatus(appName)).toEqual(DialSinkAppStatus.UNKNOWN);
      expect(sink3.getAppStatus(appName)).toEqual(DialSinkAppStatus.UNKNOWN);

      service.registerApp(appName);

      service.start();
      // Let internal promises to resolve or reject.
      mockClock.tick(1);

      expect(sink1.getAppStatus(appName)).toEqual(expectedAppStatus);
      expect(sink2.getAppStatus(appName)).toEqual(expectedAppStatus);
      expect(sink3.getAppStatus(appName)).toEqual(DialSinkAppStatus.UNKNOWN);
      expect(mockDialClient.getAppInfo.calls.count()).toBe(2);
    };

    it('Response indicates app was stopped', function() {
      setUpGetAppInfoResponse(stoppedAppInfo);
      expectAppStatus(DialSinkAppStatus.AVAILABLE, 'YouTube');
    });

    it('Response indicates app was running', function() {
      setUpGetAppInfoResponse(runningAppInfo);
      expectAppStatus(DialSinkAppStatus.AVAILABLE, 'YouTube');
    });

    it('Response indicates app was installable', function() {
      setUpGetAppInfoResponse(installableAppInfo);
      expectAppStatus(DialSinkAppStatus.UNAVAILABLE, 'YouTube');
    });

    it('Response has invalid app info', function() {
      setUpGetAppInfoError();
      expectAppStatus(DialSinkAppStatus.UNKNOWN, 'YouTube');
      expect(sink1.supportsAppAvailability()).toBe(false);
      expect(sink2.supportsAppAvailability()).toBe(false);
    });

    it('Response indicates not found', function() {
      setUpGetAppInfoNotFoundError();
      expectAppStatus(DialSinkAppStatus.UNAVAILABLE, 'YouTube');
    });

    it('Netflix with special stopped info', function() {
      setUpGetAppInfoResponse(stoppedAppInfoWithWebsocket);
      expectAppStatus(DialSinkAppStatus.AVAILABLE, 'Netflix');
    });

    it('Netflix with normal stopped info', function() {
      stoppedAppInfo.name = 'Netflix';
      setUpGetAppInfoResponse(stoppedAppInfo);
      expectAppStatus(DialSinkAppStatus.UNAVAILABLE, 'Netflix');
    });

    it('No new query generated when registering existing app with known status',
       function() {
         setUpGetAppInfoResponse(stoppedAppInfo);
         expectAppStatus(DialSinkAppStatus.AVAILABLE, 'YouTube');
         // register again
         service.registerApp('YouTube');
         mockClock.tick(1);
         expect(mockDialClient.getAppInfo.calls.count()).toBe(2);
         // unregister does nothing because sink already had the app status.
         service.unregisterApp('YouTube');
         service.registerApp('YouTube');
         mockClock.tick(1);
         expect(mockDialClient.getAppInfo.calls.count()).toBe(2);
         // unless clear app status first.
         sink1.clearAppStatus();
         sink2.clearAppStatus();
         service.unregisterApp('YouTube');
         service.registerApp('YouTube');
         mockClock.tick(1);
         expect(mockDialClient.getAppInfo.calls.count()).toBe(4);
       });

    it('No new query generated when register existing app with unknown status',
       function() {
         setUpGetAppInfoError();
         expectAppStatus(DialSinkAppStatus.UNKNOWN, 'YouTube');
         // Make sink1 have known status. But sink2 has unknown status.
         sink1.setAppStatus('YouTube', DialSinkAppStatus.UNAVAILABLE);
         // register again
         service.registerApp('YouTube');
         mockClock.tick(1);
         // No re-query
         expect(mockDialClient.getAppInfo.calls.count()).toBe(2);
       });
  });

  describe('Tests app status caching and onAppStatusChanged', function() {
    beforeEach(function() {
      mockDiscoveryService.getSinks.and.returnValue([sink1]);
      mockDiscoveryService.getSinkCount.and.returnValue(1);
    });

    it('Known app status does not change during caching period', function() {
      service.init();
      setUpGetAppInfoResponse(stoppedAppInfo);
      expect(sink1.getAppStatus('YouTube')).toEqual(DialSinkAppStatus.UNKNOWN);
      service.registerApp('YouTube');
      service.start();
      mockClock.tick(1);
      expect(sink1.getAppStatus('YouTube'))
          .toEqual(DialSinkAppStatus.AVAILABLE);
      expect(mockDialClient.getAppInfo.calls.count()).toBe(1);
      expect(mockDiscoveryService.onAppStatusChanged.calls.count()).toBe(1);

      // Make app unavailable
      setUpGetAppInfoNotFoundError();
      service.doScan_();
      mockClock.tick(1);
      // No new query and app is still available since cache is not expired.
      expect(mockDialClient.getAppInfo.calls.count()).toBe(1);
      expect(sink1.getAppStatus('YouTube'))
          .toEqual(DialSinkAppStatus.AVAILABLE);
      expect(mockDiscoveryService.onAppStatusChanged.calls.count()).toBe(1);
    });

    it('Does not re-query app status when getAppInfo throws error', function() {
      service.init();
      setUpGetAppInfoError();
      expect(sink1.getAppStatus('YouTube')).toEqual(DialSinkAppStatus.UNKNOWN);
      service.registerApp('YouTube');
      service.start();
      mockClock.tick(1);
      expect(sink1.getAppStatus('YouTube')).toEqual(DialSinkAppStatus.UNKNOWN);
      expect(mockDialClient.getAppInfo.calls.count()).toBe(1);
      expect(mockDiscoveryService.onAppStatusChanged.calls.count()).toBe(0);

      service.doScan_();
      mockClock.tick(1);
      // No new query
      expect(mockDialClient.getAppInfo.calls.count()).toBe(1);
      expect(sink1.getAppStatus('YouTube')).toEqual(DialSinkAppStatus.UNKNOWN);
      expect(mockDiscoveryService.onAppStatusChanged.calls.count()).toBe(0);
    });

    it('Cache expires, re-query, different status', function() {
      service.init();
      setUpGetAppInfoResponse(stoppedAppInfo);
      expect(sink1.getAppStatus('YouTube')).toEqual(DialSinkAppStatus.UNKNOWN);
      service.registerApp('YouTube');
      service.start();
      mockClock.tick(1);
      expect(sink1.getAppStatus('YouTube'))
          .toEqual(DialSinkAppStatus.AVAILABLE);
      expect(mockDialClient.getAppInfo.calls.count()).toBe(1);
      expect(mockDiscoveryService.onAppStatusChanged.calls.count()).toBe(1);

      // Make app unavailable
      setUpGetAppInfoNotFoundError();
      // Make cache expire
      mockClock.tick(AppDiscoveryService.CACHE_PERIOD_);
      service.doScan_();
      mockClock.tick(1);
      // new query and app becomes unavailable
      expect(mockDialClient.getAppInfo.calls.count()).toBe(2);
      expect(sink1.getAppStatus('YouTube'))
          .toEqual(DialSinkAppStatus.UNAVAILABLE);
      expect(mockDiscoveryService.onAppStatusChanged.calls.count()).toBe(2);
    });

    it('Cache expires, re-query, same status', function() {
      service.init();
      setUpGetAppInfoResponse(stoppedAppInfo);
      expect(sink1.getAppStatus('YouTube')).toEqual(DialSinkAppStatus.UNKNOWN);
      service.registerApp('YouTube');
      service.start();
      mockClock.tick(1);
      expect(sink1.getAppStatus('YouTube'))
          .toEqual(DialSinkAppStatus.AVAILABLE);
      expect(mockDialClient.getAppInfo.calls.count()).toBe(1);
      expect(mockDiscoveryService.onAppStatusChanged.calls.count()).toBe(1);

      // Make cache expire
      mockClock.tick(AppDiscoveryService.CACHE_PERIOD_);
      service.doScan_();
      mockClock.tick(1);
      // New query and app becomes unavailable
      expect(mockDialClient.getAppInfo.calls.count()).toBe(2);
      expect(sink1.getAppStatus('YouTube'))
          .toEqual(DialSinkAppStatus.AVAILABLE);
      // Did not trigger app status changed event
      expect(mockDiscoveryService.onAppStatusChanged.calls.count()).toBe(1);
    });
  });

  describe('Tests activity scanning', function() {
    beforeEach(function() {
      mockDiscoveryService.getSinks.and.returnValue([sink1, sink2, sink3]);
      mockDiscoveryService.getSinkCount.and.returnValue(3);
      mockDiscoveryService.getSinkById.and.returnValue(sink1);

      service.init();
      const route = Route.createRoute(
          'presentationId', 'providerName', sink1.getId(), 'source', true,
          'description', null);
      const activity = new Activity(route, 'YouTube');
      activityRecords.add(activity);
      expect(mockActivityCallbacks.onActivityAdded).toHaveBeenCalled();
    });

    it('Activity is removed when app is no longer running', function() {
      setUpGetAppInfoResponse(stoppedAppInfo);
      service.start();
      mockClock.tick(1);
      expect(mockDialClient.getAppInfo.calls.count()).toBe(1);
      expect(mockActivityCallbacks.onActivityRemoved.calls.count()).toBe(1);
      expect(activityRecords.getActivityCount()).toBe(0);
    });

    it('Activity is not removed when app is still running', function() {
      setUpGetAppInfoResponse(runningAppInfo);
      service.start();
      mockClock.tick(1);
      expect(mockDialClient.getAppInfo.calls.count()).toBe(1);
      expect(mockActivityCallbacks.onActivityRemoved).not.toHaveBeenCalled();
      expect(activityRecords.getActivityCount()).toBe(1);

      // Trigger periodic rescan
      mockClock.tick(AppDiscoveryService.CHECK_INTERVAL_MILLIS + 1);
      expect(mockDialClient.getAppInfo.calls.count()).toBe(2);
      expect(mockActivityCallbacks.onActivityRemoved).not.toHaveBeenCalled();
      expect(activityRecords.getActivityCount()).toBe(1);

      // No more periodic rescan.
      service.stop();
      mockClock.tick(AppDiscoveryService.CHECK_INTERVAL_MILLIS + 1);
      expect(mockDialClient.getAppInfo.calls.count()).toBe(2);
    });

    it('Activity scanning not duplicated with app scanning', function() {
      setUpGetAppInfoResponse(stoppedAppInfo);
      service.registerApp('YouTube');
      mockClock.tick(1);
      // One for sink1 and one for sink2. Activity scanning does not issue a
      // duplicated request. Both app status and activity status can be updated
      // with the same response.
      expect(mockDialClient.getAppInfo.calls.count()).toBe(2);
      expect(sink1.getAppStatus('YouTube'))
          .toEqual(DialSinkAppStatus.AVAILABLE);
      expect(sink2.getAppStatus('YouTube'))
          .toEqual(DialSinkAppStatus.AVAILABLE);
      expect(mockActivityCallbacks.onActivityRemoved.calls.count()).toBe(1);
      expect(activityRecords.getActivityCount()).toBe(0);
    });
  });

  it('Persistent with data', function() {
    mockDiscoveryService.getSinks.and.returnValue([]);
    mockDiscoveryService.getSinkCount.and.returnValue(0);
    const mockServiceToSuspend = new AppDiscoveryService(
        mockDiscoveryService, new ActivityRecords(mockActivityCallbacks));
    mockServiceToSuspend.init();
    mockServiceToSuspend.registerApp('app1');
    mockServiceToSuspend.registerApp('app2');
    PersistentDataManager.suspendForTest();

    const mockServiceToLoad = new AppDiscoveryService(
        mockDiscoveryService, new ActivityRecords(mockActivityCallbacks));
    mockServiceToLoad.loadSavedData();
    expect(mockServiceToLoad.getRegisteredApps()).toEqual(['app1', 'app2']);
    expect(mockServiceToLoad.getAppCount()).toEqual(2);
  });
});
