// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * @fileoverview Polymer element for displaying material design HID detection
 * screen.
 */

(function() {
/** @const {number} */ var PINCODE_LENGTH = 6;

Polymer({
  is: 'oobe-hid-detection-md',

  properties: {
    /** "Continue" button is disabled until HID devices are paired. */
    continueButtonDisabled: {
      type: Boolean,
      value: true,
    },

    /** This is the displayed text for keyboard "Pairing" state. */
    keyboardPairingLabel: String,

    /** This is the displayed text for keyboard "Paired" state. */
    keyboardPairedLabel: String,

    /**
     * Current state in mouse pairing process.
     * @private
     */
    mouseState_: String,

    /**
     * Current state in keyboard pairing process.
     * @private
     */
    keyboardState_: String,

    /**
     * Controls visibility of keyboard pincode.
     * @private
     */
    keyboardPincodeVisible_: Boolean,

    /**
     * Reference to OOBE screen object.
     * @type {!OobeTypes.Screen}
     */
    screen: Object,
  },

  /**
   * Displayed keyboard pincode.
   */
  keyboardPincode_: String,

  /**
   * Helper function to update keyboard/mouse state.
   * @param {string} state Existing connection state (one of
   *   screen.CONNECTION).
   * @param {string} newState New connection state (one of screen.CONNECTION).
   * @private
   */
  calculateState_: function(state, newState) {
    if (newState === undefined)
      return state;

    if (newState == this.screen.CONNECTION.UPDATE)
      return state;

    return newState;
  },

  /**
   * Helper function to calculate visibility of 'connected' icons.
   * @param {string} state Connection state (one of screen.CONNECTION).
   * @private
   */
  tickIsVisible_: function(state) {
    return (state == this.screen.CONNECTION.USB) ||
        (state == this.screen.CONNECTION.CONNECTED) ||
        (state == this.screen.CONNECTION.PAIRED);
  },

  /**
   * Helper function to update keyboard/mouse state.
   * Returns true if strings are not equal. False otherwize.
   * @param {string} string1
   * @param {string} string2
   * @private
   */
  notEq_: function(string1, string2) {
    return string1 != string2;
  },

  /**
   * Sets current state in mouse pairing process.
   * @param {string} state Connection state (one of screen.CONNECTION).
   */
  setMouseState: function(state) {
    this.mouseState_ = this.calculateState_(this.mouseState_, state);
  },

  /**
   * Updates visibility of keyboard pincode.
   * @param {string} state Connection state (one of screen.CONNECTION).
   * @private
   */
  updateKeyboardPincodeVisible_: function(state) {
    this.keyboardPincodeVisible_ = this.keyboardPincode_ &&
        (this.keyboardState_ == this.screen.CONNECTION.PAIRING);
  },

  /**
   * Sets current state in keyboard pairing process.
   * @param {string} state Connection state (one of screen.CONNECTION).
   */
  setKeyboardState: function(state) {
    this.keyboardState_ = this.calculateState_(this.keyboardState_, state);
    this.updateKeyboardPincodeVisible_();
  },

  /**
   * Sets displayed keyboard pin.
   * @param {string} pincode Pincode.
   * @param {number} entered Number of digits already entered.
   * @param {boolean} expected
   * @param {string} label Connection state displayed description.
   */
  setPincodeState: function(pincode, entered, expected, label) {
    this.keyboardPincode_ = pincode;
    if (!pincode) {
      this.updateKeyboardPincodeVisible_();
      return;
    }

    if (pincode.length != PINCODE_LENGTH)
      console.error('Wrong pincode length');

    // Pincode keys plus Enter key.
    for (let i = 0; i < (PINCODE_LENGTH + 1); i++) {
      var pincodeSymbol = this.$['hid-keyboard-pincode-sym-' + (i + 1)];
      pincodeSymbol.classList.toggle('key-typed', i < entered && expected);
      pincodeSymbol.classList.toggle('key-untyped', i > entered && expected);
      pincodeSymbol.classList.toggle('key-next', i == entered && expected);
      if (i < PINCODE_LENGTH)
        pincodeSymbol.textContent = pincode[i] ? pincode[i] : '';
    }

    var wasVisible = this.keyboardPincodeVisible_;
    this.updateKeyboardPincodeVisible_();
    if (this.keyboardPincodeVisible_ && !wasVisible) {
      announceAccessibleMessage(
          label + ' ' + pincode + ' ' +
          loadTimeData.getString('hidDetectionBTEnterKey'));
    }
  },

  /**
   * This is 'on-tap' event handler for 'Continue' button.
   */
  onHIDContinueTap_: function(event) {
    chrome.send('HIDDetectionOnContinue');
    event.stopPropagation();
  },
});
})();
