// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

goog.module('mr.DialProviderTest');
goog.setTestOnly('mr.DialProviderTest');

const Activity = goog.require('mr.dial.Activity');
const DialClient = goog.require('mr.dial.Client');
const DialProvider = goog.require('mr.DialProvider');
const DialSink = goog.require('mr.dial.Sink');
const PersistentDataManager = goog.require('mr.PersistentDataManager');
const PresentationConnectionState = goog.require('mr.PresentationConnectionState');
const Route = goog.require('mr.Route');
const SinkAvailability = goog.require('mr.SinkAvailability');


describe('DialProvider tests', function() {
  let provider;
  let mockPmCallbacks;
  const pmCallbackMethods = [
    'getRouteMessageEventTarget', 'getProviderFromRouteId',
    'onPresentationConnectionClosed', 'onPresentationConnectionStateChanged',
    'onRouteMessage', 'onRouteRemoved', 'onRouteAdded', 'onSinksUpdated',
    'onSinkAvailabilityUpdated'
  ];
  let mockSinkDiscoveryService;
  const sinkDiscoveryServiceMethods =
      ['init', 'getSinkCount', 'init', 'getSinksByAppName', 'getSinkById'];
  let mockAppDiscoveryService;
  const appDiscoveryServiceMethods = [
    'init', 'start', 'stop', 'registerApp', 'unregisterApp', 'getAppCount',
    'scanSink'
  ];
  let mockDialClient;

  const appInfo = new DialClient.AppInfo();
  const youTubeUrl = 'dial:YouTube?postData=dj0xMjM=';

  beforeEach(function() {
    mockPmCallbacks =
        jasmine.createSpyObj('ProviderManagerCallbacks', pmCallbackMethods);

    mockSinkDiscoveryService = jasmine.createSpyObj(
        'SinkDiscoveryService', sinkDiscoveryServiceMethods);
    mockAppDiscoveryService =
        jasmine.createSpyObj('AppDiscoveryService', appDiscoveryServiceMethods);

    provider = new DialProvider(
        mockPmCallbacks, mockSinkDiscoveryService, mockAppDiscoveryService);
    provider.initialize({enable_dial_discovery: true});
    expect(mockAppDiscoveryService.init).toHaveBeenCalled();

    const fakeDialSink = new DialSink('Fake DIAL sink', 'uniqueId');
    fakeDialSink.setDialAppUrl(youTubeUrl);
    mockDialClient = jasmine.createSpyObj(
        'dialClient', ['getAppInfo', 'launchApp', 'stopApp']);
    spyOn(DialProvider.prototype, 'newClient_').and.returnValue(mockDialClient);
    mockSinkDiscoveryService.getSinkById.and.returnValue(fakeDialSink);
    appInfo.name = 'YouTube';
    appInfo.state = DialClient.DialAppState.STOPPED;
    mr.DialAnalytics.recordCreateRoute = jasmine.createSpy('recordCreateRoute');
  });

  afterEach(function() {
    PersistentDataManager.clear();
  });

  describe('startObservingMediaSinks Test', function() {
    it('Handles non-dial sink query', function() {
      provider.startObservingMediaSinks('urn:not-dial:YouTube');
      expect(mockAppDiscoveryService.registerApp).not.toHaveBeenCalled();
    });

    it('Handles valid dial sink query, no sinks', function() {
      mockSinkDiscoveryService.getSinkCount.and.returnValue(0);
      mockAppDiscoveryService.getAppCount.and.returnValue(1);
      provider.startObservingMediaSinks(youTubeUrl);
      expect(mockAppDiscoveryService.registerApp)
          .toHaveBeenCalledWith('YouTube');
      expect(mockAppDiscoveryService.start).not.toHaveBeenCalled();
    });

    it('Handles valid dial sink query, at least one sink', function() {
      mockSinkDiscoveryService.getSinkCount.and.returnValue(1);
      mockAppDiscoveryService.getAppCount.and.returnValue(1);
      provider.startObservingMediaSinks(youTubeUrl);
      expect(mockAppDiscoveryService.registerApp)
          .toHaveBeenCalledWith('YouTube');
      expect(mockAppDiscoveryService.start).toHaveBeenCalled();
    });
  });

  describe('stopObservingMediaSinks Test', function() {
    it('Handles non-dial sink query', function() {
      provider.stopObservingMediaSinks('urn:not-dial:YouTube');
      expect(mockAppDiscoveryService.unregisterApp).not.toHaveBeenCalled();
    });

    it('Handles valid dial sink query', function() {
      mockAppDiscoveryService.getAppCount.and.returnValue(0);
      provider.stopObservingMediaSinks(youTubeUrl);
      expect(mockAppDiscoveryService.unregisterApp)
          .toHaveBeenCalledWith('YouTube');
    });

    it('Handles valid dial sink query, app query remains', function() {
      mockAppDiscoveryService.getAppCount.and.returnValue(1);
      provider.stopObservingMediaSinks(youTubeUrl);
      expect(mockAppDiscoveryService.unregisterApp)
          .toHaveBeenCalledWith('YouTube');
    });
  });

  describe('onPresentationConnectionStateChanged Test', function() {
    it('Changes presentation state to terminated', function() {
      const route = provider.addRoute(
          'sink1', youTubeUrl, true, 'app1', 'presentationId1');
      provider.onActivityRemoved(new Activity(route, 'app1'));
      expect(mockPmCallbacks.onPresentationConnectionStateChanged)
          .toHaveBeenCalledWith(
              route.id, PresentationConnectionState.TERMINATED);
      expect(mockPmCallbacks.onRouteRemoved).toHaveBeenCalled();
    });

    it('Does not change state for non-local presentation', function() {
      const route = provider.addRoute(
          'sink1', youTubeUrl, false, 'app1', 'presentationId1');
      provider.onActivityRemoved(new Activity(route, 'app1'));
      expect(mockPmCallbacks.onPresentationConnectionStateChanged)
          .not.toHaveBeenCalled();
      expect(mockPmCallbacks.onRouteRemoved).toHaveBeenCalled();
    });
  });

  describe('createRoute Test', function() {
    it('Creates a route', function(done) {
      mockDialClient.getAppInfo.and.returnValue(Promise.resolve(appInfo));
      mockDialClient.launchApp.and.returnValue(Promise.resolve());
      provider.createRoute(youTubeUrl, 'sink1', 'presentationId1', false)
          .promise.then(
              route => {
                expect(route.sinkId).toBe('sink1');
                expect(route.mediaSource).toBe(youTubeUrl);
                expect(route.offTheRecord).toBe(false);
                expect(mr.DialAnalytics.recordCreateRoute)
                    .toHaveBeenCalledWith(
                        mr.DialAnalytics.DialRouteCreation.ROUTE_CREATED);
                done();
              },
              e => {
                done.fail('Unexpected error: ' + e.message);
              });
    });

    it('Creates an off-the-record route', function(done) {
      mockDialClient.getAppInfo.and.returnValue(Promise.resolve(appInfo));
      mockDialClient.launchApp.and.returnValue(Promise.resolve());
      provider.createRoute(youTubeUrl, 'sink1', 'presentationId1', true)
          .promise.then(
              route => {
                expect(route.sinkId).toBe('sink1');
                expect(route.mediaSource).toBe(youTubeUrl);
                expect(route.offTheRecord).toBe(true);
                expect(mr.DialAnalytics.recordCreateRoute)
                    .toHaveBeenCalledWith(
                        mr.DialAnalytics.DialRouteCreation.ROUTE_CREATED);
                done();
              },
              e => {
                done.fail('Unexpected error: ' + e.message);
              });
    });

    it('Fails to create a route when get app info fails', function(done) {
      mockDialClient.getAppInfo.and.returnValue(Promise.reject('fail'));
      provider.createRoute(youTubeUrl, 'sink1', 'presentationId1', true)
          .promise.then(done.fail, e => {
            expect(mr.DialAnalytics.recordCreateRoute)
                .toHaveBeenCalledWith(
                    mr.DialAnalytics.DialRouteCreation.FAILED_LAUNCH_APP);
            done();
          });
    });

    it('Fails to create a route when launch app fails', function(done) {
      mockDialClient.getAppInfo.and.returnValue(Promise.resolve(appInfo));
      mockDialClient.launchApp.and.returnValue(Promise.reject('fail'));
      provider.createRoute(youTubeUrl, 'sink1', 'presentationId1', true)
          .promise.then(done.fail, e => {
            expect(mr.DialAnalytics.recordCreateRoute)
                .toHaveBeenCalledWith(
                    mr.DialAnalytics.DialRouteCreation.FAILED_LAUNCH_APP);
            done();
          });
    });

    it('Starts and stop app discovery', function() {
      mockSinkDiscoveryService.getSinkCount.and.returnValue(1);
      let appCount = 0;
      mockAppDiscoveryService.getAppCount.and.callFake(() => appCount);
      const route = Route.createRoute(
          'presentationId', 'providerName', 'sinkId', 'source', true,
          'description', null);
      const activity = new Activity(route, 'YouTube');
      provider.activityRecords_.add(activity);
      expect(mockAppDiscoveryService.start).toHaveBeenCalled();

      appCount++;
      provider.startObservingMediaSinks(youTubeUrl);
      expect(mockAppDiscoveryService.registerApp)
          .toHaveBeenCalledWith('YouTube');

      appCount--;
      provider.stopObservingMediaSinks(youTubeUrl);
      expect(mockAppDiscoveryService.stop).not.toHaveBeenCalled();

      provider.activityRecords_.removeByRouteId(route.id);
      expect(mockAppDiscoveryService.stop).toHaveBeenCalled();
    });

    it('Sets SinkAvailability to UNAVAILABLE if no more sinks', () => {
      mockSinkDiscoveryService.getSinkCount.and.returnValue(0);
      // Note: This should also work if an non-empty list if passed in. For
      // simplicity, an empty list is used here.
      provider.onSinksRemoved([]);
      expect(mockPmCallbacks.onSinkAvailabilityUpdated)
          .toHaveBeenCalledWith(provider, SinkAvailability.UNAVAILABLE);
    });
  });

  describe('Disables Dial sink query', function() {
    beforeEach(function() {
      PersistentDataManager.clear();
      provider.initialize({enable_dial_sink_query: false});
      expect(mockAppDiscoveryService.start).not.toHaveBeenCalled();
    });

    it('Starting and stopping observing media sinks does nothing', function() {
      provider.startObservingMediaSinks(youTubeUrl);
      expect(mockAppDiscoveryService.registerApp).not.toHaveBeenCalled();

      provider.stopObservingMediaSinks(youTubeUrl);
      expect(mockAppDiscoveryService.unregisterApp).not.toHaveBeenCalled();
    });

    it('onSinkAdded does not start app discovery', function() {
      const sink = new DialSink('s1', 'sink1');
      provider.onSinkAdded(sink);
      expect(mockAppDiscoveryService.scanSink).not.toHaveBeenCalled();

      const sinkList = provider.getAvailableSinks();
      expect(sinkList.sinks.length).toBe(0);
    });
  });
});
