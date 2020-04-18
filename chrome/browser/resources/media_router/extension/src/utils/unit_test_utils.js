// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**

 * @fileoverview Test utilities for unit tests.
 */
goog.provide('mr.UnitTestUtils');
goog.setTestOnly('mr.UnitTestUtils');

goog.require('mr.Assertions');
goog.require('mr.MockClock');
goog.require('mr.MockPromise');


/**
 * Creates a mock implementation of the RouteControllerCallbacks interface.
 * @return {!Object}
 */
mr.UnitTestUtils.createRouteControllerCallbacksSpyObj = function() {
  return jasmine.createSpyObj('RouteControllerCallbacks', [
    'onRouteControllerInvalidated', 'sendMediaControlRequest',
    'sendVolumeRequest'
  ]);
};


/**
 * Creates a Mojo InterfaceRequest-like object with Jasmine spy.
 * @return {!Object}
 */
mr.UnitTestUtils.createMojoRequestSpyObj = function() {
  return jasmine.createSpyObj('Request', ['close']);
};


/**
 * Creates a Mojo Binding-like object with Jasmine spy.
 * @return {!Object}
 */
mr.UnitTestUtils.createMojoBindingSpyObj = function() {
  return jasmine.createSpyObj(
      'Binding', ['bind', 'close', 'setConnectionErrorHandler']);
};


/**
 * Creates a Mojo MediaStatusObserver-like object with Jasmine spies.
 * @return {!Object}
 */
mr.UnitTestUtils.createMojoMediaStatusObserverSpyObj = function() {
  return {
    ptr: jasmine.createSpyObj(
        'PtrController', ['reset', 'setConnectionErrorHandler']),
    onMediaStatusUpdated: jasmine.createSpy('onMediaStatusUpdated')
  };
};


/**
 * Creates a Jasmine spy object with the same methods of the given constructor
 * type.
 * @param {Function} constructor The object's constructor to be mocked. E.g.
 *   MyClass.prototype.
 * @param {string} mockName The mock's name (will show up in failed test
 *   results).
 * @return {Object}
 */
mr.UnitTestUtils.createMock = function(constructor, mockName) {
  const methodNames = new Set();
  while (constructor) {
    for (let name of Object.getOwnPropertyNames(constructor)) {
      methodNames.add(name);
    }
    constructor = Object.getPrototypeOf(constructor);
  }
  return jasmine.createSpyObj(mockName, [...methodNames]);
};

/**
 * Replaces parts of the window.mojo API with Jasmine spys used by unit tests.
 */
mr.UnitTestUtils.mockMojoApi = function() {
  window.mojo = {
    Binding: {},
    HangoutsMediaRouteController: {},
    HangoutsMediaStatusExtraData: {},
    MediaStatus: {
      PlayState: {PLAYING: 0, PAUSED: 1, BUFFERING: 2},
    },
    MediaController: {},
    RouteControllerType: {kNone: 0, kGeneric: 1, kHangouts: 2},
    TimeDelta: {},
    Origin: {}
  };
  // We return a copy of the object used to construct the MediaStatus.
  // This works because the fields in the object maps directly to the fields
  // in MediaStatus.
  spyOn(window.mojo, 'HangoutsMediaStatusExtraData')
      .and.callFake(obj => Object.assign({}, obj));
  spyOn(window.mojo, 'MediaStatus').and.callFake(obj => Object.assign({}, obj));
  spyOn(window.mojo, 'TimeDelta').and.callFake(obj => Object.assign({}, obj));
  spyOn(window.mojo, 'Origin').and.callFake(obj => Object.assign({}, obj));
};

/**
 * Replaces parts of the chrome API with Jasmine spy objects. After calling this
 * function, call restoreChromeApi() during test teardown to restore the
 * original API.
 */
