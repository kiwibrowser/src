// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

goog.setTestOnly('external_message_listener_test');

goog.require('mr.ExternalMessageListener');
goog.require('mr.InternalMessageType');
goog.require('mr.UnitTestUtils');

describe('Tests mr.ExternalMessageListener', () => {
  let listener;

  beforeEach(() => {
    mr.UnitTestUtils.mockChromeApi();
    listener = new mr.ExternalMessageListener();
  });

  afterEach(() => {
    mr.UnitTestUtils.restoreChromeApi();
  });

  it('rejects sender not in whitelist', () => {
    const sender = {'id': 'invalid'};
    const callback = response => {
      fail('should not have called back');
    };

    expect(listener.validateEvent({}, sender, callback)).toBe(false);
  });

  it('invalid type returns empty and closes channel', () => {
    const sender = {'id': 'njjegkblellcjnakomndbaloifhcoccg'};
    const callback = response => {
      fail('should not have called back');
    };

    expect(listener.validateEvent({}, sender, callback)).toBe(false);
  });

  it('valid start message', () => {
    const sender = {'id': 'njjegkblellcjnakomndbaloifhcoccg'};
    const callback = response => {
      fail('should not have called back');
    };
    const message = {'type': mr.InternalMessageType.START};
    expect(listener.validateEvent(message, sender, callback)).toBe(true);
  });

  it('valid stop message', () => {
    const sender = {'id': 'njjegkblellcjnakomndbaloifhcoccg'};
    const callback = response => {
      fail('should not have called back');
    };
    const message = {'type': mr.InternalMessageType.STOP};
    expect(listener.validateEvent(message, sender, callback)).toBe(true);
  });

  it('valid log subscription message', () => {
    const sender = {'id': 'njjegkblellcjnakomndbaloifhcoccg'};
    const callback = response => {
      fail('should not have called back');
    };
    const message = {'type': mr.InternalMessageType.SUBSCRIBE_LOG_DATA};
    expect(listener.validateEvent(message, sender, callback)).toBe(true);
  });
});
