// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

goog.module('mr.LoggerTest');
goog.setTestOnly('mr.LoggerTest');

const Config = goog.require('mr.Config');
const Logger = goog.require('mr.Logger');

describe('Test mr.Logger', function() {
  let originalLevel;
  let logger;

  beforeEach(() => {
    originalLevel = Logger.level;
    logger = new Logger('test');
  });

  afterEach(() => {
    Logger.level = originalLevel;
    Logger.handlers_ = [];
  });

  it('logs string messages only at INFO and above', () => {
    Logger.level = Logger.Level.WARNING;
    const loggedMessages = [];
    Logger.addHandler(record => {
      expect(record.level).not.toBeLessThan(Logger.Level.WARNING);
      expect(record.logger).toEqual('test');
      expect(typeof record.time).toBe('number');
      expect(typeof record.message).toBe('string');
      loggedMessages.push(record.message);
    });

    logger.fine('Should not log this message.');
    logger.info('Should not log this message either.');
    logger.warning('Should log this warning message.');
    logger.error('Should log this error message.');

    expect(loggedMessages).toEqual([
      'Should log this warning message.', 'Should log this error message.'
    ]);
  });

  it('logs lazy-evaluated messages', () => {
    Logger.level = Logger.Level.FINE;
    const loggedMessages = [];
    Logger.addHandler(record => {
      expect(record.level).not.toBeLessThan(Logger.Level.FINE);
      expect(record.logger).toEqual('test');
      expect(typeof record.time).toBe('number');
      expect(typeof record.message).toBe('string');
      loggedMessages.push(record.message);
    });

    logger.fine(() => 'Should log this fine message.');
    logger.info(() => 'Should log this info message.');
    logger.warning(() => 'Should log this warning message.');
    logger.error(() => 'Should log this error message.');

    expect(loggedMessages).toEqual([
      'Should log this fine message.', 'Should log this info message.',
      'Should log this warning message.', 'Should log this error message.'
    ]);
  });

  describe('Personally-identifying info scrubbbing tests', () => {
    let loggedMessages;
    let isDebugChannelDefault = Config.isDebugChannel;

    beforeEach(() => {
      Config.isDebugChannel = false;
      loggedMessages = [];
      Logger.level = Logger.Level.FINE;
      Logger.addHandler(record => {
        expect(typeof record.message).toBe('string');
        loggedMessages.push(record.message);
      });
    });

    afterEach(() => {
      Config.isDebugChannel = isDebugChannelDefault;
    });

    it('does not scrub non-PII from messages', () => {
      // Things that shouldn't be scrubbed.
      logger.info('');
      logger.info('42');
      logger.info(
          'Found sink with id: ac6982d68e687faf6ebf8cc (Chromecast Ultra)');
      logger.info('The event occurred at 20:21:22 on 29 Mar 2017.');

      expect(loggedMessages).toEqual([
        '', '42',
        'Found sink with id: ac6982d68e687faf6ebf8cc (Chromecast Ultra)',
        'The event occurred at 20:21:22 on 29 Mar 2017.'
      ]);
    });

    it('scrubs domains', () => {
      // Things that look like domain names.
      logger.info('Visiting www.google.com...');
      logger.info('Tab favicon domain is: ftp.myfilez.net');
      // The following example shows the RegExp currently used does not match
      // against all possible domains perfectly.
      logger.info(
          'mail.personaldata.security.biz mapped to ' +
          'personaldata.security.biz.');

      expect(loggedMessages).toEqual([
        'Visiting [Redacted domain/email]...',
        'Tab favicon domain is: [Redacted domain/email]',
        '[Redacted domain/email] mapped to personaldata.security.biz.'
      ]);
    });

    it('scrubs email addresses', () => {
      // Things that look like e-mail addresses.
      logger.info('Reply to nobody@love-spam.net, and see what happens.');
      logger.info(
          'This CL was written by somebody@developers.chromium.org, ' +
          'or was it somebody@hooli.com?');

      expect(loggedMessages).toEqual([
        'Reply to [Redacted domain/email], and see what happens.',
        'This CL was written by [Redacted domain/email], or was it ' +
            '[Redacted domain/email]?'
      ]);
    });

    it('scrubs URLs', () => {
      // Things that look like URLs.
      logger.info(
          'Downloading from http://www.pictures.com/gifs/' +
          'kittens%20falling%20of%20furniture.png...');
      logger.info('Page navigation detected: https://youtube.com/profile');
      logger.info(
          'Relpacing content with: ' +
          'data:text/plain;base64,SGVsbG8sIFdvcmxkIQ%3D%3D');

      expect(loggedMessages).toEqual([
        'Downloading from [Redacted URL]',
        'Page navigation detected: [Redacted URL]',
        'Relpacing content with: [Redacted URL]'
      ]);
    });

    it('scrubs sink IDs', () => {
      logger.info(
          'Sink has pending connection' +
          ' dial:<05f5e10100641000bc6f90f1aaa0bd90>');
      logger.info(
          'Adding new session: cast:<de51d94921f15f8af6dbf65592bb3610>, ' +
          '5d85e5da-b773-4382-ba06-43c2a6dc6ba6');
      logger.info('Connecting to (id 1) rf72niQ3FPe8VpTz_tIEGNSkfGUo.');
      logger.info('');

      expect(loggedMessages).toEqual([
        'Sink has pending connection dial:<bd90>',
        'Adding new session: cast:<3610>, 5d85e5da-b773-4382-ba06-43c2a6dc6ba6',
        'Connecting to (id 1) rf72niQ3FPe8VpTz_tIEGNSkfGUo.', ''
      ]);
    });
  });
});
