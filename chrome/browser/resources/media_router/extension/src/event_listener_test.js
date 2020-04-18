// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

goog.setTestOnly('event_listener_test');

goog.require('mr.EventAnalytics');
goog.require('mr.EventListener');
goog.require('mr.Module');
goog.require('mr.PersistentDataManager');
goog.require('mr.PromiseResolver');
goog.require('mr.UnitTestUtils');


describe('Tests event listeners', function() {
  let mockEvent;

  beforeEach(function() {
    mr.UnitTestUtils.mockChromeApi();
    mockEvent = jasmine.createSpyObj(
        'mockEvent', ['addListener', 'hasListener', 'removeListener']);
  });

  afterEach(function() {
    mr.Module.clearForTest();
    mr.PersistentDataManager.clear();
    mr.UnitTestUtils.restoreChromeApi();
  });

  it('EventListener addListener no listener args', function() {
    const listener = new mr.EventListener(
        mr.EventAnalytics.Event.DIAL_ON_ERROR, 'MockEventListener',
        'SomeModule', mockEvent);
    listener.addListener();
    expect(mockEvent.addListener).toHaveBeenCalled();
    expect(listener.isRegistered()).toBe(true);

    listener.removeListener();
    expect(mockEvent.removeListener).toHaveBeenCalled();
    expect(listener.isRegistered()).toBe(false);
  });

  it('EventListener addListener with listener args', function() {
    const listenerArgs = ['foo', 1];
    const listener = new mr.EventListener(
        mr.EventAnalytics.Event.DIAL_ON_ERROR, 'MockEventListener',
        'SomeModule', mockEvent, ...listenerArgs);
    listener.addListener();
    expect(mockEvent.addListener)
        .toHaveBeenCalledWith(jasmine.any(Function), ...listenerArgs);
    expect(listener.isRegistered()).toBe(true);

    listener.removeListener();
    expect(mockEvent.removeListener).toHaveBeenCalled();
    expect(listener.isRegistered()).toBe(false);
  });

  it('EventListener addOnStartup no prior register', function() {
    const listener = new mr.EventListener(
        mr.EventAnalytics.Event.DIAL_ON_ERROR, 'MockEventListener',
        'SomeModule', mockEvent);
    listener.addOnStartup();
    expect(mockEvent.addListener).not.toHaveBeenCalled();
  });

  it('EventListener addOnStartup registered before', function() {
    const listener = new mr.EventListener(
        mr.EventAnalytics.Event.DIAL_ON_ERROR, 'MockEventListener',
        'SomeModule', mockEvent);
    listener.addOnStartup();
    expect(mockEvent.addListener.calls.count()).toBe(0);
    listener.addListener();
    expect(mockEvent.addListener.calls.count()).toBe(1);

    mr.PersistentDataManager.suspendForTest();

    const listener2 = new mr.EventListener(
        mr.EventAnalytics.Event.DIAL_ON_ERROR, 'MockEventListener',
        'SomeModule', mockEvent);
    // Registers with mr.PersistentDataManager again. It should see that it was
    // previously registered before suspend, and re-adds the listener
    // auatomatically.
    listener2.addOnStartup();
    expect(mockEvent.addListener.calls.count()).toBe(2);
  });

  it('EventListener rejected invalid event', function() {
    let savedListener = null;
    const listener = new mr.EventListener(
        mr.EventAnalytics.Event.DIAL_ON_ERROR, 'MockEventListener',
        'SomeModule', mockEvent);
    mockEvent.addListener.and.callFake(listener => {
      savedListener = listener;
    });
    listener.addListener();
    expect(mockEvent.addListener).toHaveBeenCalled();
    expect(savedListener).not.toBeNull();
    spyOn(listener, 'validateEvent').and.returnValue(false);

    // Module won't be loaded since event did not pass validation.
    spyOn(mr.Module, 'load');
    let returnedValue = savedListener('foo', 1);
    expect(returnedValue).toBe(false);
    expect(mr.Module.load.calls.count()).toBe(0);
  });

  it('EventListener dispatch event', function(done) {
    spyOn(mr.EventAnalytics, 'recordEvent');
    let savedListener = null;
    const listener = new mr.EventListener(
        mr.EventAnalytics.Event.DIAL_ON_ERROR, 'MockEventListener',
        'SomeModule', mockEvent);
    spyOn(listener, 'deferredReturnValue').and.returnValue(123);
    mockEvent.addListener.and.callFake(listener => {
      savedListener = listener;
    });
    listener.addListener();
    expect(mockEvent.addListener).toHaveBeenCalled();
    expect(savedListener).not.toBeNull();

    // Module not ready yet; events are queued up, and deferred value is
    // returned synchronously.
    let resolver = new mr.PromiseResolver();
    spyOn(mr.Module, 'load').and.returnValue(resolver.promise);
    let returnedValue = savedListener('foo', 1);
    expect(returnedValue).toBe(123);
    returnedValue = savedListener('bar', 2);
    expect(returnedValue).toBe(123);
    expect(mr.Module.load.calls.count()).toBe(2);

    expect(mr.EventAnalytics.recordEvent)
        .toHaveBeenCalledWith(mr.EventAnalytics.Event.DIAL_ON_ERROR);
    expect(mr.EventAnalytics.recordEvent.calls.count()).toEqual(2);

    const module = jasmine.createSpyObj('mockModule', ['handleEvent']);
    module.handleEvent.and.callFake((e, arg1, arg2) => {
      if (arg1 == 'bar') {
        expect(module.handleEvent).toHaveBeenCalledWith(mockEvent, 'foo', 1);
        expect(module.handleEvent).toHaveBeenCalledWith(mockEvent, 'bar', 2);
        done();
      }
    });

    // Fake loading the module.
    resolver.resolve(module);
  });
});
