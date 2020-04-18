// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * @fileoverview Polymer element for displaying material design OOBE.
 */

Polymer({
  is: 'oobe-welcome-md',

  behaviors: [I18nBehavior],

  properties: {
    /**
     * Currently selected system language (display name).
     */
    currentLanguage: {
      type: String,
      value: '',
    },

    /**
     * Currently selected input method (display name).
     */
    currentKeyboard: {
      type: String,
      value: '',
    },

    /**
     * List of languages for language selector dropdown.
     * @type {!Array<OobeTypes.LanguageDsc>}
     */
    languages: {
      type: Array,
      observer: 'onLanguagesChanged_',
    },

    /**
     * List of keyboards for keyboard selector dropdown.
     * @type {!Array<OobeTypes.IMEDsc>}
     */
    keyboards: {
      type: Array,
      observer: 'onKeyboardsChanged_',
    },

    /**
     * Flag that enables MD-OOBE.
     */
    enabled: {
      type: Boolean,
      value: false,
    },

    /**
     * Accessibility options status.
     * @type {!OobeTypes.A11yStatuses}
     */
    a11yStatus: {
      type: Object,
    },

    /**
     * A list of timezones for Timezone Selection screen.
     * @type {!Array<OobeTypes.TimezoneDsc>}
     */
    timezones: {
      type: Object,
      value: [],
    },

    /**
     * If UI uses forced keyboard navigation.
     */
    highlightStrength: {
      type: String,
      value: '',
    },

    /**
     * True when connected to a network.
     * @private
     */
    isConnected_: {
      type: Boolean,
      value: false,
    },

    /**
     * Controls displaying of "Enable debugging features" link.
     */
    debuggingLinkVisible: Boolean,
  },

  /**
   * GUID of the user-selected network. It is remembered after user taps on
   * network entry. After we receive event "connected" on this network,
   * OOBE will proceed.
   * @private {string}
   */
  networkLastSelectedGuid_: '',

  /** @override */
  ready: function() {
    this.updateLocalizedContent();
  },

  /**
   * This is called when UI strings are changed.
   */
  updateLocalizedContent: function() {
    CrOncStrings = {
      OncTypeCellular: loadTimeData.getString('OncTypeCellular'),
      OncTypeEthernet: loadTimeData.getString('OncTypeEthernet'),
      OncTypeTether: loadTimeData.getString('OncTypeTether'),
      OncTypeVPN: loadTimeData.getString('OncTypeVPN'),
      OncTypeWiFi: loadTimeData.getString('OncTypeWiFi'),
      OncTypeWiMAX: loadTimeData.getString('OncTypeWiMAX'),
      networkListItemConnected:
          loadTimeData.getString('networkListItemConnected'),
      networkListItemConnecting:
          loadTimeData.getString('networkListItemConnecting'),
      networkListItemConnectingTo:
          loadTimeData.getString('networkListItemConnectingTo'),
      networkListItemInitializing:
          loadTimeData.getString('networkListItemInitializing'),
      networkListItemScanning:
          loadTimeData.getString('networkListItemScanning'),
      networkListItemNotConnected:
          loadTimeData.getString('networkListItemNotConnected'),
      networkListItemNoNetwork:
          loadTimeData.getString('networkListItemNoNetwork'),
      vpnNameTemplate: loadTimeData.getString('vpnNameTemplate'),

      // Additional strings for custom items.
      addWiFiNetworkMenuName: loadTimeData.getString('addWiFiNetworkMenuName'),
      proxySettingsMenuName: loadTimeData.getString('proxySettingsMenuName'),
    };

    this.$.welcomeScreen.i18nUpdateLocale();
    this.i18nUpdateLocale();
  },

  /**
   * Updates "device in tablet mode" state when tablet mode is changed.
   * @param {Boolean} isInTabletMode True when in tablet mode.
   */
  setTabletModeState: function(isInTabletMode) {
    this.$.welcomeScreen.isInTabletMode = isInTabletMode;
  },

  /**
   * Window-resize event listener (delivered through the display_manager).
   */
  onWindowResize: function() {
    this.$.welcomeScreen.onWindowResize();
  },

  /**
   * Hides all screens to help switching from one screen to another.
   * @private
   */
  hideAllScreens_: function() {
    this.$.welcomeScreen.hidden = true;

    var screens = Polymer.dom(this.root).querySelectorAll('oobe-dialog');
    for (var i = 0; i < screens.length; ++i) {
      screens[i].hidden = true;
    }
  },

  /**
   * Shows given screen.
   * @param id String Screen ID.
   * @private
   */
  showScreen_: function(id) {
    this.hideAllScreens_();

    var screen = this.$[id];
    assert(screen);
    screen.hidden = false;
    screen.show();
  },

  /**
   * Returns active screen object.
   * @private
   */
  getActiveScreen_: function() {
    var screens = Polymer.dom(this.root).querySelectorAll('oobe-dialog');
    for (var i = 0; i < screens.length; ++i) {
      if (!screens[i].hidden)
        return screens[i];
    }
    return this.$.welcomeScreen;
  },

  focus: function() {
    this.getActiveScreen_().focus();
  },

  /** @private */
  onNetworkSelectionScreenShown_: function() {
    // After #networkSelect is stamped, trigger a refresh so that the list
    // will be updated with the currently visible networks and sized
    // appropriately.
    this.async(function() {
      this.$.networkSelect.refreshNetworks();
      this.$.networkSelect.focus();
    }.bind(this));
  },

  /**
   * Handles "visible" event.
   * @private
   */
  onAnimationFinish_: function() {
    this.focus();
  },

  /**
   * Returns custom items for network selector. Shows 'Proxy settings' only
   * when connected to a network.
   * @private
   */
  getNetworkCustomItems_: function(isConnected_) {
    var self = this;
    var items = [];
    if (isConnected_) {
      items.push({
        customItemName: 'proxySettingsMenuName',
        polymerIcon: 'oobe-welcome-20:add-proxy',
        customData: {
          onTap: function() {
            self.OpenInternetDetailDialog_();
          },
        },
      });
    }
    items.push({
      customItemName: 'addWiFiNetworkMenuName',
      polymerIcon: 'oobe-welcome-20:add-wifi',
      customData: {
        onTap: function() {
          self.OpenAddWiFiNetworkDialog_();
        },
      },
    });
    return items;
  },

  /**
   * Returns true if timezone button should be visible.
   * @private
   */
  isTimezoneButtonVisible_: function(highlightStrength) {
    return highlightStrength === 'strong';
  },

  /**
   * Handle "Next" button for "Welcome" screen.
   *
   * @private
   */
  onWelcomeNextButtonClicked_: function() {
    this.showScreen_('networkSelectionScreen');
  },

  /**
   * Handle "launch-advanced-options" button for "Welcome" screen.
   *
   * @private
   */
  onWelcomeLaunchAdvancedOptions_: function() {
    this.showScreen_('oobeAdvancedOptionsScreen');
  },

  /**
   * Handle "Language" button for "Welcome" screen.
   *
   * @private
   */
  onWelcomeSelectLanguageButtonClicked_: function() {
    this.showScreen_('languageScreen');
  },

  /**
   * Handle "Accessibility" button for "Welcome" screen.
   *
   * @private
   */
  onWelcomeAccessibilityButtonClicked_: function() {
    this.showScreen_('accessibilityScreen');
  },

  /**
   * Handle "Timezone" button for "Welcome" screen.
   *
   * @private
   */
  onWelcomeTimezoneButtonClicked_: function() {
    this.showScreen_('timezoneScreen');
  },

  /**
   * Handle Network Setup screen "Proxy settings" button.
   *
   * @private
   */
  OpenInternetDetailDialog_: function(item) {
    chrome.send('launchInternetDetailDialog');
  },

  /**
   * Handle Network Setup screen "Add WiFi network" button.
   *
   * @private
   */
  OpenAddWiFiNetworkDialog_: function(item) {
    chrome.send('launchAddWiFiNetworkDialog');
  },

  /**
   * This is called when network setup is done.
   *
   * @private
   */
  onSelectedNetworkConnected_: function() {
    this.networkLastSelectedGuid_ = '';
    chrome.send('login.NetworkScreen.userActed', ['continue']);
  },

  /**
   * Event triggered when the default network state may have changed.
   * @param {!{detail: ?CrOnc.NetworkStateProperties}} event
   * @private
   */
  onDefaultNetworkChanged_: function(event) {
    var state = event.detail;
    this.isConnected_ =
        state && state.ConnectionState == CrOnc.ConnectionState.CONNECTED;
  },

  /**
   * Event triggered when a cr-network-list-item connection state changes.
   * @param {!{detail: !CrOnc.NetworkStateProperties}} event
   * @private
   */
  onNetworkConnectChanged_: function(event) {
    var state = event.detail;
    if (state && state.GUID == this.networkLastSelectedGuid_ &&
        state.ConnectionState == CrOnc.ConnectionState.CONNECTED) {
      this.onSelectedNetworkConnected_();
    }
  },

  /**
   * This is called when user taps on network entry in networks list.
   *
   * @param {!{detail: !CrOnc.NetworkStateProperties}} event
   * @private
   */
  onNetworkListNetworkItemSelected_: function(event) {
    var state = event.detail;
    assert(state);
    // If a connected network is selected, continue to the next screen.
    if (state.ConnectionState == CrOnc.ConnectionState.CONNECTED) {
      this.onSelectedNetworkConnected_();
      return;
    }

    // If user has previously selected another network, there
    // is pending connection attempt. So even if new selection is currently
    // connected, it may get disconnected at any time.
    // So just send one more connection request to cancel current attempts.
    this.networkLastSelectedGuid_ = (state ? state.GUID : '');

    if (!state)
      return;

    var self = this;
    var networkStateCopy = Object.assign({}, state);

    // Cellular should normally auto connect. If it is selected, show the
    // details UI since there is no configuration UI for Cellular.
    if (state.Type == chrome.networkingPrivate.NetworkType.CELLULAR) {
      chrome.send('showNetworkDetails', [state.Type, state.GUID]);
      return;
    }

    // Allow proxy to be set for connected networks.
    if (state.ConnectionState == CrOnc.ConnectionState.CONNECTED) {
      chrome.send('showNetworkDetails', [state.Type, state.GUID]);
      return;
    }

    if (state.Connectable === false || state.ErrorState) {
      chrome.send('showNetworkConfig', [state.GUID]);
      return;
    }

    chrome.networkingPrivate.startConnect(state.GUID, () => {
      const lastError = chrome.runtime.lastError;
      if (!lastError)
        return;
      const message = lastError.message;
      if (message == 'connecting' || message == 'connect-canceled' ||
          message == 'connected' || message == 'Error.InvalidNetworkGuid') {
        return;
      }
      console.error(
          'networkingPrivate.startConnect error: ' + message +
          ' For: ' + state.GUID);
      chrome.send('showNetworkConfig', [state.GUID]);
    });
  },

  /**
   * @param {!Event} event
   * @private
   */
  onNetworkListCustomItemSelected_: function(e) {
    var itemState = e.detail;
    itemState.customData.onTap();
  },

  /**
   * Handle "<- Back" button on network selection screen.
   *
   * @private
   */
  onNetworkSelectionBackButtonPressed_: function() {
    this.networkLastSelectedGuid_ = '';
    this.showScreen_('welcomeScreen');
  },

  /**
   * Handle language selection.
   *
   * @param {!{detail: {!OobeTypes.LanguageDsc}}} event
   * @private
   */
  onLanguageSelected_: function(event) {
    var item = event.detail;
    var languageId = item.value;
    this.currentLanguage = item.title;
    this.screen.onLanguageSelected_(languageId);
  },

  /**
   * Handle keyboard layout selection.
   *
   * @param {!{detail: {!OobeTypes.IMEDsc}}} event
   * @private
   */
  onKeyboardSelected_: function(event) {
    var item = event.detail;
    var inputMethodId = item.value;
    this.currentKeyboard = item.title;
    this.screen.onKeyboardSelected_(inputMethodId);
  },

  onLanguagesChanged_: function() {
    this.currentLanguage = getSelectedTitle(this.languages);
  },

  setSelectedKeyboard: function(keyboard_id) {
    var found = false;
    for (var i = 0; i < this.keyboards.length; ++i) {
      if (this.keyboards[i].value != keyboard_id) {
        this.keyboards[i].selected = false;
        continue;
      }
      this.keyboards[i].selected = true;
      found = true;
    }
    if (!found)
      return;

    // Force i18n-dropdown to refresh.
    this.keyboards = this.keyboards.slice();
    this.onKeyboardsChanged_();
  },

  onKeyboardsChanged_: function() {
    this.currentKeyboard = getSelectedTitle(this.keyboards);
  },

  /**
   * Handle "OK" button for "LanguageSelection" screen.
   *
   * @private
   */
  closeLanguageSection_: function() {
    this.showScreen_('welcomeScreen');
  },

  /** ******************** Accessibility section ******************* */

  /**
   * Handle "OK" button for "Accessibility Options" screen.
   *
   * @private
   */
  closeAccessibilitySection_: function() {
    this.showScreen_('welcomeScreen');
  },

  /**
   * Handle all accessibility buttons.
   * Note that each <oobe-a11y-option> has chromeMessage attribute
   * containing Chromium callback name.
   *
   * @private
   * @param {!Event} event
   */
  onA11yOptionChanged_: function(event) {
    chrome.send(
        event.currentTarget.chromeMessage, [event.currentTarget.checked]);
  },

  /** ******************** Timezone section ******************* */

  /**
   * Handle "OK" button for "Timezone Selection" screen.
   *
   * @private
   */
  closeTimezoneSection_: function() {
    this.showScreen_('welcomeScreen');
  },

  /**
   * Handle timezone selection.
   *
   * @param {!{detail: {!OobeTypes.Timezone}}} event
   * @private
   */
  onTimezoneSelected_: function(event) {
    var item = event.detail;
    if (!item)
      return;

    this.screen.onTimezoneSelected_(item.value);
  },

  /** ******************** AdvancedOptions section ******************* */

  /**
   * Handle "OK" button for "AdvancedOptions Selection" screen.
   *
   * @private
   */
  closeAdvancedOptionsSection_: function() {
    this.showScreen_('welcomeScreen');
  },

  /**
   * Handle click on "Enable remote enrollment" option.
   *
   * @private
   */
  onEEBootstrappingClicked_: function() {
    cr.ui.Oobe.handleAccelerator(ACCELERATOR_BOOTSTRAPPING_SLAVE);
  },

  /**
   * Handle click on "Set up as CFM device" option.
   *
   * @private
   */
  onCFMBootstrappingClicked_: function() {
    cr.ui.Oobe.handleAccelerator(ACCELERATOR_DEVICE_REQUISITION_REMORA);
  },

  /**
   * Handle click on "Device requisition" option.
   *
   * @private
   */
  onDeviceRequisitionClicked_: function() {
    cr.ui.Oobe.handleAccelerator(ACCELERATOR_DEVICE_REQUISITION);
  },
});