mr.UnitTestUtils.mockChromeApi = function() {
  mr.UnitTestUtils.originalChromeApi_ = chrome;
  chrome = {
    cast: {
      channel: {
        onError: jasmine.createSpy('chrome.cast.channel.onError spy'),
        onMessage: jasmine.createSpy('chrome.cast.channel.onMessage spy')
      },
      SessionStatus: {
        CONNECTED: 'connected',
        DISCONNECTED: 'disconnected',
        STOPPED: 'stopped'
      },
      VolumeControlType:
          {ATTENUATION: 'attenuation', FIXED: 'fixed', MASTER: 'master'}
    },
    identity: {
      onSignInChanged: {
        addListener:
            jasmine.createSpy('chrome.identity.onSignInChanged.addListener spy')
      }
    },
    runtime: {
      onMessage: {
        addListener:
            jasmine.createSpy('chrome.runtime.onMessage.addListener spy')
      },
      onMessageExternal: {
        addListener: jasmine.createSpy(
            'chrome.runtime.onMessageExternal.addListener spy')
      },
      onStartup: {
        addListener:
            jasmine.createSpy('chrome.runtime.onStartup.addListener spy')
      },
      onSuspend: {
        addListener:
            jasmine.createSpy('chrome.runtime.onSuspend.addListener spy')
      },
      getManifest: () => {
        return {version: '1.0'};
      },
      sendMessage: jasmine.createSpy('chrome.runtime.sendMessage spy'),
    },
    mdns: {onSerivceList: jasmine.createSpy('chrome.mdns.onServiceList spy')},
    metricsPrivate: {
      recordMediumTime:
          jasmine.createSpy('chrome.metricsPrivate.recordMediumTime spy'),
      recordUserAction:
          jasmine.createSpy('chrome.metricsPrivate.recordUserAction spy'),
    },
    networkingPrivate: {
      onNetworksChanged:
          jasmine.createSpy('chrome.networkingPrivate.onNetworksChanged spy')
    },
    gcm: {
      register: jasmine.createSpy('chrome.gcm.register spy'),
      onMessage: {
        addListener: jasmine.createSpy('chrome.gcm.onMessage.addListener spy')
      }
    },
    tabs: {
      get: jasmine.createSpy('chrome.tabs.get spy'),
    },
  };
};

/**
 * Restores the original chrome API after it's been mocked by a mockChromeApi()
 * call.
 */
mr.UnitTestUtils.restoreChromeApi = function() {
  chrome = mr.UnitTestUtils.originalChromeApi_;
};

/**
 * Stores a reference to the original chrome API while it is mocked.
 * @private {?Object}
 */
mr.UnitTestUtils.originalChromeApi_ = null;



/**
 * @private {mr.MockClock}
 */
mr.UnitTestUtils.mockClock_ = null;


/**
 * Installs a mock clock and replaces the native Promise type with goog.Promise.
 *

 *
 * @return {!mr.MockClock}
 */
mr.UnitTestUtils.useMockClockAndPromises = function() {
  mr.Assertions.assert(mr.UnitTestUtils.mockClock_ == null);
  mr.MockPromise.install();
  const mockClock = new mr.MockClock(true);
  mr.UnitTestUtils.mockClock_ = mockClock;
  return mockClock;
};


/**
 * Undoes the effect of calling useMockClockAndPromises().
 */
mr.UnitTestUtils.restoreRealClockAndPromises = function() {
  mr.UnitTestUtils.mockClock_.uninstall();
  mr.UnitTestUtils.mockClock_ = null;
  mr.MockPromise.uninstall();
};


/**
 * Asserts that mock clock and mock promises are in use.
 * @private
 */
mr.UnitTestUtils.assertUsingMockPromises_ = function() {
  mr.Assertions.assert(Promise === mr.MockPromise);
};


/**
 * Waits for the provided promise to resolve.
 * @param {!Promise<T>} promise The promise to resolve.
 * @template T
 */
mr.UnitTestUtils.awaitPromiseResolved = function(promise) {
  mr.UnitTestUtils.assertUsingMockPromises_();
  let resolved = false;
  promise.then(result => {
    resolved = true;
  });
  mr.MockPromise.callPendingHandlers();
  expect(resolved).toBe(true);
};


/**
 * Expects the provided promise to resolve to a value that makes the given
 * predicate true.
 * @param {!Promise<T>} promise The promise to resolve.
 * @param {function(T):boolean} isCorrect A function called with the resolved
 *     value of the promise; expected to return true.
 * @template T
 */
mr.UnitTestUtils.expectPromiseResult = function(promise, isCorrect) {
  mr.UnitTestUtils.assertUsingMockPromises_();
  let resolved = false;
  let actual;
  promise.then(result => {
    resolved = true;
    actual = result;
  });
  mr.MockPromise.callPendingHandlers();
  expect(resolved).toBe(true);
  expect(isCorrect(actual)).toBe(true);
};


/**
 * Expects the provided promise to resolve to the given result.
 * @param {!Promise} promise
 * @param {*} expectedResult
 */
mr.UnitTestUtils.expectPromiseResultToEqual = function(
    promise, expectedResult) {
  mr.UnitTestUtils.assertUsingMockPromises_();
  let resolved = false;
  let actual;
  promise.then(result => {
    resolved = true;
    actual = result;
  });
  mr.MockPromise.callPendingHandlers();
  expect(resolved).toBe(true);
  expect(actual).toEqual(expectedResult);
};


/**
 * Expects the provided promise to resolve to the given result.
 * @param {!Promise} promise
 * @param {Error} expectedError
 */
mr.UnitTestUtils.expectPromiseRejection = function(promise, expectedError) {
  mr.UnitTestUtils.assertUsingMockPromises_();
  let actual;
  promise.catch(error => {
    actual = error;
  });
  mr.MockPromise.callPendingHandlers();
  expect(actual).toEqual(expectedError);
};
