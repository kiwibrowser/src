// Copyright (c) 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * @fileoverview Login UI based on a stripped down OOBE controller.
 */

// <include src="md_login_shared.js">

/**
 * Ensures that the pin keyboard is loaded.
 * @param {function()} onLoaded Callback run when the pin keyboard is loaded.
 */
function ensurePinKeyboardLoaded(onLoaded) {
  'use strict';

  // The element we want to see if loaded.
  var getPinKeyboard = function() {
    return $('pod-row').querySelectorAll('pin-keyboard')[0];
  };

  // Do not reload assets if they are already loaded. Run |onLoaded| once assets
  // are done loading, though.
  if (cr.ui.login.ResourceLoader.hasDeferredAssets('custom-elements')) {
    cr.ui.login.ResourceLoader.waitUntilLayoutComplete(
        getPinKeyboard, onLoaded);
    return;
  }

  // Register loader for custom elements.
  cr.ui.login.ResourceLoader.registerAssets({
    id: 'custom-elements',
    html: [{url: 'chrome://oobe/custom_elements.html'}]
  });

  // We only load the PIN element when it is actually shown so that lock screen
  // load times remain low when the user is not using a PIN.
  //
  // Loading the PIN element blocks the DOM, which will interrupt any running
  // animations. We load the PIN after an idle notification to allow the pod
  // fly-in animation to complete without interruption.
  cr.ui.login.ResourceLoader.loadAssetsOnIdle('custom-elements', function() {
    cr.ui.login.ResourceLoader.waitUntilLayoutComplete(
        getPinKeyboard, onLoaded);
  });
}

cr.define('cr.ui.Oobe', function() {
  return {
    /**
     * Initializes the OOBE flow.  This will cause all C++ handlers to
     * be invoked to do final setup.
     */
    initialize: function() {
      cr.ui.login.DisplayManager.initialize();
      login.AccountPickerScreen.register();

      cr.ui.Bubble.decorate($('bubble-persistent'));
      $('bubble-persistent').persistent = true;
      $('bubble-persistent').hideOnKeyPress = false;

      cr.ui.Bubble.decorate($('bubble'));
      login.HeaderBar.decorate($('login-header-bar'));
      login.TopHeaderBar.decorate($('top-header-bar'));

      chrome.send('screenStateInitialize');
    },

    /**
     * Notification from the host that the PIN keyboard will be used in the
     * lock session so it should also get preloaded.
     */
    preloadPinKeyboard: function() {
      ensurePinKeyboardLoaded(function() {});
    },

    // Dummy Oobe functions not present with stripped login UI.
    initializeA11yMenu: function(e) {},
    handleAccessibilityLinkClick: function(e) {},
    handleSpokenFeedbackClick: function(e) {},
    handleHighContrastClick: function(e) {},
    handleScreenMagnifierClick: function(e) {},
    setUsageStats: function(checked) {},
    setOemEulaUrl: function(oemEulaUrl) {},
    setTpmPassword: function(password) {},
    refreshA11yInfo: function(data) {},
    reloadEulaContent: function(data) {},

    /**
     * Reloads content of the page.
     * @param {!Object} data New dictionary with i18n values.
     */
    reloadContent: function(data) {
      loadTimeData.overrideValues(data);
      i18nTemplate.process(document, loadTimeData);
      Oobe.getInstance().updateLocalizedContent_();
    },

    /**
     * Updates "device in tablet mode" state when tablet mode is changed.
     * @param {Boolean} isInTabletMode True when in tablet mode.
     */
    setTabletModeState: function(isInTabletMode) {
      Oobe.getInstance().setTabletModeState_(isInTabletMode);
    },
  };
});
