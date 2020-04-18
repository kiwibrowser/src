// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * @fileoverview An abstract throttling sender. It sends right away if there is
 *  no sending in the past 'interval' time. Otherwise, it waits till 'interval'
 *  is passed.
 *

 */

goog.provide('mr.ThrottlingSender');


/**
 * Note: Strictly speaking, this class should implement PersistentData to store
 * lastSendTime_. But since we never specify an interval large enough for the
 * extension to become suspended in between, it is not needed in practice.
 */
mr.ThrottlingSender = class {
  /**
   * @param {number} interval in milliseconds.
   */
  constructor(interval) {
    /** @private {number} */
    this.interval_ = interval;

    /** @private {?number} */
    this.lastSendTime_ = null;

    /** @private {?number} */
    this.timerId_ = null;
  }

  /**
   * Clears the sender timer and sets it to null.
   * @private
   */
  clearTimer_() {
    if (this.timerId_ != null) {
      clearTimeout(this.timerId_);
      this.timerId_ = null;
    }
  }

  /**
   * Schedule a send.
   * @protected
   */
  scheduleSend() {
    if (this.timerId_ != null) {
      return;
    }
    if (this.lastSendTime_ == null ||
        Date.now() - this.lastSendTime_ >= this.interval_) {
      // Send right away
      this.send_();
    } else {
      // Delay a while
      const delay =
          Math.max(this.lastSendTime_ + this.interval_ - Date.now(), 5);
      this.timerId_ = setTimeout(this.send_.bind(this), delay);
    }
  }

  /**
   * Sends messages immediately.
   */
  sendImmediately() {
    this.send_();
  }

  /**
   * Sends right away and schedule another send if there is more to send.
   * @private
   */
  send_() {
    this.clearTimer_();
    this.doSend();
    this.lastSendTime_ = Date.now();
  }

  /**
   * Sends the message if available.
   * @protected
   */
  doSend() {}
};
