/**
 * @fileoverview Tests for provider_manager.
 */
goog.setTestOnly('provider_manager_test');

goog.require('mr.CancellablePromise');
goog.require('mr.MediaSourceUtils');
goog.require('mr.MirrorAnalytics');
goog.require('mr.Module');
goog.require('mr.PersistentDataManager');
goog.require('mr.PresentationConnectionCloseReason');
goog.require('mr.PresentationConnectionState');
goog.require('mr.ProviderManager');
goog.require('mr.Route');
goog.require('mr.RouteMessage');
goog.require('mr.RouteRequestError');
goog.require('mr.RouteRequestResultCode');
goog.require('mr.Sink');
goog.require('mr.SinkAvailability');
goog.require('mr.SinkList');
goog.require('mr.UnitTestUtils');
goog.require('mr.mirror.Error');
goog.require('mr.mirror.ServiceName');

describe('Tests ProviderManager', function() {
  let sourceUrn;
  let mockMediaRouterService;
  let providerManager;
  let mockProvider1;
  let mockProvider1Name;
  let mockProvider2;
  let mockBrokenProvider;
  let mockCastMirrorService;
  let mockWebrtcMirrorService;
  let mirrorServiceMap;
  const provider1Routes = [];
  const provider2Routes = [];
  const providerMethods = [
    'getName', 'initialize', 'getAvailableSinks', 'createRoute',
    'terminateRoute', 'sendRouteMessage', 'getSinkById', 'getMirrorSettings',
    'getMirrorServiceName', 'canRoute', 'startObservingMediaSinks',
    'stopObservingMediaSinks', 'startObservingMediaRoutes',
    'stopObservingMediaRoutes', 'getRoutes', 'canJoin', 'searchSinks',
    'createMediaRouteController'
  ];
  const mirrorServiceMethods = [
    'initialize',
    'getName',
    'startMirroring',
    'stopCurrentMirroring',
    'createMirrorSession',
    'updateMirroring',
  ];
  const castMirrorServiceMethods = mirrorServiceMethods.slice();
  const presentationId = 'presentationId';
  const routeId = '123';

  let mockClock;

  beforeEach(function() {
    mr.PersistentDataManager.clear();
    mr.Module.clearForTest();
    mr.UnitTestUtils.mockMojoApi();
    sourceUrn = 'urn:dial-multiscreen-org:dial:application:YouTube';
    mockClock = null;
    window['chrome'] = chrome || {};
    chrome['runtime'] = chrome.runtime || {};
    chrome.runtime.id = '123';
    chrome.runtime.getManifest = function() {
      return {version: 'fakeVersion'};
    };
    mockProvider1 = jasmine.createSpyObj('provider1', providerMethods);
    mockProvider1Name = 'p1';
    mockProvider1.getName.and.returnValue(mockProvider1Name);
    mockProvider1.getRoutes.and.callFake(() => provider1Routes);
    mockProvider2 = jasmine.createSpyObj('provider2', providerMethods);
    mockProvider2.getName.and.returnValue('p2');
    mockProvider2.getRoutes.and.callFake(() => provider2Routes);
    mockBrokenProvider =
        jasmine.createSpyObj('brokenProvider', providerMethods);
    mockBrokenProvider.initialize.and.throwError(
        new Error('I forgot how to initialize. @_@'));
    mockCastMirrorService =
        jasmine.createSpyObj('castMirrorService', castMirrorServiceMethods);
    mockCastMirrorService.getName.and.returnValue(
        mr.mirror.ServiceName.CAST_STREAMING);
    mockWebrtcMirrorService =
        jasmine.createSpyObj('webrtcMirrorService', mirrorServiceMethods);
    mockWebrtcMirrorService.getName.and.returnValue(
        mr.mirror.ServiceName.WEBRTC);
    mirrorServiceMap = new Map();
    mirrorServiceMap.set(
        mr.mirror.ServiceName.CAST_STREAMING, mockCastMirrorService);
    mirrorServiceMap.set(mr.mirror.ServiceName.WEBRTC, mockWebrtcMirrorService);

    // These two not needed?
    spyOn(mr.mirror.cast, 'Service').and.returnValue(mockCastMirrorService);
    spyOn(mr.mirror.webrtc, 'WebRtcService')
        .and.returnValue(mockWebrtcMirrorService);
    mockMediaRouterService = jasmine.createSpyObj('mrService', [
      'setKeepAlive', 'getKeepAlive', 'setHandlers',
      'onPresentationConnectionClosed', 'onPresentationConnectionStateChanged',
      'onRoutesUpdated', 'onSinkAvailabilityUpdated', 'onSinksReceived',
      'start', 'onSearchSinkIdReceived', 'onRouteMessagesReceived'
    ]);
    const mockCloudComponentProvider = {
      getIdentityService: function() {
        return {};
      }
    };
    spyOn(mr.cloud.CloudComponentProvider, 'getInstance')
        .and.returnValue(mockCloudComponentProvider);
    providerManager = new mr.ProviderManager();
    expect(mockMediaRouterService.start.calls.count()).toBe(0);
    providerManager.initialize(mockMediaRouterService, []);
    mockMediaRouterService.start.and.returnValue('instance123');
    spyOn(providerManager.mrRouteMessageSender_, 'send').and.callThrough();
    spyOn(providerManager, 'getMirrorService').and.callFake(serviceName => {
      return Promise.resolve(mirrorServiceMap.get(serviceName));
    });
  });

  afterEach(function() {
    if (mockClock) {
      mr.UnitTestUtils.restoreRealClockAndPromises();
    }
  });

  describe('Test presentation connection state changes', function() {
    it('onPresentationConnectionStateChanged', function() {
      const state = mr.PresentationConnectionState.TERMINATED;
      providerManager.onPresentationConnectionStateChanged(routeId, state);
      expect(mockMediaRouterService.onPresentationConnectionStateChanged)
          .toHaveBeenCalledWith(routeId, state);
    });

    it('onPresentationConnectionStateClosed', function() {
      const closeReason = mr.PresentationConnectionCloseReason.WENT_AWAY;
      const message = 'Connection went away';
      providerManager.onPresentationConnectionClosed(
          routeId, closeReason, message);
      expect(mockMediaRouterService.onPresentationConnectionClosed)
          .toHaveBeenCalledWith(routeId, closeReason, message);
    });
  });

  describe('Test registerAllProviders', function() {
    it('Provider is initialized', function() {
      providerManager.registerAllProviders(
          [mockProvider1, mockProvider2, mockBrokenProvider]);
      expect(mockProvider1.initialize.calls.count()).toBe(1);
      expect(mockProvider2.initialize.calls.count()).toBe(1);
      expect(mockBrokenProvider.initialize.calls.count()).toBe(1);
      expect(providerManager.getProviders().some(
                 p => p.getName() == mockProvider1.getName()))
          .toBe(true);
      expect(providerManager.getProviders().some(
                 p => p.getName() == mockProvider2.getName()))
          .toBe(true);
      expect(!providerManager.getProviders().some(
                 p => p.getName() == mockBrokenProvider.getName()))
          .toBe(true);
    });

    it('Route message event is listened to', function() {
      mockClock = mr.UnitTestUtils.useMockClockAndPromises();
      const route = new mr.Route(routeId, 'pId', '0', null, false, '', null);
      const message = 'msg';
      providerManager.registerAllProviders([mockProvider1]);
      providerManager.onRouteAdded(mockProvider1, route);
      providerManager.onRouteMessage(mockProvider1, routeId, message, true);
      providerManager.startListeningForRouteMessages(routeId);
      mockClock.tick(mr.RouteMessageSender.SEND_MESSAGE_INTERVAL_MILLIS);
      expect(mockMediaRouterService.onRouteMessagesReceived)
          .toHaveBeenCalledWith(
              routeId, [new mr.RouteMessage(routeId, message)]);
    });

    it('Message of a closed route is not forwarded', function() {
      const message = 'msg';
      providerManager.registerAllProviders([mockProvider1]);
      providerManager.onRouteMessage(mockProvider1, routeId, message);
      expect(providerManager.mrRouteMessageSender_.send).not.toHaveBeenCalled();
    });

    it('InternalMessage sends and is not forwarded to app', function() {
      const route = new mr.Route(routeId, 'pId', '0', null, false, '', null);
      const message = 'msg';
      providerManager.registerAllProviders([mockProvider1]);
      providerManager.onRouteAdded(mockProvider1, route);
      providerManager.onInternalMessage(mockProvider1, routeId, message);
      expect(providerManager.mrRouteMessageSender_.send).not.toHaveBeenCalled();
    });

    it('start/stopObservingMediaRoutes is passed to provider', function() {
      mockProvider1.getRoutes.and.returnValue([]);
      mockProvider2.getRoutes.and.returnValue([]);
      providerManager.registerAllProviders([mockProvider1, mockProvider2]);

      expect(mockProvider1.startObservingMediaRoutes.calls.count()).toBe(0);
      expect(mockProvider2.startObservingMediaRoutes.calls.count()).toBe(0);
      expect(mockProvider1.stopObservingMediaRoutes.calls.count()).toBe(0);
      expect(mockProvider2.stopObservingMediaRoutes.calls.count()).toBe(0);

      providerManager.startObservingMediaRoutes(sourceUrn);
      expect(mockProvider1.startObservingMediaRoutes.calls.count()).toBe(1);
      expect(mockProvider2.startObservingMediaRoutes.calls.count()).toBe(1);
      expect(mockProvider1.stopObservingMediaRoutes.calls.count()).toBe(0);
      expect(mockProvider2.stopObservingMediaRoutes.calls.count()).toBe(0);

      providerManager.stopObservingMediaRoutes(sourceUrn);
      expect(mockProvider1.startObservingMediaRoutes.calls.count()).toBe(1);
      expect(mockProvider2.startObservingMediaRoutes.calls.count()).toBe(1);
      expect(mockProvider1.stopObservingMediaRoutes.calls.count()).toBe(1);
      expect(mockProvider2.stopObservingMediaRoutes.calls.count()).toBe(1);
    });

    it('Routes query result is sent back to MR initially', function() {
      providerManager.startObservingMediaRoutes(sourceUrn);
      expect(mockMediaRouterService.onRoutesUpdated.calls.count()).toBe(1);
      providerManager.stopObservingMediaRoutes(sourceUrn);
      expect(mockMediaRouterService.onRoutesUpdated.calls.count()).toBe(1);
    });

    it('Routes query result is sent back to MR on events', function() {
      mockClock = mr.UnitTestUtils.useMockClockAndPromises();

      const route = new mr.Route(routeId, 'pId', '0', null, false, '', null);

      providerManager.startObservingMediaRoutes(sourceUrn);
      // Send routes right away
      expect(mockMediaRouterService.onRoutesUpdated.calls.count()).toBe(1);

      // Call to MR is throttled.
      mockClock.tick(mr.ProviderManager.ALL_QUERIES_INTERVAL_MS_ / 2);
      providerManager.onRouteAdded(mockProvider1, route);
      expect(mockMediaRouterService.onRoutesUpdated.calls.count()).toBe(1);
      providerManager.onRouteRemoved(mockProvider1, route);
      expect(mockMediaRouterService.onRoutesUpdated.calls.count()).toBe(1);
      providerManager.onRouteAdded(mockProvider1, route);
      expect(mockMediaRouterService.onRoutesUpdated.calls.count()).toBe(1);
      mockClock.tick(mr.ProviderManager.ALL_QUERIES_INTERVAL_MS_ + 1);
      expect(mockMediaRouterService.onRoutesUpdated.calls.count()).toBe(2);
    });

    it('Routes query result is not sent back to MR if stopped', function() {
      mockClock = mr.UnitTestUtils.useMockClockAndPromises();

      const route = new mr.Route(routeId, 'pId', '0', null, false, '', null);

      providerManager.startObservingMediaRoutes(sourceUrn);
      expect(mockMediaRouterService.onRoutesUpdated.calls.count()).toBe(1);

      mockClock.tick(mr.ProviderManager.ALL_QUERIES_INTERVAL_MS_ / 2);
      providerManager.onRouteAdded(mockProvider1, route);
      expect(mockMediaRouterService.onRoutesUpdated.calls.count()).toBe(1);
      providerManager.onRouteRemoved(mockProvider1, route);
      providerManager.onRouteAdded(mockProvider1, route);
      providerManager.stopObservingMediaRoutes(sourceUrn);
      // Query was stopped to MR is not called.
      mockClock.tick(mr.ProviderManager.ALL_QUERIES_INTERVAL_MS_ / 2 + 1);
      expect(mockMediaRouterService.onRoutesUpdated.calls.count()).toBe(1);

    });

    it('Multiple route queries do not cause duplicate routes', function() {
      mockClock = mr.UnitTestUtils.useMockClockAndPromises();

      const route = new mr.Route(routeId, 'pId', '0', null, false, '', null);
      const sourceUrn2 = '';
      // Only one route will be returned.
      mockProvider1.getRoutes.and.returnValue([route]);
      mockProvider1.canJoin.and.returnValue(false);
      providerManager.registerAllProviders([mockProvider1]);

      // Multiple route queries means the one route should be returned for
      // multiple route queries.
      providerManager.startObservingMediaRoutes(sourceUrn);
      providerManager.startObservingMediaRoutes(sourceUrn2);
      // Call to MR is throttled - so this will only happen once.
      expect(mockMediaRouterService.onRoutesUpdated.calls.count()).toBe(1);
      expect(mockMediaRouterService.onRoutesUpdated)
          .toHaveBeenCalledWith([route], sourceUrn, []);
      mockClock.tick(mr.ProviderManager.ALL_QUERIES_INTERVAL_MS_ + 1);
      // This will fire twice because there are two route queries, bringing the
      // total to 3 times.
      expect(mockMediaRouterService.onRoutesUpdated.calls.count()).toBe(3);
      expect(mockMediaRouterService.onRoutesUpdated)
          .toHaveBeenCalledWith([route], sourceUrn, []);
      expect(mockMediaRouterService.onRoutesUpdated)
          .toHaveBeenCalledWith([route], sourceUrn2, []);
    });
  });

  describe('Test keep alive', function() {
    it('Keep alive for 1 provider', function() {
      providerManager.requestKeepAlive(mockProvider1, true);
      expect(mockMediaRouterService.setKeepAlive).toHaveBeenCalledWith(true);
      providerManager.requestKeepAlive(mockProvider1, false);
      expect(mockMediaRouterService.setKeepAlive).toHaveBeenCalledWith(false);
    });

    it('Keep alive for more than 1 provider', function() {
      providerManager.requestKeepAlive(mockProvider1, true);
      expect(mockMediaRouterService.setKeepAlive).toHaveBeenCalledWith(true);
      providerManager.requestKeepAlive(mockProvider2, true);
      expect(mockMediaRouterService.setKeepAlive).toHaveBeenCalledWith(true);
      providerManager.requestKeepAlive(mockProvider1, false);
      expect(mockMediaRouterService.setKeepAlive).toHaveBeenCalledWith(true);
      providerManager.requestKeepAlive(mockProvider2, false);
      expect(mockMediaRouterService.setKeepAlive).toHaveBeenCalledWith(false);
    });
  });

  describe('Test createRoute', function() {
    let sinkId;
    let localRoute;

    beforeEach(function() {
      sourceUrn = 'urn:dial-multiscreen-org:dial:application:YouTube';
      sinkId = 'sink1';
      localRoute = new mr.Route('r2', 'pId', sinkId, sourceUrn, true, '', null);
      providerManager.registerAllProviders([mockProvider1, mockProvider2]);
    });

    it('No provider can handle it', function(done) {
      mockProvider1.canRoute.and.returnValue(false);
      mockProvider2.canRoute.and.returnValue(false);
      providerManager.createRoute(sourceUrn, sinkId, presentationId)
          .catch(e => {
            expect(e instanceof mr.RouteRequestError).toBe(true);
            expect(e.errorCode)
                .toEqual(mr.RouteRequestResultCode.NO_SUPPORTED_PROVIDER);
            expect(mockProvider1.canRoute)
                .toHaveBeenCalledWith(sourceUrn, sinkId);
            expect(mockProvider2.canRoute)
                .toHaveBeenCalledWith(sourceUrn, sinkId);

            expect(mockMediaRouterService.setKeepAlive.calls.count()).toBe(0);

            done();
          });
    });

    it('One provider can handle, but fail to create session', function(done) {
      mockProvider1.canRoute.and.returnValue(true);
      mockProvider2.canRoute.and.returnValue(false);
      mockProvider1.createRoute.and.returnValue(
          mr.CancellablePromise.reject(Error('err')));
      providerManager.createRoute(sourceUrn, sinkId, presentationId)
          .catch(e => {
            expect(e instanceof mr.RouteRequestError).toBe(true);
            expect(e.errorCode)
                .toEqual(mr.RouteRequestResultCode.UNKNOWN_ERROR);
            expect(mockProvider1.createRoute.calls.count()).toBe(1);
            expect(mockProvider1.createRoute)
                .toHaveBeenCalledWith(
                    sourceUrn, sinkId, presentationId, false,
                    mr.ProviderManager.CREATE_ROUTE_TIMEOUT_MS, undefined,
                    undefined);
            expect(mockProvider2.createRoute.calls.count()).toBe(0);

            expect(mockMediaRouterService.setKeepAlive.calls.argsFor(0))
                .toEqual([true]);
            expect(mockMediaRouterService.setKeepAlive.calls.argsFor(1))
                .toEqual([false]);

            done();
          });
    });

    it('One provider can handle it, and created session', function(done) {
      mockProvider1.canRoute.and.returnValue(true);
      mockProvider2.canRoute.and.returnValue(false);
      mockProvider1.createRoute.and.callFake(() => {
        providerManager.onRouteAdded(mockProvider1, localRoute);
        return mr.CancellablePromise.resolve(localRoute);
      });
      providerManager.createRoute(sourceUrn, sinkId, presentationId).then(r => {
        expect(r).toEqual(localRoute);
        expect(mockProvider1.createRoute.calls.count()).toBe(1);
        expect(mockProvider1.createRoute)
            .toHaveBeenCalledWith(
                sourceUrn, sinkId, presentationId, false,
                mr.ProviderManager.CREATE_ROUTE_TIMEOUT_MS, undefined,
                undefined);
        expect(mockProvider2.createRoute.calls.count()).toBe(0);

        done();
      });
    });

    it('createRoute with custom timeout', function(done) {
      mockProvider1.canRoute.and.returnValue(true);
      mockProvider2.canRoute.and.returnValue(false);
      mockProvider1.createRoute.and.callFake(() => {
        providerManager.onRouteAdded(mockProvider1, localRoute);
        return mr.CancellablePromise.resolve(localRoute);
      });
      const timeoutMillis = 12345;
      providerManager
          .createRoute(
              sourceUrn, sinkId, presentationId, undefined, undefined,
              timeoutMillis)
          .then(r => {
            expect(r).toEqual(localRoute);
            expect(mockProvider1.createRoute.calls.count()).toBe(1);
            expect(mockProvider1.createRoute)
                .toHaveBeenCalledWith(
                    sourceUrn, sinkId, presentationId, false, timeoutMillis,
                    undefined, undefined);
            expect(mockProvider2.createRoute.calls.count()).toBe(0);

            done();
          });
    });

    it('a mirror session is created', function(done) {
      sourceUrn = mr.MediaSourceUtils.DESKTOP_MIRROR_URN;
      localRoute = new mr.Route('r2', 'pId', sinkId, sourceUrn, true, '', null);
      mockProvider1.canRoute.and.returnValue(true);
      mockProvider2.canRoute.and.returnValue(false);
      const mirrorSettings = {};
      mockProvider1.getMirrorSettings.and.returnValue(mirrorSettings);
      mockProvider1.getMirrorServiceName.and.returnValue(
          mr.mirror.ServiceName.CAST_STREAMING);
      mockProvider1.createRoute.and.callFake(() => {
        providerManager.onRouteAdded(this, localRoute);
        return mr.CancellablePromise.resolve(localRoute);
      });
      mockCastMirrorService.startMirroring.and.returnValue(
          mr.CancellablePromise.resolve(localRoute));
      expect(providerManager.getLastUsedMirrorService()).toBeNull();
      providerManager.createRoute(sourceUrn, sinkId, presentationId)
          .then(route => {
            providerManager.startMirroring(mockProvider1, route, presentationId)
                .promise.then(r => {
                  expect(mockProvider1.createRoute.calls.count()).toBe(1);
                  expect(mockProvider1.createRoute)
                      .toHaveBeenCalledWith(
                          sourceUrn, sinkId, presentationId, false,
                          mr.ProviderManager.CREATE_ROUTE_TIMEOUT_MS, undefined,
                          undefined);
                  expect(mockProvider2.createRoute.calls.count()).toBe(0);
                  expect(mockCastMirrorService.startMirroring.calls.count())
                      .toBe(1);
                  expect(mockCastMirrorService.startMirroring)
                      .toHaveBeenCalledWith(
                          localRoute, sourceUrn, mirrorSettings, presentationId,
                          undefined);
                  expect(providerManager.routeIdToProvider_.has(localRoute.id))
                      .toBe(true);
                  expect(providerManager.routeIdToMirrorServiceName_.has(
                             localRoute.id))
                      .toBe(true);
                  expect(providerManager.getLastUsedMirrorService())
                      .toBe(mr.mirror.ServiceName.CAST_STREAMING);
                  done();
                });
          });
    });

    it('a mirror session is not created', function(done) {
      sourceUrn = mr.MediaSourceUtils.DESKTOP_MIRROR_URN;
      localRoute = new mr.Route('r2', 'pId', sinkId, sourceUrn, true, '', null);
      mockProvider1.canRoute.and.returnValue(true);
      mockProvider2.canRoute.and.returnValue(false);
      const mirrorSettings = {};
      mockProvider1.getMirrorSettings.and.returnValue(mirrorSettings);
      mockProvider1.getMirrorServiceName.and.returnValue(
          mr.mirror.ServiceName.CAST_STREAMING);
      mockProvider1.createRoute.and.callFake(() => {
        providerManager.onRouteAdded(mockProvider1, localRoute);
        return mr.CancellablePromise.resolve(localRoute);
      });
      const error = Error('failed to mirror');
      mockCastMirrorService.startMirroring.and.callFake(
          () => mr.CancellablePromise.reject(error));

      providerManager.createRoute(sourceUrn, sinkId, presentationId)
          .then(route => {
            providerManager.startMirroring(mockProvider1, route, presentationId)
                .promise.catch(err => {
                  expect(err).toEqual(error);
                  expect(mockProvider1.createRoute.calls.count()).toBe(1);
                  expect(mockProvider1.createRoute)
                      .toHaveBeenCalledWith(
                          sourceUrn, sinkId, presentationId, false,
                          mr.ProviderManager.CREATE_ROUTE_TIMEOUT_MS, undefined,
                          undefined);
                  expect(mockProvider2.createRoute.calls.count()).toBe(0);
                  expect(mockCastMirrorService.startMirroring.calls.count())
                      .toBe(1);
                  expect(mockCastMirrorService.startMirroring)
                      .toHaveBeenCalledWith(
                          localRoute, sourceUrn, mirrorSettings, presentationId,
                          undefined);
                  // Call onMirrorSessionEnded as the mirror service would.

                  providerManager.onMirrorSessionEnded(route.id);
                  // Call onRouteRemoved as the provider would.

                  providerManager.onRouteRemoved(mockProvider1, route);
                  expect(providerManager.routeIdToProvider_.has(localRoute.id))
                      .toBe(false);
                  expect(providerManager.routeIdToMirrorServiceName_.has(
                             localRoute.id))
                      .toBe(false);

                  done();
                });
          });
    });

    it('an mr.RouteRequestError is returned for user cancellation', (done) => {
      mockProvider1.canRoute.and.returnValue(true);
      mockProvider1.getMirrorSettings.and.returnValue({});
      mockProvider1.getMirrorServiceName.and.returnValue(
          mr.mirror.ServiceName.CAST_STREAMING);
      const error = new mr.mirror.Error(
          '',
          mr.MirrorAnalytics.CapturingFailure
              .CAPTURE_DESKTOP_FAIL_ERROR_USER_CANCEL);
      mockCastMirrorService.startMirroring.and.callFake(
          () => mr.CancellablePromise.reject(error));

      providerManager
          .startMirroring(
              mockProvider1,
              new mr.Route(
                  'r2', 'pId', sinkId, mr.MediaSourceUtils.DESKTOP_MIRROR_URN,
                  true, '', null),
              presentationId)
          .promise.catch(err => {
            expect(err instanceof mr.RouteRequestError).toBe(true);
            expect(err.errorCode).toBe(mr.RouteRequestResultCode.CANCELLED);
            done();
          });
    });

    it('createRoute with default timeout', function(done) {
      mockClock = mr.UnitTestUtils.useMockClockAndPromises();

      mockProvider1.canRoute.and.returnValue(true);
      mockProvider2.canRoute.and.returnValue(false);
      // The provider will never resolve the promise. Provider manager will
      // reject it on timeout.
      mockProvider1.createRoute.and.returnValue(
          new mr.CancellablePromise(() => {}));
      providerManager.createRoute(sourceUrn, sinkId, presentationId)
          .then(
              r => {
                fail('Route unexpectedly created');
              },
              e => {
                expect(e instanceof mr.RouteRequestError).toBe(true);
                expect(e.errorCode)
                    .toEqual(mr.RouteRequestResultCode.TIMED_OUT);
                expect(e.message).toMatch('timeout');
                done();
              });
      mockClock.tick(mr.ProviderManager.CREATE_ROUTE_TIMEOUT_MS + 1);
    });

    it('createRoute with custom timeout', function(done) {
      mockClock = mr.UnitTestUtils.useMockClockAndPromises();

      mockProvider1.canRoute.and.returnValue(true);
      mockProvider2.canRoute.and.returnValue(false);
      // The provider will never resolve the promise. Provider manager will
      // reject it on timeout.
      mockProvider1.createRoute.and.returnValue(
          new mr.CancellablePromise(() => {}));
      const timeoutMillis = 20 * 1000;
      providerManager
          .createRoute(
              sourceUrn, sinkId, 'presentationId', 'origin', 0, timeoutMillis)
          .then(
              r => {
                fail('Route unexpectedly created');
              },
              e => {
                expect(e instanceof mr.RouteRequestError).toBe(true);
                expect(e.errorCode)
                    .toEqual(mr.RouteRequestResultCode.TIMED_OUT);
                expect(e.message).toMatch('timeout');
                done();
              });
      mockClock.tick(timeoutMillis + 1);
    });

    it('update mirror session', function(done) {
      localRoute = new mr.Route('r2', 'pId', sinkId, sourceUrn, true, '', null);
      mockProvider1.canRoute.and.returnValue(true);
      mockProvider2.canRoute.and.returnValue(false);
      const mirrorSettings = {};
      mockProvider1.getMirrorSettings.and.returnValue(mirrorSettings);
      mockProvider1.getMirrorServiceName.and.returnValue(
          mr.mirror.ServiceName.CAST_STREAMING);
      mockProvider1.createRoute.and.callFake(() => {
        providerManager.onRouteAdded(this, localRoute);
        return mr.CancellablePromise.resolve(localRoute);
      });
      mockCastMirrorService.startMirroring.and.returnValue(
          mr.CancellablePromise.resolve(localRoute));
      mockCastMirrorService.updateMirroring.and.returnValue(
          mr.CancellablePromise.resolve(localRoute));
      providerManager.createRoute(sourceUrn, sinkId, presentationId)
          .then(route => {
            providerManager.startMirroring(mockProvider1, route, presentationId)
                .promise.then(r => {
                  expect(mockProvider1.createRoute.calls.count()).toBe(1);
                  expect(mockProvider1.createRoute)
                      .toHaveBeenCalledWith(
                          sourceUrn, sinkId, presentationId, false,
                          mr.ProviderManager.CREATE_ROUTE_TIMEOUT_MS, undefined,
                          undefined);
                  expect(mockProvider2.createRoute.calls.count()).toBe(0);
                  expect(mockCastMirrorService.startMirroring.calls.count())
                      .toBe(1);
                  expect(mockCastMirrorService.startMirroring)
                      .toHaveBeenCalledWith(
                          localRoute, sourceUrn, mirrorSettings, presentationId,
                          undefined);
                  expect(providerManager.routeIdToProvider_.has(localRoute.id))
                      .toBe(true);
                  expect(providerManager.routeIdToMirrorServiceName_.has(
                             localRoute.id))
                      .toBe(true);

                  const newSourceUrn = mr.MediaSourceUtils.DESKTOP_MIRROR_URN;
                  const callback = function() {};
                  providerManager
                      .updateMirroring(
                          mockProvider1, r, newSourceUrn, presentationId,
                          undefined, callback)
                      .promise.then(updatedRoute => {
                        expect(
                            mockCastMirrorService.updateMirroring.calls.count())
                            .toBe(1);
                        expect(mockCastMirrorService.updateMirroring)
                            .toHaveBeenCalledWith(
                                r, newSourceUrn, mirrorSettings, presentationId,
                                undefined, callback);
                        done();
                      });
                });
          });
    });

    it('updateMirroring before startMirroring fails', function(done) {
      localRoute = new mr.Route('r2', 'pId', sinkId, sourceUrn, true, '', null);
      const callback = function() {};

      providerManager
          .updateMirroring(
              mockProvider1, localRoute, sourceUrn, undefined, callback)
          .promise.catch(done);
    });
  });

  describe('Test PersistentData', function() {
    it('ProviderManager is restored properly as PersistentData', function() {
      providerManager.registerAllProviders([mockProvider1]);
      providerManager.sinkQueries_.add(sourceUrn);
      providerManager.routeQueries_.add(sourceUrn);
      providerManager.routeIdToProvider_.set(routeId, mockProvider1);
      providerManager.sinkAvailabilityMap_.set(
          mockProvider1Name, mr.SinkAvailability.AVAILABLE);

      const expectedSinkQueries = new Set(providerManager.sinkQueries_);
      const expectedRouteQueries = new Set(providerManager.routeQueries_);

      const data = providerManager.getData();
      expect(data.length).toEqual(1);
      const tempData = data[0];
      expect(tempData.providerNames).toEqual([mockProvider1Name]);
      expect(tempData.sinkQueries).toEqual([sourceUrn]);
      expect(tempData.routeQueries).toEqual([sourceUrn]);
      expect(tempData.routeIdToProviderName).toEqual([
        [routeId, mockProvider1Name]
      ]);
      expect(tempData.sinkAvailabilityMap).toEqual([
        [mockProvider1Name, mr.SinkAvailability.AVAILABLE]
      ]);

      // Data is saved to localStorage.
      mr.PersistentDataManager.onSuspend_();

      // Make PersistentDataManager forget providerManager, so it can be
      // registered again.
      mr.PersistentDataManager.dataInstances_.clear();

      providerManager.routeIdToProvider_.clear();
      providerManager.sinkQueries_.clear();
      providerManager.routeQueries_.clear();
      providerManager.sinkAvailabilityMap_.clear();

      // Load data back to providerManager.
      mockProvider1.getAvailableSinks.and.returnValue(mr.SinkList.EMPTY);
      mr.PersistentDataManager.register(providerManager);
      expect(providerManager.sinkQueries_).toEqual(expectedSinkQueries);
      expect(providerManager.routeQueries_).toEqual(expectedRouteQueries);
      expect(providerManager.routeIdToProvider_.get(routeId))
          .toEqual(mockProvider1);
      expect(providerManager.sinkAvailabilityMap_.get(mockProvider1Name))
          .toEqual(mr.SinkAvailability.AVAILABLE);
    });

    it('ProviderManager calls mDNS callbacks if mDNS enabled', function() {
      let callbackRuns = 0;
      const callback = function() {
        ++callbackRuns;
      };
      providerManager.mdnsEnabled_ = true;

      // Data is saved to localStorage.
      mr.PersistentDataManager.onSuspend_();

      // Prevent immediate callback so we are sure it runs in loadSavedData
      providerManager.mdnsEnabled_ = false;
      providerManager.registerMdnsDiscoveryEnabledCallback(callback);
      expect(callbackRuns).toBe(0);

      // Make PersistentDataManager forget providerManager, so it can be
      // registered again.
      mr.PersistentDataManager.dataInstances_.clear();

      // Load data back to providerManager.
      mockProvider1.getAvailableSinks.and.returnValue(mr.SinkList.EMPTY);
      mr.PersistentDataManager.register(providerManager);
      expect(callbackRuns).toBe(1);
    });
  });

  describe('Test onRouteRemoved', function() {
    let sourceUrn;
    let sinkId;
    let localRoute;

    beforeEach(function() {
      sourceUrn = 'urn:dial-multiscreen-org:dial:application:YouTube';
      sinkId = 'sink1';
      localRoute = new mr.Route('r2', 'pId', sinkId, sourceUrn, true, '', null);
      providerManager.registerAllProviders([mockProvider1]);
      spyOn(providerManager.mrRouteMessageSender_, 'onRouteRemoved')
          .and.callThrough();
      sourceUrn = mr.MediaSourceUtils.DESKTOP_MIRROR_URN;
      localRoute = new mr.Route('r2', 'pId', sinkId, sourceUrn, true, '', null);
      mockProvider1.canRoute.and.returnValue(true);
      const mirrorSettings = {};
      mockProvider1.getMirrorSettings.and.returnValue(mirrorSettings);
      mockProvider1.getMirrorServiceName.and.returnValue(
          mr.mirror.ServiceName.CAST_STREAMING);
      mockProvider1.createRoute.and.callFake(() => {
        providerManager.onRouteAdded(mockProvider1, localRoute);
        return mr.CancellablePromise.resolve(localRoute);
      });
      mockCastMirrorService.startMirroring.and.returnValue(
          mr.CancellablePromise.resolve(localRoute));
    });

    it('On a mirror session stopped', function(done) {
      let count = 0;
      const checkDone = () => {
        if (++count == 2) {
          expect(providerManager.routeIdToProvider_.has(localRoute.id))
              .toBe(false);
          expect(providerManager.routeIdToMirrorServiceName_.has(localRoute.id))
              .toBe(false);
          expect(mockCastMirrorService.stopCurrentMirroring).toHaveBeenCalled();
          expect(providerManager.mrRouteMessageSender_.onRouteRemoved)
              .toHaveBeenCalledWith(localRoute.id);
          done();
        }
      };
      mockCastMirrorService.stopCurrentMirroring.and.callFake(checkDone);
      providerManager.mrRouteMessageSender_.onRouteRemoved.and.callFake(
          checkDone);
      providerManager.createRoute(sourceUrn, sinkId, presentationId)
          .then(route => {
            providerManager.startMirroring(mockProvider1, route)
                .promise.then(r => {
                  expect(route.id).toEqual(localRoute.id);
                  expect(providerManager.routeIdToProvider_.has(localRoute.id))
                      .toBe(true);
                  expect(providerManager.routeIdToMirrorServiceName_.has(
                             localRoute.id))
                      .toBe(true);
                  providerManager.onRouteRemoved(mockProvider1, localRoute);
                });
          });
    });
  });

  it('Test add/remove MediaSinksQuery', function() {
    const sourceUrn = 'urn:x-org.chromium.media:source:desktop';
    const sink1 = new mr.Sink('s1', 'sink1');
    const sink2 = new mr.Sink('s2', 'sink2');
    const origins = ['https://www.google.com', 'https://youtube.com'];
    providerManager.registerAllProviders([mockProvider1, mockProvider2]);
    mockProvider1.getAvailableSinks.and.returnValue(
        new mr.SinkList([sink1, sink2], origins));
    mockProvider2.getAvailableSinks.and.returnValue(mr.SinkList.EMPTY);
    providerManager.startObservingMediaSinks(sourceUrn);
    expect(mockProvider1.startObservingMediaSinks)
        .toHaveBeenCalledWith(sourceUrn);
    expect(mockProvider2.startObservingMediaSinks)
        .toHaveBeenCalledWith(sourceUrn);
    expect(mockMediaRouterService.onSinksReceived)
        .toHaveBeenCalledWith(sourceUrn, [sink1, sink2], jasmine.any(Object));
    const originList =
        mockMediaRouterService.onSinksReceived.calls.argsFor(0)[2];
    expect(originList.length).toBe(2);
    expect(originList[0].scheme).toBe('https');
    expect(originList[0].host).toBe('www.google.com');
    expect(originList[1].scheme).toBe('https');
    expect(originList[1].host).toBe('youtube.com');
    // add again
    providerManager.startObservingMediaSinks(sourceUrn);
    expect(mockProvider1.startObservingMediaSinks.calls.count()).toBe(1);
    expect(mockProvider2.startObservingMediaSinks.calls.count()).toBe(1);
    expect(mockMediaRouterService.onSinksReceived.calls.count()).toBe(2);
    // remove
    providerManager.stopObservingMediaSinks(sourceUrn);
    expect(mockProvider1.stopObservingMediaSinks)
        .toHaveBeenCalledWith(sourceUrn);
    expect(mockProvider2.stopObservingMediaSinks)
        .toHaveBeenCalledWith(sourceUrn);
    // add again
    providerManager.startObservingMediaSinks(sourceUrn);
    expect(mockProvider1.startObservingMediaSinks.calls.count()).toBe(2);
    expect(mockProvider2.startObservingMediaSinks.calls.count()).toBe(2);
    expect(mockMediaRouterService.onSinksReceived.calls.count()).toBe(3);
  });

  it('Test onSinkAvailabilityUpdated', function() {
    providerManager.registerAllProviders([mockProvider1, mockProvider2]);

    providerManager.onSinkAvailabilityUpdated(
        mockProvider1, mr.SinkAvailability.UNAVAILABLE);
    expect(mockMediaRouterService.onSinkAvailabilityUpdated)
        .not.toHaveBeenCalled();

    providerManager.onSinkAvailabilityUpdated(
        mockProvider1, mr.SinkAvailability.PER_SOURCE);
    expect(mockMediaRouterService.onSinkAvailabilityUpdated)
        .toHaveBeenCalledWith(mr.SinkAvailability.PER_SOURCE);

    providerManager.onSinkAvailabilityUpdated(
        mockProvider2, mr.SinkAvailability.PER_SOURCE);
    expect(mockMediaRouterService.onSinkAvailabilityUpdated.calls.count())
        .toBe(1);

    providerManager.onSinkAvailabilityUpdated(
        mockProvider2, mr.SinkAvailability.AVAILABLE);
    expect(mockMediaRouterService.onSinkAvailabilityUpdated)
        .toHaveBeenCalledWith(mr.SinkAvailability.AVAILABLE);

    providerManager.onSinkAvailabilityUpdated(
        mockProvider1, mr.SinkAvailability.UNAVAILABLE);
    expect(mockMediaRouterService.onSinkAvailabilityUpdated.calls.count())
        .toBe(2);

    providerManager.onSinkAvailabilityUpdated(
        mockProvider2, mr.SinkAvailability.UNAVAILABLE);
    expect(mockMediaRouterService.onSinkAvailabilityUpdated)
        .toHaveBeenCalledWith(mr.SinkAvailability.UNAVAILABLE);
  });

  it('Test mDNS callbacks wait until mDNS is enabled', function() {
    providerManager.mdnsEnabled_ = true;
    let callbackRuns = 0;
    const callback = function() {
      ++callbackRuns;
    };
    providerManager.registerMdnsDiscoveryEnabledCallback(callback);
    // Execute immediately when mDNS discovery is enabled.
    expect(callbackRuns).toBe(1);

    callbackRuns = 0;
    providerManager.mdnsEnabled_ = false;
    providerManager.registerMdnsDiscoveryEnabledCallback(callback);
    // Defer execution until mDNS discovery is enabled.
    expect(callbackRuns).toBe(0);
    providerManager.enableMdnsDiscovery();
    expect(callbackRuns).toBe(1);
  });

  it('Test mDNS callback registration disallows duplicates', function() {
    providerManager.mdnsEnabled_ = true;
    let callbackRuns = 0;
    const callback = function() {
      ++callbackRuns;
    };

    providerManager.mdnsEnabled_ = false;
    providerManager.registerMdnsDiscoveryEnabledCallback(callback);
    providerManager.registerMdnsDiscoveryEnabledCallback(callback);
    // Defer execution until mDNS discovery is enabled and ensure only called
    // once.
    expect(callbackRuns).toBe(0);
    providerManager.enableMdnsDiscovery();
    expect(callbackRuns).toBe(1);
  });

  describe('searchSinks', function() {
    const getSearchCriteria = function(input) {
      return {'input': input, 'domain': 'google.com'};
    };
    let pseudoId;

    beforeEach(function() {
      sourceUrn = 'urn:x-org.chromium.media:source:desktop';
      pseudoId = 'pseudo:' + mockProvider1Name;
      providerManager.registerAllProviders([mockProvider1, mockProvider2]);
    });

    it('returns the sink id if the provider returns a sink', done => {
      const sinkId = 'sinkId1';
      const sinkName = 'sink name';
      const foundSink = new mr.Sink(sinkId, sinkName);
      mockProvider1.searchSinks.and.returnValue(Promise.resolve(foundSink));

      providerManager
          .searchSinks(pseudoId, sourceUrn, getSearchCriteria(sinkName))
          .then((resolvedSinkId) => {
            expect(resolvedSinkId).toBe(sinkId);
            done();
          });
    });

    it('returns nothing if the provider returns nothing', done => {
      mockProvider1.searchSinks.and.returnValue(
          Promise.resolve(new mr.Sink('', '')));
      providerManager
          .searchSinks(pseudoId, sourceUrn, getSearchCriteria('sink name'))
          .then((resolvedSinkId) => expect(resolvedSinkId).toBe(''))
          .then(done, done.fail);
    });

    it('returns nothing when no provider owns the pseudo sink', done => {
      pending(
          'This may never have worked, because the test ' +
          'wasn\'t checking what it was supposed to check.');
      pseudoId = 'pseudo:bad';
      providerManager
          .searchSinks(pseudoId, sourceUrn, getSearchCriteria('sink name'))
          .then((resolvedSinkId) => expect(resolvedSinkId).toBe(''))
          .then(done, done.fail);
    });
  });

  describe('updateMediaSinks', function() {
    const sink1 = new mr.Sink('s1', 'sink1');
    const sink2 = new mr.Sink('s2', 'sink2');

    beforeEach(function() {
      sourceUrn = 'urn:x-org.chromium.media:source:desktop';
      providerManager.registerAllProviders([mockProvider1, mockProvider2]);
      mockProvider1.getAvailableSinks.and.returnValue(
          new mr.SinkList([sink1, sink2], ['https://www.google.com']));
      mockProvider2.getAvailableSinks.and.returnValue(mr.SinkList.EMPTY);
    });

    it('queries providers with desktop source', function() {
      providerManager.updateMediaSinks(sourceUrn);
      expect(mockProvider1.getAvailableSinks).toHaveBeenCalledWith(sourceUrn);
      expect(mockProvider2.getAvailableSinks).toHaveBeenCalledWith(sourceUrn);
      expect(mockMediaRouterService.onSinksReceived)
          .toHaveBeenCalledWith(
              sourceUrn, [sink1, sink2], [jasmine.any(Object)]);
      const originList =
          mockMediaRouterService.onSinksReceived.calls.argsFor(0)[2];
      expect(originList.length).toBe(1);
      expect(originList[0].scheme).toBe('https');
      expect(originList[0].host).toBe('www.google.com');
    });

    it('returns immediately if query already exists', function() {
      providerManager.sinkQueries_.add(sourceUrn);
      // updateMediaSinks should return and not touch providers
      providerManager.updateMediaSinks(sourceUrn);
      expect(mockProvider1.getAvailableSinks).not.toHaveBeenCalled();
      expect(mockProvider2.getAvailableSinks).not.toHaveBeenCalled();
      expect(mockMediaRouterService.onSinksReceived).not.toHaveBeenCalled();
    });
  });

  describe('calls terminateRoute', function() {
    const routeIdToTerminate = 'deadbeef';

    beforeEach(function() {
      providerManager.registerAllProviders([mockProvider1]);
      providerManager.routeIdToProvider_.set(routeIdToTerminate, mockProvider1);
    });

    it('with correct routeId', function(done) {
      mockProvider1.terminateRoute.and.callFake(routeId => {
        expect(routeId).toBe(routeIdToTerminate);
        done();
      });
      providerManager.terminateRoute(routeIdToTerminate);
    });

    it('and returns a rejected promise when it fails', function(done) {
      mockProvider1.terminateRoute.and.callFake(routeId => {
        expect(routeId).toBe(routeIdToTerminate);
        return Promise.reject(new Error('Some error'));
      });
      providerManager.terminateRoute(routeIdToTerminate).then(done.fail, done);
    });

    it('and terminates a mirroring route', function(done) {
      providerManager.routeIdToMirrorServiceName_.set(
          routeIdToTerminate, mr.mirror.ServiceName.CAST_STREAMING);
      providerManager.terminateRoute(routeIdToTerminate).then(() => {
        expect(mockProvider1.terminateRoute)
            .toHaveBeenCalledWith(routeIdToTerminate);
        expect(mockCastMirrorService.stopCurrentMirroring).toHaveBeenCalled();
        expect(
            providerManager.routeIdToMirrorServiceName_.has(routeIdToTerminate))
            .toBe(false);
        done();
      });
    });
  });

  describe('createMediaRouteController tests', () => {
    const routeId = 'deadbeef';
    let controller;
    let observer;

    beforeEach(() => {
      controller = mr.UnitTestUtils.createMojoRequestSpyObj();
      observer = mr.UnitTestUtils.createMojoMediaStatusObserverSpyObj();
      providerManager.registerAllProviders([mockProvider1]);
      providerManager.routeIdToProvider_.set(routeId, mockProvider1);
    });

    it('succeeds', done => {
      mockProvider1.createMediaRouteController.and.callFake(
          () => Promise.resolve());
      providerManager.createMediaRouteController(routeId, controller, observer)
          .then(() => {
            expect(mockProvider1.createMediaRouteController).toHaveBeenCalled();
            expect(controller.close).not.toHaveBeenCalled();
            expect(observer.ptr.reset).not.toHaveBeenCalled();
            done();
          }, fail);
    });

    it('fails if route does not exist', done => {
      providerManager
          .createMediaRouteController(
              'nonexistentRouteId', controller, observer)
          .then(fail, () => {
            expect(controller.close).toHaveBeenCalled();
            expect(observer.ptr.reset).toHaveBeenCalled();
            done();
          });
    });

    it('fails if provider fails', done => {
      mockProvider1.createMediaRouteController.and.callFake(
          () => Promise.reject('Foo'));
      providerManager.createMediaRouteController(routeId, controller, observer)
          .then(fail, () => {
            expect(mockProvider1.createMediaRouteController).toHaveBeenCalled();
            expect(controller.close).toHaveBeenCalled();
            expect(observer.ptr.reset).toHaveBeenCalled();
            done();
          });
    });
  });
});
