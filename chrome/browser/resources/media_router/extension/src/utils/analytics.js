// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/** @fileoverview API for Analytics events.  */

goog.provide('mr.Analytics');
goog.provide('mr.LongTiming');
goog.provide('mr.MediumTiming');
goog.provide('mr.Timing');

goog.require('mr.Logger');


/**
 * Begins the timing period.
 */
mr.Timing = class {
  /**
   * @param {!string} name
   */
  constructor(name) {
    /** @private {!string} */
    this.name_ = name;

    /** @private {number} */
    this.startTime_ = Date.now();
  }

  /**
   * Gets the full name with the suffix appended if provided.
   * @param {string} name The name of the event or histogram.
   * @param {string=} opt_suffix The optional suffix to add.
   * @return {string} The full name with suffix added.
   * @private
   */
  getFullName_(name, opt_suffix) {
    if (opt_suffix != null) {
      name += '_' + opt_suffix;
    }
    return name;
  }

  /**
   * Sets the name for the timing object.
   * @param {string} name The new name.
   */
  setName(name) {
    this.name_ = name;
  }

  /**
   * Ends the timing period and reports to UMA.
   * @param {string=} opt_suffix An optional suffix.
   */
  end(opt_suffix) {
    const duration = Date.now() - this.startTime_;
    const name = this.getFullName_(this.name_, opt_suffix);
    mr.Timing.recordDuration(name, duration);
  }

  /**
   * Sends a short duration value (up to 10 seconds) for analytics collection.
   * @param {string} name
   * @param {number} duration Duration in milliseconds.
   */
  static recordDuration(name, duration) {
    if (duration < 0) {
      mr.Timing.logger_.warning('Timing analytics event with negative time');
      duration = 0;
    }

    if (duration > mr.Timing.TEN_SECONDS_) {
      duration = mr.Timing.TEN_SECONDS_;
    }

    try {
      chrome.metricsPrivate.recordTime(name, duration);
    } catch (e) {
      mr.Timing.logger_.warning(
          'Failed to record time ' + duration + ' in ' + name);
    }
  }
};


/** @private const */
mr.Timing.logger_ = mr.Logger.getInstance('mr.Timing');


/**
 * Ten seconds in milliseconds.
 * @const {number}
 * @private
 */
mr.Timing.TEN_SECONDS_ = 10 * 1000;


/**
 * Begins a medium timing period (should be measured in seconds).
 */
mr.MediumTiming = class extends mr.Timing {
  /**
   * @param {string} name The histogram name.
   */
  constructor(name) {
    super(name);
  }

  /**
   * @override
   */
  end(opt_suffix) {
    const duration = Date.now() - this.startTime_;
    const name = this.getFullName_(this.name_, opt_suffix);
    mr.MediumTiming.recordDuration(name, duration);
  }

  /**
   * Sends a medium duration value (up to 3 minutes) for analytics collection.
   * @param {string} name
   * @param {number} duration Duration in milliseconds.
   */
  static recordDuration(name, duration) {
    if (duration < 0) {
      mr.MediumTiming.logger_.warning(
          'Timing analytics event with negative time');
      return;
    }

    if (duration < mr.Timing.TEN_SECONDS_) {
      duration = mr.Timing.TEN_SECONDS_;
    }

    if (duration > mr.MediumTiming.THREE_MINUTES_) {
      duration = mr.MediumTiming.THREE_MINUTES_;
    }

    try {
      chrome.metricsPrivate.recordMediumTime(name, duration);
    } catch (e) {
      mr.MediumTiming.logger_.warning(
          'Failed to record time ' + duration + ' in ' + name);
    }
  }
};


/** @private @const */
mr.MediumTiming.logger_ = mr.Logger.getInstance('mr.MediumTiming');


/**
 * Constant of 3 minutes (in milliseconds).
 * @private @const {number}
 **/
mr.MediumTiming.THREE_MINUTES_ = 3 * 60 * 1000;


/**
 * Begins a long timing period (up to 1 hour).
 */
mr.LongTiming = class extends mr.Timing {
  /**
   * @param {string} name The name of the histogram.
   */
  constructor(name) {
    super(name);
  }

  /**
   * @override
   */
  end(opt_suffix) {
    const duration = Date.now() - this.startTime_;
    const name = this.getFullName_(this.name_, opt_suffix);
    mr.LongTiming.recordDuration(name, duration);
  }

  /**
   * Sends a long duration value (up to 1 hour) for analytics collection.
   * @param {string} name
   * @param {number} duration Duration in milliseconds.
   */
  static recordDuration(name, duration) {
    if (duration < 0) {
      mr.LongTiming.logger_.warning(
          'Timing analytics event with negative time');
      return;
    }

    if (duration < mr.MediumTiming.THREE_MINUTES_) {
      duration = mr.MediumTiming.THREE_MINUTES_;
    }

    if (duration > mr.LongTiming.ONE_HOUR_) {
      duration = mr.LongTiming.ONE_HOUR_;
    }

    try {
      chrome.metricsPrivate.recordLongTime(name, duration);
    } catch (e) {
      mr.LongTiming.logger_.warning(
          'Failed to record time ' + duration + ' in ' + name);
    }
  }
};


/** @private @const */
mr.LongTiming.logger_ = mr.Logger.getInstance('mr.LongTiming');


/**
 * Constant of 1 hour (in milliseconds).
 * @private @const {number}
 **/
mr.LongTiming.ONE_HOUR_ = 60 * 60 * 1000;


/** @const {*} */
mr.Analytics = {};


/**
 * @const {mr.Logger}
 * @private
 */
mr.Analytics.logger_ = mr.Logger.getInstance('mr.Analytics');


/**
 * Sends a user action for analytics collection.
 * @param {!string} name
 */
mr.Analytics.recordEvent = function(name) {
  try {
    chrome.metricsPrivate.recordUserAction(name);
  } catch (e) {
    mr.Analytics.logger_.warning('Failed to record event ' + name);
  }
};


/**
 * Send a value for analytics collection.
 * @param {!string} name
 * @param {!number} value
 * @param {!Object<string,number>} values
 */
mr.Analytics.recordEnum = function(name, value, values) {
  let foundKey;
  let size = 0;
  for (let key in values) {
    size++;
    if (values[key] == value) {
      foundKey = key;
    }
  }
  if (!foundKey) {
    mr.Analytics.logger_.error(
        'Unknown analytics value, ' + value + ' for histogram, ' + name,
        Error() /* for stack trace */);
    return;
  }

  const config = {
    'metricName': name,
    'type': 'histogram-linear',
    'min': 1,
    'max': size,
    // Add one for the underflow bucket.
    'buckets': size + 1
  };

  try {
    chrome.metricsPrivate.recordValue(config, value);
  } catch (/** Error */ e) {
    mr.Analytics.logger_.warning(
        'Failed to record enum value ' + foundKey + ' (' + value + ') in ' +
            name,
        e);
  }
};


/**
 * Records a small count (0 to 100) for analytics collection.
 * @param {string} name
 * @param {number} count
 */
mr.Analytics.recordSmallCount = function(name, count) {
  try {
    if (count < 0) {
      throw new Error(`Invalid count for ${name}: ${count}`);
    } else if (count > 100) {
      mr.Analytics.logger_.warning(
          `Small count for ${name} exceeded limits: ${count}`, Error());
    }
    chrome.metricsPrivate.recordSmallCount(name, count);
  } catch (/** Error */ e) {
    mr.Analytics.logger_.warning(
        `Failed to record small count ${name} (${count})`, e);
  }
};
