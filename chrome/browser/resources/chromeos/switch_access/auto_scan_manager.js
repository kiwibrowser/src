// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * Class to handle changes to auto-scan.
 *
 * @constructor
 * @param {SwitchAccessInterface} switchAccess
 */
function AutoScanManager(switchAccess) {
  /**
   * SwitchAccess reference.
   *
   * @private {SwitchAccessInterface}
   */
  this.switchAccess_ = switchAccess;

  /**
   * Auto-scan interval ID.
   *
   * @private {number|undefined}
   */
  this.intervalID_;

  /**
   * Length of auto-scan interval in milliseconds.
   *
   * @private {number}
   */
  this.scanTime_ = switchAccess.getNumberPref('autoScanTime');

  let enabled = switchAccess.getBooleanPref('enableAutoScan');
  if (enabled)
    this.start_();
}

AutoScanManager.prototype = {
  /**
   * Stop the window from moving to the next node at a fixed interval.
   *
   * @private
   */
  stop_: function() {
    window.clearInterval(this.intervalID_);
    this.intervalID_ = undefined;
  },

  /**
   * Set the window to move to the next node at an interval in milliseconds
   * equal to this.scanTime_.
   *
   * @private
   */
  start_: function() {
    this.intervalID_ = window.setInterval(
        this.switchAccess_.moveToNode.bind(this.switchAccess_, true),
        this.scanTime_);
  },

  /**
   * Return true if auto-scan is currently running. Otherwise return false.
   *
   * @return {boolean}
   */
  isRunning: function() {
    return this.intervalID_ !== undefined;
  },

  /**
   * Restart auto-scan under the current settings if it is currently running.
   */
  restartIfRunning: function() {
    if (this.isRunning()) {
      this.stop_();
      this.start_();
    }
  },

  /**
   * Stop auto-scan if it is currently running. Then, if |enabled| is true,
   * turn on auto-scan. Otherwise leave it off.
   *
   * @param {boolean} enabled
   */
  setEnabled: function(enabled) {
    if (this.isRunning())
      this.stop_();
    if (enabled)
      this.start_();
  },

  /**
   * Update this.scanTime_ to |scanTime|. Then, if auto-scan is currently
   * running, restart it.
   *
   * @param {number} scanTime Auto-scan interval time in milliseconds.
   */
  setScanTime: function(scanTime) {
    this.scanTime_ = scanTime;
    this.restartIfRunning();
  },
};
