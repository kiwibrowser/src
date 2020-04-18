// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * @fileoverview ChromeVox keyboard handler.
 */

goog.provide('BackgroundKeyboardHandler');

goog.require('ChromeVoxState');
goog.require('EventSourceState');
goog.require('Output');
goog.require('cvox.ChromeVoxKbHandler');
goog.require('cvox.ChromeVoxPrefs');

/** @constructor */
BackgroundKeyboardHandler = function() {
  /** @type {number} @private */
  this.passThroughKeyUpCount_ = 0;

  /** @type {Set} @private */
  this.eatenKeyDowns_ = new Set();

  document.addEventListener('keydown', this.onKeyDown.bind(this), false);
  document.addEventListener('keyup', this.onKeyUp.bind(this), false);

  chrome.accessibilityPrivate.setKeyboardListener(
      true, cvox.ChromeVox.isStickyPrefOn);
  window['prefs'].switchToKeyMap('keymap_next');
};

BackgroundKeyboardHandler.prototype = {
  /**
   * Handles key down events.
   * @param {Event} evt The key down event to process.
   * @return {boolean} True if the default action should be performed.
   */
  onKeyDown: function(evt) {
    EventSourceState.set(EventSourceType.STANDARD_KEYBOARD);
    evt.stickyMode = cvox.ChromeVox.isStickyModeOn() && cvox.ChromeVox.isActive;
    if (cvox.ChromeVox.passThroughMode)
      return false;

    Output.forceModeForNextSpeechUtterance(cvox.QueueMode.FLUSH);
    if (!cvox.ChromeVoxKbHandler.basicKeyDownActionsListener(evt)) {
      evt.preventDefault();
      evt.stopPropagation();
      this.eatenKeyDowns_.add(evt.keyCode);
    }
    return false;
  },

  /**
   * Handles key up events.
   * @param {Event} evt The key down event to process.
   * @return {boolean} True if the default action should be performed.
   */
  onKeyUp: function(evt) {
    // Reset pass through mode once a keyup (not involving the pass through key)
    // is seen. The pass through command involves three keys.
    if (cvox.ChromeVox.passThroughMode) {
      if (this.passThroughKeyUpCount_ >= 3) {
        cvox.ChromeVox.passThroughMode = false;
        this.passThroughKeyUpCount_ = 0;
      } else {
        this.passThroughKeyUpCount_++;
      }
    }

    if (this.eatenKeyDowns_.has(evt.keyCode)) {
      evt.preventDefault();
      evt.stopPropagation();
      this.eatenKeyDowns_.delete(evt.keyCode);
    }

    return false;
  }
};

/**
 * @param {number} keyCode
 * @param {chrome.accessibilityPrivate.SyntheticKeyboardModifiers=} modifiers
 * @return {boolean}
 */
BackgroundKeyboardHandler.sendKeyPress = function(keyCode, modifiers) {
  var key = {
    type: chrome.accessibilityPrivate.SyntheticKeyboardEventType.KEYDOWN,
    keyCode: keyCode,
    modifiers: modifiers
  };
  chrome.accessibilityPrivate.sendSyntheticKeyEvent(key);
  key['type'] = chrome.accessibilityPrivate.SyntheticKeyboardEventType.KEYUP;
  chrome.accessibilityPrivate.sendSyntheticKeyEvent(key);
  return true;
};
