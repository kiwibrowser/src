// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * @fileoverview Oobe network screen implementation.
 */

login.createScreen('NetworkScreen', 'connect', function() {
  var USER_ACTION_CONTINUE_BUTTON_CLICKED = 'continue';
  var USER_ACTION_CONNECT_DEBUGGING_FEATURES_CLICKED =
      'connect-debugging-features';
  var CONTEXT_KEY_LOCALE = 'locale';
  var CONTEXT_KEY_INPUT_METHOD = 'input-method';
  var CONTEXT_KEY_TIMEZONE = 'timezone';
  var CONTEXT_KEY_CONTINUE_BUTTON_ENABLED = 'continue-button-enabled';

  return {
    EXTERNAL_API: ['showError'],

    /**
     * Dropdown element for networks selection.
     */
    dropdown_: null,

    /** @override */
    decorate: function() {
      setupSelect(
          $('language-select'), loadTimeData.getValue('languageList'),
          this.onLanguageSelected_.bind(this));
      setupSelect(
          $('keyboard-select'), loadTimeData.getValue('inputMethodsList'),
          this.onKeyboardSelected_.bind(this));
      setupSelect(
          $('timezone-select'), loadTimeData.getValue('timezoneList'),
          this.onTimezoneSelected_.bind(this));

      // ---------- Welcome screen
      var welcomeScreen = $('oobe-welcome-md');
      welcomeScreen.screen = this;

      var languageList = loadTimeData.getValue('languageList');
      welcomeScreen.languages = languageList;

      var inputMethodsList = loadTimeData.getValue('inputMethodsList');
      welcomeScreen.keyboards = inputMethodsList;

      var timezoneList = loadTimeData.getValue('timezoneList');
      welcomeScreen.timezones = timezoneList;

      welcomeScreen.highlightStrength =
          loadTimeData.getValue('highlightStrength');
      // -------------------------

      this.dropdown_ = $('networks-list');
      cr.ui.DropDown.decorate(this.dropdown_);

      this.declareUserAction($('connect-debugging-features-link'), {
        action_id: USER_ACTION_CONNECT_DEBUGGING_FEATURES_CLICKED,
        event: 'click'
      });
      this.declareUserAction($('connect-debugging-features-link'), {
        action_id: USER_ACTION_CONNECT_DEBUGGING_FEATURES_CLICKED,
        condition: function(event) {
          return event.keyCode == 32;
        },
        event: 'keyup'
      });

      this.context.addObserver(
          CONTEXT_KEY_INPUT_METHOD, function(inputMethodId) {
            $('oobe-welcome-md').setSelectedKeyboard(inputMethodId);

            option =
                $('keyboard-select')
                    .querySelector('option[value="' + inputMethodId + '"]');
            if (option)
              option.selected = true;
          });
      this.context.addObserver(CONTEXT_KEY_TIMEZONE, function(timezoneId) {
        $('timezone-select').value = timezoneId;
      });
      this.context.addObserver(
          CONTEXT_KEY_CONTINUE_BUTTON_ENABLED, function(enabled) {
            $('continue-button').disabled = !enabled;
          });
    },

    onLanguageSelected_: function(languageId) {
      this.context.set(CONTEXT_KEY_LOCALE, languageId);
      this.commitContextChanges();
    },

    onKeyboardSelected_: function(inputMethodId) {
      this.context.set(CONTEXT_KEY_INPUT_METHOD, inputMethodId);
      this.commitContextChanges();
    },

    onTimezoneSelected_: function(timezoneId) {
      this.context.set(CONTEXT_KEY_TIMEZONE, timezoneId);
      this.commitContextChanges();
    },

    onBeforeShow: function(data) {
      this.setMDMode_();
      cr.ui.DropDown.show('networks-list', true, -1);
      var debuggingLinkVisible =
          data && 'isDeveloperMode' in data && data['isDeveloperMode'];

      this.classList.toggle('connect-debugging-view', debuggingLinkVisible);
      $('oobe-welcome-md').debuggingLinkVisible = debuggingLinkVisible;
    },

    onBeforeHide: function() {
      cr.ui.DropDown.hide('networks-list');
    },

    /**
     * Header text of the screen.
     * @type {string}
     */
    get header() {
      return loadTimeData.getString('networkScreenTitle');
    },

    /**
     * Buttons in oobe wizard's button strip.
     * @type {array} Array of Buttons.
     */
    get buttons() {
      var buttons = [];

      var continueButton = this.declareButton(
          'continue-button', USER_ACTION_CONTINUE_BUTTON_CLICKED);
      continueButton.disabled = !this.context.get(
          CONTEXT_KEY_CONTINUE_BUTTON_ENABLED, false /* default */);
      continueButton.textContent = loadTimeData.getString('continueButton');
      continueButton.classList.add('preserve-disabled-state');
      buttons.push(continueButton);

      return buttons;
    },

    /**
     * Returns a control which should receive an initial focus.
     */
    get defaultControl() {
      if (loadTimeData.getString('newOobeUI') == 'on')
        return $('oobe-welcome-md');

      return $('language-select');
    },

    /**
     * Shows the network error message.
     * @param {string} message Message to be shown.
     */
    showError: function(message) {
      var error = document.createElement('div');
      var messageDiv = document.createElement('div');
      messageDiv.className = 'error-message-bubble';
      messageDiv.textContent = message;
      error.appendChild(messageDiv);
      error.setAttribute('role', 'alert');

      $('bubble').showContentForElement(
          $('networks-list'), cr.ui.Bubble.Attachment.BOTTOM, error);
    },

    /**
     * This is called after resources are updated.
     */
    updateLocalizedContent: function() {
      this.setMDMode_();
      $('oobe-welcome-md').updateLocalizedContent();
    },

    /**
     * Updates "device in tablet mode" state when tablet mode is changed.
     * @param {Boolean} isInTabletMode True when in tablet mode.
     */
    setTabletModeState: function(isInTabletMode) {
      $('oobe-welcome-md').setTabletModeState(isInTabletMode);
    },

    /**
     * Window-resize event listener (delivered through the display_manager).
     */
    onWindowResize: function() {
      $('oobe-welcome-md').onWindowResize();
    },

    /**
     * This method takes care of switching to material-design OOBE.
     * @private
     */
    setMDMode_: function() {
      var useMDOobe = (loadTimeData.getString('newOobeUI') == 'on');

      $('oobe-connect').hidden = useMDOobe;
      $('oobe-welcome-md').hidden = !useMDOobe;

      if (useMDOobe) {
        var welcomeScreen = $('oobe-welcome-md');
        var languageList = loadTimeData.getValue('languageList');
        welcomeScreen.languages = languageList;

        welcomeScreen.keyboards = loadTimeData.getValue('inputMethodsList');
        welcomeScreen.enabled = true;

        var timezoneList = loadTimeData.getValue('timezoneList');
        welcomeScreen.timezones = timezoneList;

        welcomeScreen.highlightStrength =
            loadTimeData.getValue('highlightStrength');
      }
    },
  };
});
