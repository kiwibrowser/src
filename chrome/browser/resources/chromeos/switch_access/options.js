// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * Alias for document.getElementById.
 *
 * @param  {string} id
 * @return {Element}
 */
let $ = function(id) {
  // eslint-disable-next-line no-restricted-properties
  return document.getElementById(id);
};

/**
 * Class to manage the options page.
 *
 * @constructor
 */
function SwitchAccessOptions() {
  let background = chrome.extension.getBackgroundPage();

  /**
   * SwitchAccess reference.
   *
   * @private {SwitchAccessInterface}
   */
  this.switchAccess_ = background.switchAccess;

  this.init_();
  document.addEventListener('change', this.handleInputChange_.bind(this));
  background.document.addEventListener(
      'prefsUpdate', this.handlePrefsUpdate_.bind(this));
}

SwitchAccessOptions.prototype = {
  /**
   * Initialize the options page by setting all elements representing a user
   * preference to show the correct value.
   *
   * @private
   */
  init_: function() {
    $('enableAutoScan').checked =
        this.switchAccess_.getBooleanPref('enableAutoScan');
    $('autoScanTime').value =
        this.switchAccess_.getNumberPref('autoScanTime') / 1000;

    for (let command of this.switchAccess_.getCommands()) {
      $(command).value =
          String.fromCharCode(this.switchAccess_.getNumberPref(command));
    }
  },

  /**
   * Handle a change by the user to an element representing a user preference.
   *
   * @param {!Event} event
   * @private
   */
  handleInputChange_: function(event) {
    let input = event.target;
    switch (input.id) {
      case 'enableAutoScan':
        this.switchAccess_.setPref(input.id, input.checked);
        break;
      case 'autoScanTime':
        let oldVal = this.switchAccess_.getNumberPref(input.id);
        let val = Number(input.value) * 1000;
        let min = Number(input.min) * 1000;
        if (this.isValidScanTimeInput_(val, oldVal, min)) {
          input.value = Number(input.value);
          this.switchAccess_.setPref(input.id, val);
        } else {
          input.value = oldVal;
        }
        break;
      default:
        if (this.switchAccess_.getCommands().includes(input.id)) {
          let keyCode = input.value.toUpperCase().charCodeAt(0);
          if (this.isValidKeyCode_(keyCode)) {
            input.value = input.value.toUpperCase();
            this.switchAccess_.setPref(input.id, keyCode);
          } else {
            let oldKeyCode = this.switchAccess_.getNumberPref(input.id);
            input.value = String.fromCharCode(oldKeyCode);
          }
        }
    }
  },

  /**
   * Return true if |keyCode| is a letter or number, and if it is not already
   * being used.
   *
   * @param {number} keyCode
   * @return {boolean}
   */
  isValidKeyCode_: function(keyCode) {
    return ((keyCode >= '0'.charCodeAt(0) && keyCode <= '9'.charCodeAt(0)) ||
            (keyCode >= 'A'.charCodeAt(0) && keyCode <= 'Z'.charCodeAt(0))) &&
        !this.switchAccess_.keyCodeIsUsed(keyCode);
  },

  /**
   * Return true if the input is a valid autoScanTime input. Otherwise, return
   * false.
   *
   * @param {number} value
   * @param {number} oldValue
   * @param {number} min
   * @return {boolean}
   */
  isValidScanTimeInput_: function(value, oldValue, min) {
    return (value !== oldValue) && (value >= min);
  },

  /**
   * Handle a change in user preferences.
   *
   * @param {!Event} event
   * @private
   */
  handlePrefsUpdate_: function(event) {
    let updatedPrefs = event.detail;
    for (let key of Object.keys(updatedPrefs)) {
      switch (key) {
        case 'enableAutoScan':
          $(key).checked = updatedPrefs[key];
          break;
        case 'autoScanTime':
          $(key).value = updatedPrefs[key] / 1000;
          break;
        default:
          if (this.switchAccess_.getCommands().includes(key))
            $(key).value = String.fromCharCode(updatedPrefs[key]);
      }
    }
  }
};

document.addEventListener('DOMContentLoaded', function() {
  new SwitchAccessOptions();
});
