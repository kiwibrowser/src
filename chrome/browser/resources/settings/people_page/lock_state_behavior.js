// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * @fileoverview
 * Contains utilities that help identify the current way that the lock screen
 * will be displayed.
 */

/** @enum {string} */
const LockScreenUnlockType = {
  VALUE_PENDING: 'value_pending',
  PASSWORD: 'password',
  PIN_PASSWORD: 'pin+password'
};

/**
 * Determining if the device supports PIN sign-in takes time, as it may require
 * a cryptohome call. This means incorrect strings may be shown for a brief
 * period, and updating them causes UI flicker.
 *
 * Cache the value since the behavior is instantiated multiple times. Caching
 * is safe because PIN login support depends only on hardware capabilities. The
 * value does not change after discovered.
 *
 * @type {boolean|undefined}
 */
let cachedHasPinLogin = undefined;

/** @polymerBehavior */
const LockStateBehaviorImpl = {
  properties: {
    /**
     * The currently selected unlock type.
     * @type {!LockScreenUnlockType}
     */
    selectedUnlockType:
        {type: String, notify: true, value: LockScreenUnlockType.VALUE_PENDING},

    /**
     * True/false if there is a PIN set; undefined if the computation is still
     * pending. This is a separate value from selectedUnlockType because the UI
     * can change the selectedUnlockType before setting up a PIN.
     * @type {boolean|undefined}
     */
    hasPin: {type: Boolean, notify: true},

    /**
     * True if the PIN backend supports signin. undefined iff the value is still
     * resolving.
     * @type {boolean|undefined}
     */
    hasPinLogin: {type: Boolean, notify: true},

    /**
     * Interface for chrome.quickUnlockPrivate calls. May be overridden by
     * tests.
     * @type {QuickUnlockPrivate}
     */
    quickUnlockPrivate: {type: Object, value: chrome.quickUnlockPrivate},
  },

  /** @override */
  attached: function() {
    this.boundOnActiveModesChanged_ = this.updateUnlockType.bind(this);
    this.quickUnlockPrivate.onActiveModesChanged.addListener(
        this.boundOnActiveModesChanged_);

    // See comment on |cachedHasPinLogin| declaration.
    if (cachedHasPinLogin === undefined) {
      this.addWebUIListener(
          'pin-login-available-changed',
          this.handlePinLoginAvailableChanged_.bind(this));
      chrome.send('RequestPinLoginState');
    } else {
      this.hasPinLogin = cachedHasPinLogin;
    }

    this.updateUnlockType();
  },

  /** @override */
  detached: function() {
    this.quickUnlockPrivate.onActiveModesChanged.removeListener(
        this.boundOnActiveModesChanged_);
  },

  /**
   * Updates the selected unlock type radio group. This function will get called
   * after preferences are initialized, after the quick unlock mode has been
   * changed, and after the lockscreen preference has changed.
   */
  updateUnlockType: function() {
    this.quickUnlockPrivate.getActiveModes(modes => {
      if (modes.includes(chrome.quickUnlockPrivate.QuickUnlockMode.PIN)) {
        this.hasPin = true;
        this.selectedUnlockType = LockScreenUnlockType.PIN_PASSWORD;
      } else {
        this.hasPin = false;
        this.selectedUnlockType = LockScreenUnlockType.PASSWORD;
      }
    });
  },

  /** Sets the lock screen enabled state. */
  setLockScreenEnabled(authToken, enabled) {
    this.quickUnlockPrivate.setLockScreenEnabled(authToken, enabled);
  },

  /**
   * Handler for when the pin login available state has been updated.
   * @private
   */
  handlePinLoginAvailableChanged_: function(isAvailable) {
    this.hasPinLogin = isAvailable;
    cachedHasPinLogin = this.hasPinLogin;
  },
};

/** @polymerBehavior */
const LockStateBehavior =
    [I18nBehavior, WebUIListenerBehavior, LockStateBehaviorImpl];