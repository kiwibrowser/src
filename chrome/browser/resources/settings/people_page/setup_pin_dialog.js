// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * @fileoverview
 * 'settings-setup-pin-dialog' is the settings page for choosing a PIN.
 *
 * Example:
 * * <settings-setup-pin-dialog set-modes="[[quickUnlockSetModes]]">
 * </settings-setup-pin-dialog>
 */

(function() {
'use strict';

/**
 * Keep in sync with the string keys provided by settings.
 * @enum {string}
 */
const MessageType = {
  TOO_SHORT: 'configurePinTooShort',
  TOO_LONG: 'configurePinTooLong',
  TOO_WEAK: 'configurePinWeakPin',
  MISMATCH: 'configurePinMismatched'
};

/** @enum {string} */
const ProblemType = {
  WARNING: 'warning',
  ERROR: 'error'
};

Polymer({
  is: 'settings-setup-pin-dialog',

  behaviors: [I18nBehavior],

  properties: {
    /**
     * Reflects property set in password_prompt_dialog.js.
     * @type {?Object}
     */
    setModes: {
      type: Object,
      notify: true,
    },

    /**
     * The current PIN keyboard value.
     * @private
     */
    pinKeyboardValue_: String,

    /**
     * Stores the initial PIN value so it can be confirmed.
     * @private
     */
    initialPin_: String,

    /**
     * The actual problem message to display.
     * @private
     */
    problemMessage_: String,

    /**
     * The type of problem class to show (warning or error).
     */
    problemClass_: String,

    /**
     * Should the step-specific submit button be displayed?
     * @private
     */
    enableSubmit_: Boolean,

    /**
     * writeUma_ is a function that handles writing uma stats. It may be
     * overridden for tests.
     *
     * @type {Function}
     * @private
     */
    writeUma_: {
      type: Object,
      value: function() {
        return settings.recordLockScreenProgress;
      }
    },

    /**
     * The current step/subpage we are on.
     * @private
     */
    isConfirmStep_: {type: Boolean, value: false},

    /**
     * Interface for chrome.quickUnlockPrivate calls. May be overriden by tests.
     * @private
     */
    quickUnlockPrivate_: {type: Object, value: chrome.quickUnlockPrivate},

    /**
     * |pinHasPassedMinimumLength_| tracks whether a user has passed the minimum
     * length threshold at least once, and all subsequent PIN too short messages
     * will be displayed as errors. They will be displayed as warnings prior to
     * this.
     * @private
     */
    pinHasPassedMinimumLength_: {type: Boolean, value: false},
  },

  /** @override */
  attached: function() {
    this.resetState_();
    this.$.dialog.showModal();
    this.$.pinKeyboard.focus();

    // Show the pin is too short error when first displaying the PIN dialog.
    this.problemClass_ = ProblemType.WARNING;
    this.quickUnlockPrivate_.getCredentialRequirements(
        chrome.quickUnlockPrivate.QuickUnlockMode.PIN,
        this.processPinRequirements_.bind(this, MessageType.TOO_SHORT));
  },

  close: function() {
    if (this.$.dialog.open)
      this.$.dialog.close();

    this.resetState_();
  },

  /**
   * Resets the element to the initial state.
   * @private
   */
  resetState_: function() {
    this.initialPin_ = '';
    this.pinKeyboardValue_ = '';
    this.enableSubmit_ = false;
    this.isConfirmStep_ = false;
    this.hideProblem_();
    this.onPinChange_();
  },

  /** @private */
  onCancelTap_: function() {
    this.resetState_();
    this.$.dialog.close();
  },

  /**
   * Returns true if the PIN is ready to be changed to a new value.
   * @private
   * @return {boolean}
   */
  canSubmit_: function() {
    return this.initialPin_ == this.pinKeyboardValue_;
  },

  /**
   * Handles writing the appropriate message to |problemMessage_|.
   * @private
   * @param {string} messageId
   * @param {chrome.quickUnlockPrivate.CredentialRequirements} requirements
   *     The requirements received from getCredentialRequirements.
   */
  processPinRequirements_: function(messageId, requirements) {
    let additionalInformation = '';
    switch (messageId) {
      case MessageType.TOO_SHORT:
        additionalInformation = requirements.minLength.toString();
        break;
      case MessageType.TOO_LONG:
        additionalInformation = requirements.maxLength.toString();
        break;
      case MessageType.TOO_WEAK:
      case MessageType.MISMATCH:
        break;
      default:
        assertNotReached();
        break;
    }
    this.problemMessage_ = this.i18n(messageId, additionalInformation);
  },

  /**
   * Notify the user about a problem.
   * @private
   * @param {string} messageId
   * @param {string} problemClass
   */
  showProblem_: function(messageId, problemClass) {
    this.quickUnlockPrivate_.getCredentialRequirements(
        chrome.quickUnlockPrivate.QuickUnlockMode.PIN,
        this.processPinRequirements_.bind(this, messageId));
    this.problemClass_ = problemClass;
    this.updateStyles();
    this.enableSubmit_ =
        problemClass != ProblemType.ERROR && messageId != MessageType.TOO_SHORT;
  },

  /** @private */
  hideProblem_: function() {
    this.problemMessage_ = '';
    this.problemClass_ = '';
  },

  /**
   * Processes the message received from the quick unlock api and hides/shows
   * the problem based on the message.
   * @private
   * @param {chrome.quickUnlockPrivate.CredentialCheck} message The message
   *     received from checkCredential.
   */
  processPinProblems_: function(message) {
    if (!message.errors.length && !message.warnings.length) {
      this.hideProblem_();
      this.enableSubmit_ = true;
      this.pinHasPassedMinimumLength_ = true;
      return;
    }

    if (!message.errors.length ||
        message.errors[0] !=
            chrome.quickUnlockPrivate.CredentialProblem.TOO_SHORT) {
      this.pinHasPassedMinimumLength_ = true;
    }

    if (message.warnings.length) {
      assert(
          message.warnings[0] ==
          chrome.quickUnlockPrivate.CredentialProblem.TOO_WEAK);
      this.showProblem_(MessageType.TOO_WEAK, ProblemType.WARNING);
    }

    if (message.errors.length) {
      switch (message.errors[0]) {
        case chrome.quickUnlockPrivate.CredentialProblem.TOO_SHORT:
          this.showProblem_(
              MessageType.TOO_SHORT,
              this.pinHasPassedMinimumLength_ ? ProblemType.ERROR :
                                                ProblemType.WARNING);
          break;
        case chrome.quickUnlockPrivate.CredentialProblem.TOO_LONG:
          this.showProblem_(MessageType.TOO_LONG, ProblemType.ERROR);
          break;
        case chrome.quickUnlockPrivate.CredentialProblem.TOO_WEAK:
          this.showProblem_(MessageType.TOO_WEAK, ProblemType.ERROR);
          break;
        default:
          assertNotReached();
          break;
      }
    }
  },

  /** @private */
  onPinChange_: function() {
    if (!this.isConfirmStep_) {
      if (this.pinKeyboardValue_) {
        this.quickUnlockPrivate_.checkCredential(
            chrome.quickUnlockPrivate.QuickUnlockMode.PIN,
            this.pinKeyboardValue_, this.processPinProblems_.bind(this));
      }
      return;
    }

    this.hideProblem_();
    this.enableSubmit_ = this.pinKeyboardValue_.length > 0;
  },

  /** @private */
  onPinSubmit_: function() {
    if (!this.isConfirmStep_) {
      this.initialPin_ = this.pinKeyboardValue_;
      this.pinKeyboardValue_ = '';
      this.isConfirmStep_ = true;
      this.onPinChange_();
      this.$.pinKeyboard.focus();
      this.writeUma_(LockScreenProgress.ENTER_PIN);
      return;
    }
    // onPinSubmit_ gets called if the user hits enter on the PIN keyboard.
    // The PIN is not guaranteed to be valid in that case.
    if (!this.canSubmit_()) {
      this.showProblem_(MessageType.MISMATCH, ProblemType.ERROR);
      this.enableSubmit_ = false;
      // Focus the PIN keyboard and highlight the entire PIN.
      this.$.pinKeyboard.focus(0, this.pinKeyboardValue_.length + 1);
      return;
    }

    function onSetModesCompleted(didSet) {
      if (!didSet) {
        console.error('Failed to update pin');
        return;
      }

      this.resetState_();
      if (this.$.dialog.open)
        this.$.dialog.close();
    }

    assert(this.setModes);
    this.setModes.call(
        null, [chrome.quickUnlockPrivate.QuickUnlockMode.PIN],
        [this.pinKeyboardValue_], onSetModesCompleted.bind(this));
    this.writeUma_(LockScreenProgress.CONFIRM_PIN);
  },

  /**
   * @private
   * @param {string} problemMessage
   * @param {string} problemClass
   * @return {boolean}
   */
  hasError_: function(problemMessage, problemClass) {
    return !!problemMessage && problemClass == ProblemType.ERROR;
  },

  /**
   * @private
   * @param {boolean} isConfirmStep
   * @return {string}
   */
  getTitleMessage_: function(isConfirmStep) {
    return this.i18n(
        isConfirmStep ? 'configurePinConfirmPinTitle' :
                        'configurePinChoosePinTitle');
  },

  /**
   * @private
   * @param {boolean} isConfirmStep
   * @return {string}
   */
  getContinueMessage_: function(isConfirmStep) {
    return this.i18n(isConfirmStep ? 'confirm' : 'configurePinContinueButton');
  },
});

})();
