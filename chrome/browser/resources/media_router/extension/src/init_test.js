// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

goog.setTestOnly('init_test');

goog.require('mr.Init');
goog.require('mr.MockClock');
goog.require('mr.Module');
goog.require('mr.UnitTestUtils');



describe('Tests init', function() {
  let mockClock;
  let savedCallback;

  beforeEach(function() {
    mr.Module.clearForTest();
    mockClock = new mr.MockClock(true);
    mr.UnitTestUtils.mockChromeApi();

    for (let listener of mr.Init.getAllListeners_()) {
      spyOn(listener, 'addOnStartup').and.callThrough();
    }

    spyOn(mr.ExtensionSelector, 'shouldStart')
        .and.returnValue(Promise.resolve());
    spyOn(mr.MediaRouterService, 'getInstance').and.returnValue({
      'mrService': jasmine.createSpyObj(
          'mrService', ['setHandlers', 'onRouteMessagesReceived']),
      'mrInstanceId': 'mrInstanceId'
    });

    spyOn(mr.PersistentDataManager, 'initialize');
    spyOn(mr.PersistentDataManager, 'register');
    spyOn(mr.Init, 'getProviders_').and.returnValue([]);

    savedCallback = null;
    chrome.runtime.onSuspend.addListener.and.callFake(callback => {
      savedCallback = callback;
    });
  });

  afterEach(function() {
    mockClock.uninstall();
    mr.UnitTestUtils.restoreChromeApi();
  });

  it('records first wake duration after Chrome reload', function(done) {
    spyOn(mr.PersistentDataManager, 'isChromeReloaded').and.returnValue(true);
    mr.Init.init().then(() => {
      expect(chrome.runtime.onSuspend.addListener).toHaveBeenCalled();
      expect(savedCallback).not.toBeNull();
      mockClock.tick(12345);
      savedCallback();
      expect(chrome.metricsPrivate.recordMediumTime)
          .toHaveBeenCalledWith(mr.Init.FIRST_WAKE_DURATION, 12345);
      done();
    });
  });

  it('records wake duration after Chrome reload', function(done) {
    spyOn(mr.PersistentDataManager, 'isChromeReloaded').and.returnValue(false);
    mr.Init.init().then(() => {
      expect(chrome.runtime.onSuspend.addListener).toHaveBeenCalled();
      expect(savedCallback).not.toBeNull();
      mockClock.tick(54321);
      savedCallback();
      expect(chrome.metricsPrivate.recordMediumTime)
          .toHaveBeenCalledWith(mr.Init.WAKE_DURATION, 54321);
      done();
    });
  });

  it('Registers event listeners on bootstrap', function(done) {
    mr.Init.init().then(done, done.fail);
    for (let listener of mr.Init.getAllListeners_()) {
      expect(listener.addOnStartup).toHaveBeenCalled();
    }
  });
});
