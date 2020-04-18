// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

goog.setTestOnly();
goog.require('mr.MockClock');
goog.require('mr.PersistentDataManager');
goog.require('mr.RouteMessage');
goog.require('mr.RouteMessageSender');
goog.require('mr.UnitTestUtils');



describe('Tests RouteMessageSender', function() {
  let mockClock;
  let providerManagerCallbacks;
  let callback;
  let sender;
  const routeId1 = 'r1';
  const routeId2 = 'r2';
  const text1 = 'message1';
  const messageSizeThreshold = 100;

  beforeEach(function() {
    mr.UnitTestUtils.mockChromeApi();
    chrome.runtime.onSuspend.addListener = l => {
      onSuspendListener = l;
    };

    mr.PersistentDataManager.clear();
    mockClock = new mr.MockClock(true);
    providerManagerCallbacks =
        jasmine.createSpyObj('pmCallbacks', ['requestKeepAlive']);
    sender = new mr.RouteMessageSender(
        providerManagerCallbacks, messageSizeThreshold);
    callback = jasmine.createSpy('sendCallback');
    sender.init(callback);
  });

  afterEach(function() {
    mr.PersistentDataManager.clear();
    mockClock.uninstall();
    mr.UnitTestUtils.restoreChromeApi();
  });

  it('hasMessageFrom_', function() {
    expect(sender.hasMessageFrom_(routeId1)).toBe(false);
    sender.send(routeId1, text1);
    expect(sender.hasMessageFrom_(routeId1)).toBe(true);
  });

  it('No msg when requested; new msg sent when arrives', function() {
    sender.listenForRouteMessages(routeId1);
    sender.send(routeId1, text1);
    expect(callback).toHaveBeenCalledWith(
        routeId1, [new mr.RouteMessage(routeId1, text1)]);
    expect(sender.hasMessageFrom_(routeId1)).toBe(false);
  });

  it('Has msg when requested', function() {
    sender.send(routeId1, text1);
    expect(callback).not.toHaveBeenCalled();

    sender.listenForRouteMessages(routeId1);
    expect(callback).toHaveBeenCalledWith(
        routeId1, [new mr.RouteMessage(routeId1, text1)]);
    mockClock.tick(mr.RouteMessageSender.SEND_MESSAGE_INTERVAL_MILLIS);
    expect(sender.hasMessageFrom_(routeId1)).toBe(false);
  });

  it('onRouteRemoved removes messages', function() {
    sender.send(routeId1, text1);
    expect(sender.hasMessageFrom_(routeId1)).toBe(true);
    sender.onRouteRemoved(routeId1);
    expect(sender.hasMessageFrom_(routeId1)).toBe(false);

    expect(sender.totalMessageSize_).toEqual(0);
    expect(sender.binaryMessageCount_).toEqual(0);
    expect(sender.shouldKeepAlive_).toBe(false);
  });

  it('stopListeningForRouteMessages does not remove messages', function() {
    sender.send(routeId1, text1);
    expect(sender.hasMessageFrom_(routeId1)).toBe(true);
    sender.stopListeningForRouteMessages(routeId1);
    expect(sender.hasMessageFrom_(routeId1)).toBe(true);
  });

  it('requestKeepAlive due to binary message', function() {
    const binaryArray1 = new Uint8Array(12);
    const binaryArray2 = new Uint8Array(34);
    sender.send(routeId1, binaryArray1);
    expect(providerManagerCallbacks.requestKeepAlive)
        .toHaveBeenCalledWith(sender.getStorageKey(), true);
    providerManagerCallbacks.requestKeepAlive.calls.reset();

    sender.send(routeId2, binaryArray2);
    expect(providerManagerCallbacks.requestKeepAlive).not.toHaveBeenCalled();

    sender.listenForRouteMessages(routeId1);
    mockClock.tick(mr.RouteMessageSender.SEND_MESSAGE_INTERVAL_MILLIS);
    expect(callback).toHaveBeenCalledWith(
        routeId1, [new mr.RouteMessage(routeId1, binaryArray1)]);
    expect(providerManagerCallbacks.requestKeepAlive).not.toHaveBeenCalled();

    sender.listenForRouteMessages(routeId2);
    mockClock.tick(mr.RouteMessageSender.SEND_MESSAGE_INTERVAL_MILLIS);
    expect(callback).toHaveBeenCalledWith(
        routeId2, [new mr.RouteMessage(routeId2, binaryArray2)]);
    expect(providerManagerCallbacks.requestKeepAlive)
        .toHaveBeenCalledWith(sender.getStorageKey(), false);

    expect(sender.totalMessageSize_).toEqual(0);
    expect(sender.binaryMessageCount_).toEqual(0);
    expect(sender.shouldKeepAlive_).toBe(false);
  });

  it('requestKeepAlive due to message size threshold', function() {
    const message1 = 'a'.repeat(messageSizeThreshold / 2);
    const message2 = 'b'.repeat(messageSizeThreshold / 2 + 1);

    sender.send(routeId1, message1);
    expect(providerManagerCallbacks.requestKeepAlive).not.toHaveBeenCalled();

    sender.send(routeId2, message2);
    expect(providerManagerCallbacks.requestKeepAlive)
        .toHaveBeenCalledWith(sender.getStorageKey(), true);
    providerManagerCallbacks.requestKeepAlive.calls.reset();

    sender.listenForRouteMessages(routeId1);
    mockClock.tick(mr.RouteMessageSender.SEND_MESSAGE_INTERVAL_MILLIS);
    expect(callback).toHaveBeenCalledWith(
        routeId1, [new mr.RouteMessage(routeId1, message1)]);
    expect(providerManagerCallbacks.requestKeepAlive)
        .toHaveBeenCalledWith(sender.getStorageKey(), false);

    sender.listenForRouteMessages(routeId2);
    mockClock.tick(mr.RouteMessageSender.SEND_MESSAGE_INTERVAL_MILLIS);
    expect(callback).toHaveBeenCalledWith(
        routeId2, [new mr.RouteMessage(routeId2, message2)]);

    expect(sender.totalMessageSize_).toEqual(0);
    expect(sender.binaryMessageCount_).toEqual(0);
    expect(sender.shouldKeepAlive_).toBe(false);
  });

  it('saves pending messages, then restores and sends them', function() {
    const textMessage = 'this is a text message';
    const textMessage2 = 'this is another text message';

    // Queue up the messages in the RouteMessageSender. They will not be sent
    // because listenForRouteMessages() has not been called yet.
    sender.send(routeId1, textMessage);
    sender.send(routeId1, textMessage2);

    // Persists pending messages.
    mr.PersistentDataManager.suspendForTest();

    // Re-creating a new RouteMessageSender restores the pending messages.
    sender = new mr.RouteMessageSender(
        providerManagerCallbacks, messageSizeThreshold);
    sender.init(callback);

    // Now, call listenForRouteMessages() and the restored pending messages
    // should be processed.
    sender.listenForRouteMessages(routeId1);
    mockClock.tick(mr.RouteMessageSender.SEND_MESSAGE_INTERVAL_MILLIS);
    expect(callback).toHaveBeenCalledWith(routeId1, [
      new mr.RouteMessage(routeId1, textMessage),
      new mr.RouteMessage(routeId1, textMessage2),
    ]);
  });
});
