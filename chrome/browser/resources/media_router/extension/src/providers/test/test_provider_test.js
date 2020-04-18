// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

goog.setTestOnly();
goog.require('mr.PresentationConnectionCloseReason');
goog.require('mr.PresentationConnectionState');
goog.require('mr.Sink');
goog.require('mr.SinkList');
goog.require('mr.TestProvider');



describe('Tests TestProvider', function() {
  const INVALID_SOURCE = 'foo:bar';
  const TEST_SOURCE = 'test:test';
  const PRESENTATION_URL = 'https://www.google.com';
  let mockClock;
  let provider;
  let mockPmCallbacks;

  beforeEach(function() {
    chrome.tabCapture =
        jasmine.createSpyObj('tabCapture', ['captureOffscreenTab']);
    mockPmCallbacks = jasmine.createSpyObj('ProviderManagerCallbacks', [
      'onRouteRemoved', 'onRouteAdded', 'requestKeepAlive',
      'onSinkAvailabilityUpdated', 'onPresentationConnectionStateChanged',
      'onPresentationConnectionClosed'
    ]);
    provider = new mr.TestProvider(mockPmCallbacks);
    provider.initialize();
    delete window.localStorage['testdata'];
  });

  describe('getAvailableSinks Test', function() {
    it('Returns sink list for sources', function() {
      expect(provider.getAvailableSinks(INVALID_SOURCE))
          .toEqual(mr.SinkList.EMPTY);

      const expectedSinkList = new mr.SinkList([
        new mr.Sink('id1', 'test-sink-1'), new mr.Sink('id2', 'test-sink-2')
      ]);
      expect(provider.getAvailableSinks(TEST_SOURCE)).toEqual(expectedSinkList);
      expect(provider.getAvailableSinks(PRESENTATION_URL))
          .toEqual(expectedSinkList);
    });

    it('Returns value from localstorage', function() {
      const sinks =
          [new mr.Sink('id3', 'sink-3'), new mr.Sink('id4', 'sink-4')];
      const sinkMap = {};
      sinkMap[PRESENTATION_URL] = sinks;
      window.localStorage['testdata'] =
          JSON.stringify({'getAvailableSinks': sinkMap});
      expect(provider.getAvailableSinks(TEST_SOURCE))
          .toEqual(mr.SinkList.EMPTY);
      expect(provider.getAvailableSinks(PRESENTATION_URL))
          .toEqual(new mr.SinkList(sinks));
    });
  });

  describe('createRoute Test', function() {
    it('Succeeds with test source', function(done) {
      provider.createRoute(TEST_SOURCE, 'id1').then(route => {
        expect(route.sinkId).toBe('id1');
        expect(mockPmCallbacks.requestKeepAlive)
            .toHaveBeenCalledWith(mr.TestProvider.COMPONENT_ID, true);
        done();
      });
    });

    it('Succeeds with presentation URL', function(done) {
      provider.createRoute(PRESENTATION_URL, 'id1').then(route => {
        expect(route.sinkId).toBe('id1');
        expect(route.isOffscreenPresentation).toBe(true);
        expect(mockPmCallbacks.requestKeepAlive)
            .toHaveBeenCalledWith(mr.TestProvider.COMPONENT_ID, true);
        expect(chrome.tabCapture.captureOffscreenTab).toHaveBeenCalled();
        done();
      });
    });

    it('Returns value from localStorage', function(done) {
      const result = {'passed': 'false', 'errorMessage': 'error'};
      window.localStorage['testdata'] = JSON.stringify({'createRoute': result});
      provider.createRoute(PRESENTATION_URL, 'id1').catch(error => {
        expect(error.message).toBe('error');
        done();
      });
    });
  });

  describe('joinRoute Test', function() {
    it('Success', function(done) {
      const sourceUrn = PRESENTATION_URL;
      const presentationId = 'presentationId';
      const sinkId = 'sinkId1';
      provider.createRoute(sourceUrn, sinkId, presentationId).then(route => {
        expect(route.sinkId).toBe(sinkId);
        provider.joinRoute(sourceUrn, presentationId).then(route => {
          expect(route.sinkId).toBe(sinkId);
          expect(mockPmCallbacks.requestKeepAlive)
              .toHaveBeenCalledWith(mr.TestProvider.COMPONENT_ID, true);
          done();
        });
      });
    });


    it('Rejects when off the record does not match', function(done) {
      const sourceUrn = PRESENTATION_URL;
      const presentationId = 'presentationId';
      const sinkId = 'sinkId1';
      provider.createRoute(sourceUrn, sinkId, presentationId, true)
          .then(route => {
            expect(route.sinkId).toBe(sinkId);
            provider.joinRoute(sourceUrn, presentationId, false)
                .then(done.fail, done);
          });
    });

    it('Read data from localstorage', function(done) {
      const result = {'passed': 'false', 'errorMessage': 'error'};
      window.localStorage['testdata'] = JSON.stringify({'joinRoute': result});
      provider.joinRoute(PRESENTATION_URL, 'presentationId')
          .then(done.fail, error => {
            expect(error.message).toBe('error');
            done();
          });
    });
  });

  describe('getSinkById Test', function() {
    it('Returns values for ids', function() {
      expect(provider.getSinkById('id1'))
          .toEqual(new mr.Sink('id1', 'test-sink-1'));
      expect(provider.getSinkById('empty')).toBeNull();
    });
  });

  describe('terminateRoute Test', function() {
    it('Success', function(done) {
      const sourceUrn = PRESENTATION_URL;
      provider.createRoute(sourceUrn, 'id1').then(route => {
        provider.terminateRoute(route.id).then(_ => {
          expect(mockPmCallbacks.onRouteRemoved)
              .toHaveBeenCalledWith(provider, route);
          expect(mockPmCallbacks.onPresentationConnectionStateChanged)
              .toHaveBeenCalledWith(
                  route.id, mr.PresentationConnectionState.TERMINATED);
          expect(provider.canJoin(sourceUrn, undefined, route)).toBe(false);
          done();
        });
      });
    });
  });

  describe('detachRoute Test', function() {
    it('Success', function(done) {
      const sourceUrn = PRESENTATION_URL;
      provider.createRoute(sourceUrn, 'id1').then(route => {
        provider.detachRoute(route.id);
        expect(mockPmCallbacks.onPresentationConnectionClosed)
            .toHaveBeenCalledWith(
                route.id, mr.PresentationConnectionCloseReason.CLOSED,
                jasmine.any(String));
        expect(provider.canJoin(sourceUrn, undefined, route)).toBe(true);
        done();
      });
    });
  });

  describe('canRoute Test', function() {
    it('Returns values for sources and sinks', function() {
      expect(provider.canRoute(INVALID_SOURCE, 'id1')).toBe(false);
      expect(provider.canRoute(PRESENTATION_URL, 'id1')).toBe(true);
      expect(provider.canRoute(PRESENTATION_URL, 'empty')).toBe(false);
    });

    it('Returns data from localStorage', function() {
      window.localStorage['testdata'] = JSON.stringify({'canRoute': 'false'});
      expect(provider.canRoute(PRESENTATION_URL, 'empty')).toBe(false);
    });
  });

  describe('canJoin Test', function() {
    const expectCanJoin = function(sourceUrn, done) {
      provider.createRoute(sourceUrn, 'id1', 'presentationId').then(route => {
        expect(provider.canJoin(sourceUrn, undefined, route)).toBe(true);
        done();
      });
    };

    it('Returns false for invalid source', function() {
      expect(provider.canJoin(INVALID_SOURCE)).toBe(false);
    });

    it('Returns true for valid source 1', function(done) {
      expectCanJoin(TEST_SOURCE, done);
    });

    it('Returns true for valid source 2', function(done) {
      expectCanJoin(PRESENTATION_URL, done);
    });

    it('Returns true with no route provided', function() {
      expect(provider.canJoin(PRESENTATION_URL)).toBe(true);
    });

    it('Returns value from localStorage', function() {
      window.localStorage['testdata'] = JSON.stringify({'canJoin': 'false'});
      expect(provider.canJoin(PRESENTATION_URL)).toBe(false);
    });
  });
});
