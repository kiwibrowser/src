// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

goog.setTestOnly('internal_message_listener_test');

goog.require('mr.InternalMessageListener');

describe('Tests mr.InternalMessageListener', () => {
  let listener;

  const failCallback = response => {
    fail('should not have called back');
  };
  const validEvent = {'type': mr.InternalMessageType.RETRIEVE_LOG_DATA};
  const validSender = {
    'id': 'foo',
    'url': 'chrome-extension://foo/feedback.html'
  };
  beforeEach(() => {
    chrome.runtime = {id: 'foo'};
    listener = new mr.InternalMessageListener();
  });

  it('rejects invalid extension id', () => {
    const sender = {
      'id': 'invalid',
      'url': 'chrome-extension://invalid/feedback.html'
    };

    const result = listener.validateEvent(validEvent, sender, failCallback);
    expect(result).toBe(false);
  });

  it('rejects invalid extension url', () => {
    const sender = {'id': 'foo', 'url': 'chrome-extension://foo/invalid.html'};

    const result = listener.validateEvent(validEvent, sender, failCallback);
    expect(result).toBe(false);
  });

  it('rejects invalid message type', () => {
    const result = listener.validateEvent(
        {'type': mr.InternalMessageType.START}, validSender, failCallback);
    expect(result).toBe(false);
  });

  it('passes validation', () => {
    const result = listener.validateEvent(validEvent, validSender, () => {});
    expect(result).toBe(true);
  });
});
