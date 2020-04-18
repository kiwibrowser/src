// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * @fileoverview Login UI header bar implementation.
 */

cr.define('login', function() {
  /**
   * Enum for user actions taken from lock screen header while a lock screen
   * app is in background.
   * @enum {string}
   */
  var LOCK_SCREEN_APPS_UNLOCK_ACTION = {
    SIGN_OUT: 'LOCK_SCREEN_APPS_UNLOCK_ACTION.SIGN_OUT',
    SHUTDOWN: 'LOCK_SCREEN_APPS_UNLOCK_ACTION.SHUTDOWN'
  };

  /**
   * Creates a header bar element.
   *
   * @constructor
   * @extends {HTMLDivElement}
   */
  var HeaderBar = cr.ui.define('div');

  HeaderBar.prototype = {
    __proto__: HTMLDivElement.prototype,

    // Whether guest button should be shown when header bar is in normal mode.
    showGuest_: false,

    // Whether the reboot button should be shown the when header bar is in
    // normal mode.
    showReboot_: false,

    // Whether the shutdown button should be shown when the header bar is in
    // normal mode.
    showShutdown_: true,

    // Whether the create supervised user button should be shown when the header
    // bar is in normal mode. It will be shown in "More settings" menu.
    showCreateSupervised_: false,

    // Current UI state of the sign-in screen.
    signinUIState_: SIGNIN_UI_STATE.HIDDEN,

    // Current lock screen apps activity state. This value affects visibility of
    // tray buttons visible in the header bar - when lock screeen apps state is
    // FOREGROUND, only the unlock button should be shown (when clicked, the
    // button issues a request to move lock screen apps to background, in the
    // state where account picker is visible).
    lockScreenAppsState_: LOCK_SCREEN_APPS_STATE.NONE,

    // Whether to show kiosk apps menu.
    hasApps_: false,

    /** @override */
    decorate: function() {
      document.addEventListener('click', this.handleClick_.bind(this));
      $('shutdown-header-bar-item')
          .addEventListener('click', this.handleShutdownClick_.bind(this));
      $('shutdown-button')
          .addEventListener('click', this.handleShutdownClick_.bind(this));
      $('restart-header-bar-item')
          .addEventListener('click', this.handleShutdownClick_.bind(this));
      $('restart-button')
          .addEventListener('click', this.handleShutdownClick_.bind(this));
      $('add-user-button').addEventListener('click', this.handleAddUserClick_);
      $('more-settings-button')
          .addEventListener('click', this.handleMoreSettingsClick_.bind(this));
      $('guest-user-header-bar-item')
          .addEventListener('click', this.handleGuestClick_);
      $('guest-user-button').addEventListener('click', this.handleGuestClick_);
      $('sign-out-user-button')
          .addEventListener('click', this.handleSignoutClick_.bind(this));
      $('cancel-multiple-sign-in-button')
          .addEventListener('click', this.handleCancelMultipleSignInClick_);
      $('unlock-user-button')
          .addEventListener('click', this.handleUnlockUserClick_);
      this.addSupervisedUserMenu.addEventListener(
          'click', this.handleAddSupervisedUserClick_.bind(this));
      this.addSupervisedUserMenu.addEventListener(
          'keydown', this.handleAddSupervisedUserKeyDown_.bind(this));
      if (Oobe.getInstance().displayType == DISPLAY_TYPE.LOGIN ||
          Oobe.getInstance().displayType == DISPLAY_TYPE.OOBE) {
        if (Oobe.getInstance().newKioskUI)
          chrome.send('initializeKioskApps');
        else
          login.AppsMenuButton.decorate($('show-apps-button'));
      }
      this.updateUI_();
    },

    /**
     * Tab index value for all button elements.
     *
     * @type {number}
     */
    set buttonsTabIndex(tabIndex) {
      var buttons = this.getElementsByTagName('button');
      for (var i = 0, button; button = buttons[i]; ++i) {
        button.tabIndex = tabIndex;
      }
    },

    /**
     * Disables the header bar and all of its elements.
     *
     * @type {boolean}
     */
    set disabled(value) {
      var buttons = this.getElementsByTagName('button');
      for (var i = 0, button; button = buttons[i]; ++i)
        if (!button.classList.contains('button-restricted'))
          button.disabled = value;
    },

    get getMoreSettingsMenu() {
      return $('more-settings-header-bar-item');
    },

    get addSupervisedUserMenu() {
      return this.querySelector('.add-supervised-user-menu');
    },

    /**
     * Whether action box button is in active state.
     * @type {boolean}
     */
    get isMoreSettingsActive() {
      return this.getMoreSettingsMenu.classList.contains('active');
    },
    set isMoreSettingsActive(active) {
      if (active == this.isMoreSettingsActive)
        return;
      this.getMoreSettingsMenu.classList.toggle('active', active);
      $('more-settings-button').tabIndex = active ? -1 : 4;
    },

    /**
     * Add user button click handler.
     *
     * @private
     */
    handleAddUserClick_: function(e) {
      Oobe.showSigninUI();
      // Prevent further propagation of click event. Otherwise, the click event
      // handler of document object will set wallpaper to user's wallpaper when
      // there is only one existing user. See http://crbug.com/166477
      e.stopPropagation();
    },

    handleMoreSettingsClick_: function(e) {
      this.isMoreSettingsActive = !this.isMoreSettingsActive;
      this.addSupervisedUserMenu.focus();
      e.stopPropagation();
    },

    handleClick_: function(e) {
      this.isMoreSettingsActive = false;
    },

    /**
     * Cancel add user button click handler.
     *
     * @private
     */
    handleCancelAddUserClick_: function(e) {
      // Let screen handle cancel itself if that is capable of doing so.
      if (Oobe.getInstance().currentScreen &&
          Oobe.getInstance().currentScreen.cancel) {
        Oobe.getInstance().currentScreen.cancel();
        return;
      }

      Oobe.showUserPods();
    },

    /**
     * Guest button click handler.
     *
     * @private
     */
    handleGuestClick_: function(e) {
      Oobe.disableSigninUI();
      chrome.send('launchIncognito');
      e.stopPropagation();
    },

    /**
     * Sign out button click handler.
     *
     * @private
     */
    handleSignoutClick_: function(e) {
      this.disabled = true;

      chrome.send('signOutUser');
      e.stopPropagation();
    },

    /**
     * Shutdown button click handler.
     *
     * @private
     */
    handleShutdownClick_: function(e) {
      chrome.send('shutdownSystem');
      e.stopPropagation();
    },

    /**
     * Cancel user adding button handler.
     *
     * @private
     */
    handleCancelMultipleSignInClick_: function(e) {
      chrome.send('cancelUserAdding');
      e.stopPropagation();
    },

    /**
     * Add supervised user button handler.
     *
     * @private
     */
    handleAddSupervisedUserClick_: function(e) {
      chrome.send('showSupervisedUserCreationScreen');
      e.preventDefault();
    },

    /**
     * Add supervised user key handler, ESC closes menu.
     *
     * @private
     */
    handleAddSupervisedUserKeyDown_: function(e) {
      if (e.key == 'Escape' && this.isMoreSettingsActive) {
        this.isMoreSettingsActive = false;
        $('more-settings-button').focus();
      }
    },

    /**
     * Unlock user button handler. Sends a request to Chrome to show user pods
     * in foreground.
     *
     * @private
     */
    handleUnlockUserClick_: function(e) {
      chrome.send('closeLockScreenApp');
      e.preventDefault();
    },

    /**
     * If true then "Browse as Guest" button is shown.
     *
     * @type {boolean}
     */
    set showGuestButton(value) {
      this.showGuest_ = value;
      this.updateUI_();
    },

    set showCreateSupervisedButton(value) {
      this.showCreateSupervised_ = value;
      this.updateUI_();
    },

    /**
     * If true the "Restart" button is shown.
     *
     * @type {boolean}
     */
    set showRebootButton(value) {
      this.showReboot_ = value;
      this.updateUI_();
    },

    /**
     * If true the "Shutdown" button is shown.
     *
     * @type {boolean}
     */
    set showShutdownButton(value) {
      this.showShutdown_ = value;
      this.updateUI_();
    },

    /**
     * Current header bar UI / sign in state.
     *
     * @type {number} state Current state of the sign-in screen (see
     *       SIGNIN_UI_STATE).
     */
    get signinUIState() {
      return this.signinUIState_;
    },
    set signinUIState(state) {
      this.signinUIState_ = state;
      this.updateUI_();
    },

    /**
     * Current activity state of lock screen app windows.
     *
     * @type {LOCK_SCREEN_APPS_STATE}
     */
    set lockScreenAppsState(state) {
      this.lockScreenAppsState_ = state;
      this.updateUI_();
    },

    /**
     * Update whether there are kiosk apps.
     *
     * @type {boolean}
     */
    set hasApps(value) {
      this.hasApps_ = value;
      this.updateUI_();
    },

    /**
     * Updates visibility state of action buttons.
     *
     * @private
     */
    updateUI_: function() {
      var gaiaIsActive = (this.signinUIState_ == SIGNIN_UI_STATE.GAIA_SIGNIN);
      var enrollmentIsActive =
          (this.signinUIState_ == SIGNIN_UI_STATE.ENROLLMENT);
      var accountPickerIsActive =
          (this.signinUIState_ == SIGNIN_UI_STATE.ACCOUNT_PICKER);
      var supervisedUserCreationDialogIsActive =
          (this.signinUIState_ ==
           SIGNIN_UI_STATE.SUPERVISED_USER_CREATION_FLOW);
      var wrongHWIDWarningIsActive =
          (this.signinUIState_ == SIGNIN_UI_STATE.WRONG_HWID_WARNING);
      var isSamlPasswordConfirm =
          (this.signinUIState_ == SIGNIN_UI_STATE.SAML_PASSWORD_CONFIRM);
      var isPasswordChangedUI =
          (this.signinUIState_ == SIGNIN_UI_STATE.PASSWORD_CHANGED);
      var isMultiProfilesUI =
          (Oobe.getInstance().displayType == DISPLAY_TYPE.USER_ADDING);
      var isLockScreen = (Oobe.getInstance().displayType == DISPLAY_TYPE.LOCK);
      var errorScreenIsActive = (this.signinUIState_ == SIGNIN_UI_STATE.ERROR);

      $('add-user-button').hidden = !accountPickerIsActive ||
          isMultiProfilesUI || isLockScreen || errorScreenIsActive;
      $('more-settings-header-bar-item').hidden = !this.showCreateSupervised_ ||
          gaiaIsActive || isLockScreen || errorScreenIsActive ||
          supervisedUserCreationDialogIsActive;
      $('guest-user-header-bar-item').hidden = !this.showGuest_ ||
          isLockScreen || supervisedUserCreationDialogIsActive ||
          wrongHWIDWarningIsActive || isSamlPasswordConfirm ||
          isMultiProfilesUI || (gaiaIsActive && $('gaia-signin').closable) ||
          (enrollmentIsActive && !$('oauth-enrollment').isAtTheBeginning()) ||
          (gaiaIsActive && !$('gaia-signin').isAtTheBeginning());
      $('restart-header-bar-item').hidden = !this.showReboot_ ||
          this.lockScreenAppsState_ == LOCK_SCREEN_APPS_STATE.FOREGROUND;
      $('shutdown-header-bar-item').hidden = !this.showShutdown_ ||
          this.lockScreenAppsState_ == LOCK_SCREEN_APPS_STATE.FOREGROUND;
      $('sign-out-user-item').hidden = !isLockScreen ||
          this.lockScreenAppsState_ == LOCK_SCREEN_APPS_STATE.FOREGROUND;
      $('unlock-user-header-bar-item').hidden = !isLockScreen ||
          this.lockScreenAppsState_ != LOCK_SCREEN_APPS_STATE.FOREGROUND;

      $('add-user-header-bar-item').hidden = $('add-user-button').hidden;
      $('apps-header-bar-item').hidden =
          !this.hasApps_ || (!gaiaIsActive && !accountPickerIsActive);
      $('cancel-multiple-sign-in-item').hidden = !isMultiProfilesUI;

      if (!Oobe.getInstance().newKioskUI) {
        if (!$('apps-header-bar-item').hidden)
          $('show-apps-button').didShow();
      }

      // Lock screen apps are generally shown maximized - update the header
      // bar background opacity so the wallpaper is not visible behind it (
      // since it won't be visible in the rest of UI).
      this.classList.toggle(
          'full-header-background',
          this.lockScreenAppsState_ == LOCK_SCREEN_APPS_STATE.FOREGROUND ||
              this.lockScreenAppsState_ == LOCK_SCREEN_APPS_STATE.BACKGROUND);
    },

    /**
     * Animates Header bar to hide from the screen.
     *
     * @param {function()} callback will be called once animation is finished.
     */
    animateOut: function(callback) {
      var launcher = this;
      launcher.addEventListener('transitionend', function f(e) {
        launcher.removeEventListener('transitionend', f);
        callback();
      });
      // Guard timer for 2 seconds + 200 ms + epsilon.
      ensureTransitionEndEvent(launcher, 2250);

      this.classList.remove('login-header-bar-animate-slow');
      this.classList.add('login-header-bar-animate-fast');
      this.classList.add('login-header-bar-hidden');
    },

    /**
     * Animates Header bar to appear on the screen.
     *
     * @param {boolean} fast Whether the animation should complete quickly or
     *     slowly.
     * @param {function()} callback will be called once animation is finished.
     */
    animateIn: function(fast, callback) {
      if (callback) {
        var launcher = this;
        launcher.addEventListener('transitionend', function f(e) {
          launcher.removeEventListener('transitionend', f);
          callback();
        });
        // Guard timer for 2 seconds + 200 ms + epsilon.
        ensureTransitionEndEvent(launcher, 2250);
      }

      if (fast) {
        this.classList.remove('login-header-bar-animate-slow');
        this.classList.add('login-header-bar-animate-fast');
      } else {
        this.classList.remove('login-header-bar-animate-fast');
        this.classList.add('login-header-bar-animate-slow');
      }

      this.classList.remove('login-header-bar-hidden');
    },
  };

  /**
   * Convenience wrapper of animateOut.
   */
  HeaderBar.animateOut = function(callback) {
    $('login-header-bar').animateOut(callback);
  };

  /**
   * Convenience wrapper of animateIn.
   */
  HeaderBar.animateIn = function(fast, callback) {
    $('login-header-bar').animateIn(fast, callback);
  };

  return {HeaderBar: HeaderBar};
});
