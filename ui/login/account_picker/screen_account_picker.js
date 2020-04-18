// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * @fileoverview Account picker screen implementation.
 */

login.createScreen('AccountPickerScreen', 'account-picker', function() {
  /**
   * Maximum number of offline login failures before online login.
   * @type {number}
   * @const
   */
  var MAX_LOGIN_ATTEMPTS_IN_POD = 3;

  /**
   * Distance between error bubble and user POD.
   * @type {number}
   * @const
   */
   var BUBBLE_POD_OFFSET = 4;

   return {
     EXTERNAL_API: [
       'loadUsers',
       'runAppForTesting',
       'setApps',
       'setShouldShowApps',
       'showAppError',
       'updateUserImage',
       'setCapsLockState',
       'forceLockedUserPodFocus',
       'removeUser',
       'showBannerMessage',
       'showUserPodCustomIcon',
       'hideUserPodCustomIcon',
       'setUserPodFingerprintIcon',
       'removeUserPodFingerprintIcon',
       'setPinEnabledForUser',
       'setAuthType',
       'setTabletModeState',
       'setPublicSessionDisplayName',
       'setPublicSessionLocales',
       'setPublicSessionKeyboardLayouts',
       'setLockScreenAppsState',
     ],

     preferredWidth_: 0,
     preferredHeight_: 0,

     // Whether this screen is shown for the first time.
     firstShown_: true,

     // Whether this screen is currently being shown.
     showing_: false,

     // Last reported lock screen app activity state.
     lockScreenAppsState_: LOCK_SCREEN_APPS_STATE.NONE,

     /** @override */
     decorate: function() {
       login.PodRow.decorate($('pod-row'));
       this.ownerDocument.addEventListener(
           'click', this.handleOwnerDocClick_.bind(this));
     },

     /** @override */
     getPreferredSize: function() {
       return {width: this.preferredWidth_, height: this.preferredHeight_};
     },

     /** @override */
     onWindowResize: function() {
       $('pod-row').onWindowResize();

       // Reposition the error bubble, if it is showing. Since we are just
       // moving the bubble, the number of login attempts tried doesn't matter.
       var errorBubble = $('bubble');
       if (errorBubble && !errorBubble.hidden)
         this.showErrorBubble(0, undefined /* Reuses the existing message. */);
     },

     /**
      * Sets preferred size for account picker screen.
      */
     setPreferredSize: function(width, height) {
       this.preferredWidth_ = width;
       this.preferredHeight_ = height;
     },

     /**
      * When the account picker is being used to lock the screen, pressing the
      * exit accelerator key will sign out the active user as it would when
      * they are signed in.
      */
     exit: function() {
       // Check and disable the sign out button so that we can never have two
       // sign out requests generated in a row.
       if ($('pod-row').lockedPod && !$('sign-out-user-button').disabled) {
         $('sign-out-user-button').disabled = true;
         chrome.send('signOutUser');
       }
     },

     /* Cancel user adding if ESC was pressed.
      */
     cancel: function() {
       if (Oobe.getInstance().displayType == DISPLAY_TYPE.USER_ADDING)
         chrome.send('cancelUserAdding');
     },

     /**
      * Event handler that is invoked just after the frame is shown.
      * @param {string} data Screen init payload.
      */
     onAfterShow: function(data) {
       $('pod-row').handleAfterShow();
     },

     /**
      * Event handler that is invoked just before the frame is shown.
      * @param {string} data Screen init payload.
      */
     onBeforeShow: function(data) {
       this.showing_ = true;
       chrome.send('loginUIStateChanged', ['account-picker', true]);
       $('login-header-bar').signinUIState = SIGNIN_UI_STATE.ACCOUNT_PICKER;
       // Header bar should be always visible on Account Picker screen.
       Oobe.getInstance().headerHidden = false;
       chrome.send('hideCaptivePortal');
       var podRow = $('pod-row');
       podRow.handleBeforeShow();

       // In case of the preselected pod onShow will be called once pod
       // receives focus.
       if (!podRow.preselectedPod)
         this.onShow();
     },

     /**
      * Event handler invoked when the page is shown and ready.
      */
     onShow: function() {
       if (!this.showing_) {
         // This method may be called asynchronously when the pod row finishes
         // initializing. However, at that point, the screen may have been
         // hidden again already. If that happens, ignore the onShow() call.
         return;
       }
       chrome.send('getTabletModeState');
       if (!this.firstShown_)
         return;
       this.firstShown_ = false;

       // Ensure that login is actually visible.
       window.requestAnimationFrame(function() {
         chrome.send('accountPickerReady');
         chrome.send('loginVisible', ['account-picker']);
       });
     },

     /**
      * Event handler that is invoked just before the frame is hidden.
      */
     onBeforeHide: function() {
       $('pod-row').clearFocusedPod();
       this.showing_ = false;
       chrome.send('loginUIStateChanged', ['account-picker', false]);
       $('login-header-bar').signinUIState = SIGNIN_UI_STATE.HIDDEN;
       $('pod-row').handleHide();
     },

     /**
      * Shows sign-in error bubble.
      * @param {number} loginAttempts Number of login attemps tried.
      * @param {HTMLElement} content Content to show in bubble.
      */
     showErrorBubble: function(loginAttempts, error) {
       var activatedPod = $('pod-row').activatedPod;
       if (!activatedPod) {
         $('bubble').showContentForElement(
             $('pod-row'), cr.ui.Bubble.Attachment.RIGHT, error);
         return;
       }
       // Show web authentication if this is not a supervised user.
       if (loginAttempts > MAX_LOGIN_ATTEMPTS_IN_POD &&
           !activatedPod.user.supervisedUser) {
         chrome.send(
             'maxIncorrectPasswordAttempts', [activatedPod.user.emailAddress]);
         activatedPod.showSigninUI();
       } else {
         if (loginAttempts == 1) {
           chrome.send(
               'firstIncorrectPasswordAttempt',
               [activatedPod.user.emailAddress]);
         }
         // Update the pod row display if incorrect password.
         $('pod-row').setFocusedPodErrorDisplay(true);

         /** @const */ var BUBBLE_OFFSET = 25;
         // -8 = 4(BUBBLE_POD_OFFSET) - 2(bubble margin)
         //      - 10(internal bubble adjustment)
         var bubblePositioningPadding = -8;

         var bubbleAnchor;
         var attachment;
         if (activatedPod.pinContainer &&
             activatedPod.pinContainer.style.visibility == 'visible') {
           // Anchor the bubble to the input field.
           bubbleAnchor =
               (activatedPod.getElementsByClassName('auth-container'))[0];
           if (!bubbleAnchor) {
             console.error('auth-container not found!');
             bubbleAnchor = activatedPod.mainInput;
           }
           attachment = cr.ui.Bubble.Attachment.RIGHT;
         } else {
           // Anchor the bubble to the pod instead of the input.
           bubbleAnchor = activatedPod;
           attachment = cr.ui.Bubble.Attachment.BOTTOM;
         }

         var bubble = $('bubble');

         // Cannot use cr.ui.LoginUITools.get* on bubble until it is attached to
         // the element. getMaxHeight/Width rely on the correct up/left element
         // side positioning that doesn't happen until bubble is attached.
         var maxHeight = cr.ui.LoginUITools.getMaxHeightBeforeShelfOverlapping(
                             bubbleAnchor) -
             bubbleAnchor.offsetHeight - BUBBLE_POD_OFFSET;
         var maxWidth = cr.ui.LoginUITools.getMaxWidthToFit(bubbleAnchor) -
             bubbleAnchor.offsetWidth - BUBBLE_POD_OFFSET;

         // Change bubble visibility temporary to calculate height.
         var bubbleVisibility = bubble.style.visibility;
         bubble.style.visibility = 'hidden';
         bubble.hidden = false;
         // Now we need the bubble to have the new content before calculating
         // size. Undefined |error| == reuse old content.
         if (error !== undefined)
           bubble.replaceContent(error);

         // Get bubble size.
         var bubbleOffsetHeight = parseInt(bubble.offsetHeight);
         var bubbleOffsetWidth = parseInt(bubble.offsetWidth);
         // Restore attributes.
         bubble.style.visibility = bubbleVisibility;
         bubble.hidden = true;

         if (attachment == cr.ui.Bubble.Attachment.BOTTOM) {
           // Move error bubble if it overlaps the shelf.
           if (maxHeight < bubbleOffsetHeight)
             attachment = cr.ui.Bubble.Attachment.TOP;
         } else {
           // Move error bubble if it doesn't fit screen.
           if (maxWidth < bubbleOffsetWidth) {
             bubblePositioningPadding = 2;
             attachment = cr.ui.Bubble.Attachment.LEFT;
           }
         }
         var showBubbleCallback = function() {
           activatedPod.removeEventListener(
               'transitionend', showBubbleCallback);
           $('bubble').showContentForElement(
               bubbleAnchor, attachment, error, BUBBLE_OFFSET,
               bubblePositioningPadding, true);
         };
         activatedPod.addEventListener('transitionend', showBubbleCallback);
         ensureTransitionEndEvent(activatedPod);
       }
     },

     /**
      * Loads given users in pod row.
      * @param {array} users Array of user.
      * @param {boolean} showGuest Whether to show guest session button.
      */
     loadUsers: function(users, showGuest) {
       $('pod-row').loadPods(users);
       $('login-header-bar').showGuestButton = showGuest;
       // On Desktop, #login-header-bar has a shadow if there are 8+ profiles.
       if (Oobe.getInstance().displayType == DISPLAY_TYPE.DESKTOP_USER_MANAGER)
         $('login-header-bar').classList.toggle('shadow', users.length > 8);
     },

     /**
      * Runs app with a given id from the list of loaded apps.
      * @param {!string} app_id of an app to run.
      * @param {boolean=} opt_diagnostic_mode Whether to run the app in
      *     diagnostic mode.  Default is false.
      */
     runAppForTesting: function(app_id, opt_diagnostic_mode) {
       $('pod-row').findAndRunAppForTesting(app_id, opt_diagnostic_mode);
     },

     /**
      * Adds given apps to the pod row.
      * @param {array} apps Array of apps.
      */
     setApps: function(apps) {
       $('pod-row').setApps(apps);
     },

     /**
      * Sets the flag of whether app pods should be visible.
      * @param {boolean} shouldShowApps Whether to show app pods.
      */
     setShouldShowApps: function(shouldShowApps) {
       $('pod-row').setShouldShowApps(shouldShowApps);
     },

     /**
      * Shows the given kiosk app error message.
      * @param {!string} message Error message to show.
      */
     showAppError: function(message) {
       // TODO(nkostylev): Figure out a way to show kiosk app launch error
       // pointing to the kiosk app pod.
       /** @const */ var BUBBLE_PADDING = 12;
       $('bubble').showTextForElement(
           $('pod-row'), message, cr.ui.Bubble.Attachment.BOTTOM,
           $('pod-row').offsetWidth / 2, BUBBLE_PADDING);
     },

     /**
      * Updates current image of a user.
      * @param {string} username User for which to update the image.
      */
     updateUserImage: function(username) {
       $('pod-row').updateUserImage(username);
     },

     /**
      * Updates Caps Lock state (for Caps Lock hint in password input field).
      * @param {boolean} enabled Whether Caps Lock is on.
      */
     setCapsLockState: function(enabled) {
       $('pod-row').classList.toggle('capslock-on', enabled);
     },

     /**
      * Enforces focus on user pod of locked user.
      */
     forceLockedUserPodFocus: function() {
       var row = $('pod-row');
       if (row.lockedPod)
         row.focusPod(row.lockedPod, true);
     },

     /**
      * Remove given user from pod row if it is there.
      * @param {string} user name.
      */
     removeUser: function(username) {
       $('pod-row').removeUserPod(username);
     },

     /**
      * Displays a banner containing |message|. If the banner is already present
      * this function updates the message in the banner. This function is used
      * by the chrome.screenlockPrivate.showMessage API.
      * @param {string} message Text to be displayed or empty to hide the
      *     banner.
      * @param {boolean} isWarning True if the given message is a warning.
      */
     showBannerMessage: function(message, isWarning) {
       var banner = $('signin-banner');
       banner.textContent = message;
       banner.classList.toggle('message-set', !!message);
     },

     /**
      * Shows a custom icon in the user pod of |username|. This function
      * is used by the chrome.screenlockPrivate API.
      * @param {string} username Username of pod to add button
      * @param {!{id: !string,
      *           hardlockOnClick: boolean,
      *           isTrialRun: boolean,
      *           tooltip: ({text: string, autoshow: boolean} | undefined)}}
      * icon The icon parameters.
      */
     showUserPodCustomIcon: function(username, icon) {
       $('pod-row').showUserPodCustomIcon(username, icon);
     },

     /**
      * Hides the custom icon in the user pod of |username| added by
      * showUserPodCustomIcon(). This function is used by the
      * chrome.screenlockPrivate API.
      * @param {string} username Username of pod to remove button
      */
     hideUserPodCustomIcon: function(username) {
       $('pod-row').hideUserPodCustomIcon(username);
     },

     /**
      * Set a fingerprint icon in the user pod of |username|.
      * @param {string} username Username of the selected user
      * @param {number} state Fingerprint unlock state
      */
     setUserPodFingerprintIcon: function(username, state) {
       $('pod-row').setUserPodFingerprintIcon(username, state);
     },

     /**
      * Removes the fingerprint icon in the user pod of |username|.
      * @param {string} username Username of the selected user.
      */
     removeUserPodFingerprintIcon: function(username) {
       $('pod-row').removeUserPodFingerprintIcon(username);
     },

     /**
      * Sets the authentication type used to authenticate the user.
      * @param {string} username Username of selected user
      * @param {number} authType Authentication type, must be a valid value in
      *                          the AUTH_TYPE enum in user_pod_row.js.
      * @param {string} value The initial value to use for authentication.
      */
     setAuthType: function(username, authType, value) {
       $('pod-row').setAuthType(username, authType, value);
     },

     /**
      * Sets the state of tablet mode.
      * @param {boolean} isTabletModeEnabled true if the mode is on.
      */
     setTabletModeState: function(isTabletModeEnabled) {
       $('pod-row').setTabletModeState(isTabletModeEnabled);
     },

     /**
      * Enables or disables the pin keyboard for the given user. This may change
      * pin keyboard visibility.
      * @param {!string} user
      * @param {boolean} enabled
      */
     setPinEnabledForUser: function(user, enabled) {
       $('pod-row').setPinEnabled(user, enabled);
     },

     /**
      * Updates the display name shown on a public session pod.
      * @param {string} userID The user ID of the public session
      * @param {string} displayName The new display name
      */
     setPublicSessionDisplayName: function(userID, displayName) {
       $('pod-row').setPublicSessionDisplayName(userID, displayName);
     },

     /**
      * Updates the list of locales available for a public session.
      * @param {string} userID The user ID of the public session
      * @param {!Object} locales The list of available locales
      * @param {string} defaultLocale The locale to select by default
      * @param {boolean} multipleRecommendedLocales Whether |locales| contains
      *     two or more recommended locales
      */
     setPublicSessionLocales: function(
         userID, locales, defaultLocale, multipleRecommendedLocales) {
       $('pod-row').setPublicSessionLocales(
           userID, locales, defaultLocale, multipleRecommendedLocales);
     },

     /**
      * Updates the list of available keyboard layouts for a public session pod.
      * @param {string} userID The user ID of the public session
      * @param {string} locale The locale to which this list of keyboard layouts
      *     applies
      * @param {!Object} list List of available keyboard layouts
      */
     setPublicSessionKeyboardLayouts: function(userID, locale, list) {
       $('pod-row').setPublicSessionKeyboardLayouts(userID, locale, list);
     },

     /**
      * Updates UI based on the provided lock screen apps state.
      *
      * @param {LOCK_SCREEN_APPS_STATE} state The current lock screen apps
      *     state.
      */
     setLockScreenAppsState: function(state) {
       if (Oobe.getInstance().displayType != DISPLAY_TYPE.LOCK ||
           state == this.lockScreenAppsState_) {
         return;
       }

       this.lockScreenAppsState_ = state;
       $('login-header-bar').lockScreenAppsState = state;
       // When an lock screen app window is in background - i.e. visible behind
       // the lock screen UI - dim the lock screen background, so it's more
       // noticeable that the app widow in background is not actionable.
       $('background')
           .classList.toggle(
               'dimmed-background', state == LOCK_SCREEN_APPS_STATE.BACKGROUND);

       if (state === LOCK_SCREEN_APPS_STATE.FOREGROUND)
         $('pod-row').clearFocusedPod();

     },

     /**
      * Handles clicks on the document which displays the account picker UI.
      * If the click event target is outer container - i.e. background portion
      * of UI with no other UI elements, and lock screen apps are in background,
      * a request is issued to chrome to move lock screen apps to foreground.
      * @param {Event} event The click event.
      */
     handleOwnerDocClick_: function(event) {
       if (this.lockScreenAppsState_ != LOCK_SCREEN_APPS_STATE.BACKGROUND ||
           event.target != $('outer-container')) {
         return;
       }
       chrome.send(
           'setLockScreenAppsState', [LOCK_SCREEN_APPS_STATE.FOREGROUND]);

       event.preventDefault();
       event.stopPropagation();
     },
   };
});
