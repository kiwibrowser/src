// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

goog.provide('mr.Logger');

goog.require('mr.Assertions');
goog.require('mr.Config');


/**
 * An object for recording logs.
 */
mr.Logger = class {
  /**
   * @param {string} name
   */
  constructor(name) {
    /**
     * @private @const {string}
     */
    this.name_ = name;
  }

  /**
   * @param {string} name
   * @return {!mr.Logger}
   */
  static getInstance(name) {
    let instance = mr.Logger.instances_.get(name);
    if (!instance) {
      instance = new mr.Logger(name);
      mr.Logger.instances_.set(name, instance);
    }
    return instance;
  }

  /**
   * @param {function(!mr.Logger.Record)} handler
   */
  static addHandler(handler) {
    mr.Logger.handlers_.push(handler);
  }

  /**
   * Logs a pre-built record if its level is high enough.
   * @param {!mr.Logger.Record} record
   */
  static logRecord(record) {
    if (record.level >= mr.Logger.level) {
      mr.Logger.handlers_.forEach(handler => handler(record));
    }
  }

  /**
   * Logs a message at the specified log level with an optional exception.
   *
   * @param {mr.Logger.Level} level
   * @param {mr.Logger.Loggable} message
   * @param {*=} exception An exception to associate with the log message.  This
   *     should normally be an Error instance for best results, but any type is
   *     acceptable.
   */
  log(level, message, exception = undefined) {
    if (level < mr.Logger.level) {
      return;
    }

    // Logging will occur at the current logging level. If the message is a
    // lazy-evaluated one, eval now.
    if (typeof message == 'function') {
      message = message();
    }

    // For non-debug builds, make an effort to programmatically scrub message
    // text that potentially contains personally-identifying information.
    // However, note that this only covers some of the more-obvious forms of
    // PII, and no heuristic can ever hope to provide 100% safety. Also, some of
    // the regular expressions may sometimes match more or less than what was
    // intended.
    mr.Assertions.assert(
        typeof message == 'string', 'Expected message to be a string.');
    if (!mr.Config.isDebugChannel) {
      message = message.replace(mr.Logger.URL_REGEXP_, '[Redacted URL]');
      message = message.replace(
          mr.Logger.DOMAIN_OR_EMAIL_REGEXP_, '[Redacted domain/email]');
      message = message.replace(mr.Logger.SINK_ID_REGEXP_, (match, p1, p2) => {
        return p1 + ':<' + p2.substr(-4) + '>';
      });
    }

    const record = {
      logger: this.name_,
      level: level,
      time: Date.now(),
      message: message,
      exception: exception,
    };
    mr.Logger.handlers_.forEach(handler => handler(record));
  }

  /**
   * @param {mr.Logger.Loggable} message
   * @param {*=} exception
   */
  error(message, exception = undefined) {
    this.log(mr.Logger.Level.SEVERE, message, exception);
  }

  /**
   * @param {mr.Logger.Loggable} message
   * @param {*=} exception
   */
  warning(message, exception = undefined) {
    this.log(mr.Logger.Level.WARNING, message, exception);
  }

  /**
   * @param {mr.Logger.Loggable} message
   * @param {*=} exception
   */
  info(message, exception = undefined) {
    this.log(mr.Logger.Level.INFO, message, exception);
  }

  /**
   * @param {mr.Logger.Loggable} message
   * @param {*=} exception
   */
  fine(message, exception = undefined) {
    this.log(mr.Logger.Level.FINE, message, exception);
  }

  /**
   * @param {mr.Logger.Level} level
   * @return {string}
   */
  static levelToString(level) {
    return mr.Logger.LEVEL_NAMES_[level];
  }

  /**
   * @param {string} levelName
   * @param {mr.Logger.Level} defaultLevel
   * @return {mr.Logger.Level}
   */
  static stringToLevel(levelName, defaultLevel) {
    const index = mr.Logger.LEVEL_NAMES_.indexOf(levelName);
    return index == -1 ? defaultLevel : /** @type {mr.Logger.Level} */ (index);
  }

  /**
   * Converts a numeric log level (as used in the Closure library) into a log
   * level constant.
   * @param {number} levelValue
   * @return {mr.Logger.Level}
   */
  static numberToLevel(levelValue) {
    if (levelValue <= 600) {
      return mr.Logger.Level.FINE;
    } else if (levelValue <= 850) {
      return mr.Logger.Level.INFO;
    } else if (levelValue <= 950) {
      return mr.Logger.Level.WARNING;
    } else {
      return mr.Logger.Level.SEVERE;
    }
  }
};


/**
 * @private @const {!Array<function(mr.Logger.Record)>}
 */
mr.Logger.handlers_ = [];


/**
 * @private @const {!Map<string, !mr.Logger>}
 */
mr.Logger.instances_ = new Map();


/**
 * The available log levels.
 * @enum {number}
 */
mr.Logger.Level = {
  FINE: 0,
  INFO: 1,
  WARNING: 2,
  SEVERE: 3,
};


/**
 * The canonical names of log levels in ascending order of severity.
 * @private const {!Array<string>}
 */
mr.Logger.LEVEL_NAMES_ = ['FINE', 'INFO', 'WARNING', 'SEVERE'];


/**
 * A regular expression that matches a very broad-range of text that looks like
 * it could be a domain name or an e-mail address.
 * @private const {!RegExp}
 */
mr.Logger.DOMAIN_OR_EMAIL_REGEXP_ =
    /(([\w.+-]+@)|((www|m|mail|ftp)[.]))[\w.-]+[.][\w-]{2,4}/gi;


/**
 * A regular expression that matches a very broad-range of text that looks like
 * it could be an URL.
 * @private const {!RegExp}
 */
mr.Logger.URL_REGEXP_ = /(data:|https?:\/\/)\S+/gi;


/**
 * A regular expression that matches a very broad-range of text that looks like
 * it could be a sink ID.
 * @private const {!RegExp}
 */
mr.Logger.SINK_ID_REGEXP_ = /(dial|cast):<([a-zA-Z0-9]+)>/gi;


/**
 * An abstract represenation of a log message.
 *
 * The `time` field should be in the format returned by `Date.now()`.  The
 * `exception` field will typically be an Error instance, but code that handles
 * log records must be prepared to handle any type.
 *
 * @typedef {{
 *   level: mr.Logger.Level,
 *   logger: string,
 *   time: number,
 *   message: string,
 *   exception: *,
 * }}
 */
mr.Logger.Record;


/**
 * @typedef {string|function():string}
 */
mr.Logger.Loggable;


/**
 * @const
 */
mr.Logger.DEFAULT_LEVEL = mr.Logger.Level.INFO;


/**
 * @type {mr.Logger.Level}
 */
mr.Logger.level = mr.Logger.DEFAULT_LEVEL;
