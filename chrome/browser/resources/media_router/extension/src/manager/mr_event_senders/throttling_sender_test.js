// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

goog.setTestOnly();
goog.require('mr.MockClock');
goog.require('mr.ThrottlingSender');

describe('Tests ThrottlingSender', function() {
  let mockClock;
  let sender;
  const interval = 50;
  let numOfMessages;

  beforeEach(function() {
    mockClock = new mr.MockClock(true);
    sender = new mr.ThrottlingSender(interval);
    numOfMessages = 0;
    sender.doSend = jasmine.createSpy('doSend').and.callFake(() => {
      numOfMessages--;
    });
  });

  afterEach(function() {
    mockClock.uninstall();
  });

  it('first msg is sent right away', function() {
    numOfMessages++;
    sender.scheduleSend();
    expect(sender.doSend.calls.count()).toEqual(1);
    expect(numOfMessages).toEqual(0);
    mockClock.tick(interval);
    expect(sender.doSend.calls.count()).toEqual(1);
    expect(numOfMessages).toEqual(0);
  });

  it('messages are throttled', function() {
    numOfMessages++;
    sender.scheduleSend();
    expect(sender.doSend.calls.count()).toEqual(1);
    expect(numOfMessages).toEqual(0);

    numOfMessages++;
    sender.scheduleSend();
    expect(sender.doSend.calls.count()).toEqual(1);
    expect(numOfMessages).toEqual(1);

    mockClock.tick(interval);
    expect(sender.doSend.calls.count()).toEqual(2);
    expect(numOfMessages).toEqual(0);
  });

  it('messages are sent gradually 1', function() {
    numOfMessages++;
    sender.scheduleSend();
    expect(sender.doSend.calls.count()).toEqual(1);
    mockClock.tick(interval / 2);
    expect(sender.doSend.calls.count()).toEqual(1);

    numOfMessages++;
    sender.scheduleSend();
    expect(sender.doSend.calls.count()).toEqual(1);
    mockClock.tick(interval / 2);
    expect(sender.doSend.calls.count()).toEqual(2);
    mockClock.tick(interval);
    expect(sender.doSend.calls.count()).toEqual(2);
  });
});
