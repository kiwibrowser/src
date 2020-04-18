// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * @fileoverview 'settings-clear-browsing-data-dialog' allows the user to
 * delete browsing data that has been cached by Chromium.
 */
Polymer({
  is: 'settings-clear-browsing-data-dialog',

  behaviors: [WebUIListenerBehavior, settings.RouteObserverBehavior],

  properties: {
    /**
     * Preferences state.
     */
    prefs: {
      type: Object,
      notify: true,
    },

    /**
     * Results of browsing data counters, keyed by the suffix of
     * the corresponding data type deletion preference, as reported
     * by the C++ side.
     * @private {!Object<string>}
     */
    counters_: {
      type: Object,
      // Will be filled as results are reported.
      value: function() {
        return {};
      }
    },

    /**
     * List of options for the dropdown menu.
     * @private {!DropdownMenuOptionList}
     */
    clearFromOptions_: {
      readOnly: true,
      type: Array,
      value: [
        {value: 0, name: loadTimeData.getString('clearPeriodHour')},
        {value: 1, name: loadTimeData.getString('clearPeriod24Hours')},
        {value: 2, name: loadTimeData.getString('clearPeriod7Days')},
        {value: 3, name: loadTimeData.getString('clearPeriod4Weeks')},
        {value: 4, name: loadTimeData.getString('clearPeriodEverything')},
      ],
    },

    /** @private */
    clearingInProgress_: {
      type: Boolean,
      value: false,
    },

    /** @private */
    clearButtonDisabled_: {
      type: Boolean,
      value: false,
    },

    /** @private */
    isSupervised_: {
      type: Boolean,
      value: function() {
        return loadTimeData.getBoolean('isSupervised');
      },
    },

    /** @private */
    showHistoryDeletionDialog_: {
      type: Boolean,
      value: false,
    },

    /** @private */
    isSignedIn_: {
      type: Boolean,
      value: false,
    },

    /** @private */
    isSyncingHistory_: {
      type: Boolean,
      value: false,
    },

    /** @private {!Array<ImportantSite>} */
    importantSites_: {
      type: Array,
      value: function() {
        return [];
      }
    },

    /** @private */
    importantSitesFlagEnabled_: {
      type: Boolean,
      value: function() {
        return loadTimeData.getBoolean('importantSitesInCbd');
      },
    },

    /** @private */
    showImportantSitesDialog_: {
      type: Boolean,
      value: false,
    },

    /** @private */
    showImportantSitesCacheSubtitle_: {
      type: Boolean,
      value: false,
    },

    /**
     * Time in ms, when the dialog was opened.
     * @private
     */
    dialogOpenedTime_: {
      type: Number,
      value: 0,
    }
  },

  listeners: {'settings-boolean-control-change': 'updateClearButtonState_'},

  /** @private {settings.ClearBrowsingDataBrowserProxy} */
  browserProxy_: null,

  /** @override */
  ready: function() {
    this.addWebUIListener(
        'update-sync-state', this.updateSyncState_.bind(this));
    this.addWebUIListener(
        'update-counter-text', this.updateCounterText_.bind(this));
  },

  /** @override */
  attached: function() {
    this.browserProxy_ =
        settings.ClearBrowsingDataBrowserProxyImpl.getInstance();
    this.dialogOpenedTime_ = Date.now();
    this.browserProxy_.initialize().then(() => {
      this.$.clearBrowsingDataDialog.showModal();
    });

    if (this.importantSitesFlagEnabled_) {
      this.browserProxy_.getImportantSites().then(sites => {
        this.importantSites_ = sites;
      });
    }
  },

  /**
   * Returns true if either clearing is in progress or no data type is selected.
   * @param {boolean} clearingInProgress
   * @param {boolean} clearButtonDisabled
   * @return {boolean}
   * @private
   */
  isClearButtonDisabled_: function(clearingInProgress, clearButtonDisabled) {
    return clearingInProgress || clearButtonDisabled;
  },

  /**
   * Disables the Clear Data button if no data type is selected.
   * @private
   */
  updateClearButtonState_: function() {
    // on-select-item-changed gets called with undefined during a tab change.
    // https://github.com/PolymerElements/iron-selector/issues/95
    const tab = this.$.tabs.selectedItem;
    if (!tab)
      return;
    this.clearButtonDisabled_ = this.getSelectedDataTypes_(tab).length == 0;
  },

  /**
   * Record visits to the CBD dialog.
   *
   * settings.RouteObserverBehavior
   * @param {!settings.Route} currentRoute
   * @protected
   */
  currentRouteChanged: function(currentRoute) {
    if (currentRoute == settings.routes.CLEAR_BROWSER_DATA) {
      chrome.metricsPrivate.recordUserAction('ClearBrowsingData_DialogCreated');
      this.dialogOpenedTime_ = Date.now();
    }
  },

  /**
   * Updates the history description to show the relevant information
   * depending on sync and signin state.
   *
   * @param {boolean} signedIn Whether the user is signed in.
   * @param {boolean} syncing Whether the user is syncing history.
   * @private
   */
  updateSyncState_: function(signedIn, syncing) {
    this.isSignedIn_ = signedIn;
    this.isSyncingHistory_ = syncing;
    this.$.clearBrowsingDataDialog.classList.add('fully-rendered');
  },

  /**
   * Choose a summary checkbox label.
   * @param {boolean} isSignedIn
   * @param {boolean} isSyncingHistory
   * @param {string} historySummary
   * @param {string} historySummarySigned
   * @param {string} historySummarySynced
   * @return {string}
   * @private
   */
  browsingCheckboxLabel_: function(
      isSignedIn, isSyncingHistory, historySummary, historySummarySigned,
      historySummarySynced) {
    if (isSyncingHistory) {
      return historySummarySynced;
    } else if (isSignedIn) {
      return historySummarySigned;
    }
    return historySummary;
  },

  /**
   * Choose a content/site settings label.
   * @param {string} siteSettings
   * @param {string} contentSettings
   * @return {string}
   * @private
   */
  siteSettingsLabel_: function(siteSettings, contentSettings) {
    return loadTimeData.getBoolean('enableSiteSettings') ? siteSettings :
                                                           contentSettings;
  },

  /**
   * Updates the text of a browsing data counter corresponding to the given
   * preference.
   * @param {string} prefName Browsing data type deletion preference.
   * @param {string} text The text with which to update the counter
   * @private
   */
  updateCounterText_: function(prefName, text) {
    // Data type deletion preferences are named "browser.clear_data.<datatype>".
    // Strip the common prefix, i.e. use only "<datatype>".
    const matches = prefName.match(/^browser\.clear_data\.(\w+)$/);
    this.set('counters_.' + assert(matches[1]), text);
  },

  /**
   * @return {boolean} Whether the ImportantSites dialog should be shown.
   * @private
   */
  shouldShowImportantSites_: function() {
    if (!this.importantSitesFlagEnabled_)
      return false;
    const tab = this.$.tabs.selectedItem;
    if (!tab.querySelector('.cookies-checkbox').checked) {
      return false;
    }

    const haveImportantSites = this.importantSites_.length > 0;
    chrome.send(
        'metricsHandler:recordBooleanHistogram',
        ['History.ClearBrowsingData.ImportantDialogShown', haveImportantSites]);
    return haveImportantSites;
  },

  /**
   * Handles the tap on the Clear Data button.
   * @private
   */
  onClearBrowsingDataTap_: function() {
    if (this.shouldShowImportantSites_()) {
      const tab = this.$.tabs.selectedItem;
      this.showImportantSitesDialog_ = true;
      this.showImportantSitesCacheSubtitle_ =
          tab.querySelector('.cache-checkbox').checked;
      this.$.clearBrowsingDataDialog.close();
      // Show important sites dialog after dom-if is applied.
      this.async(() => this.$$('#importantSitesDialog').showModal());
    } else {
      this.clearBrowsingData_();
    }
  },

  /**
   * Handles closing of the clear browsing data dialog. Stops the close
   * event from propagating if another dialog is shown to prevent the
   * privacy-page from closing this dialog.
   * @private
   */
  onClearBrowsingDataDialogClose_: function(event) {
    if (this.showImportantSitesDialog_)
      event.stopPropagation();
  },

  /**
   * Returns a list of selected data types.
   * @param {!HTMLElement} tab
   * @return {!Array<string>}
   * @private
   */
  getSelectedDataTypes_: function(tab) {
    const checkboxes = tab.querySelectorAll('settings-checkbox');
    const dataTypes = [];
    checkboxes.forEach((checkbox) => {
      if (checkbox.checked && !checkbox.hidden)
        dataTypes.push(checkbox.pref.key);
    });
    return dataTypes;
  },

  /**
   * Clears browsing data and maybe shows a history notice.
   * @private
   */
  clearBrowsingData_: function() {
    this.clearingInProgress_ = true;
    const tab = this.$.tabs.selectedItem;
    const dataTypes = this.getSelectedDataTypes_(tab);
    const timePeriod = tab.querySelector('.time-range-select').pref.value;

    if (tab.id == 'basic-tab') {
      chrome.metricsPrivate.recordUserAction('ClearBrowsingData_BasicTab');
    } else {
      chrome.metricsPrivate.recordUserAction('ClearBrowsingData_AdvancedTab');
    }

    this.browserProxy_
        .clearBrowsingData(dataTypes, timePeriod, this.importantSites_)
        .then(shouldShowNotice => {
          this.clearingInProgress_ = false;
          this.showHistoryDeletionDialog_ = shouldShowNotice;
          chrome.metricsPrivate.recordMediumTime(
              'History.ClearBrowsingData.TimeSpentInDialog',
              Date.now() - this.dialogOpenedTime_);
          if (!shouldShowNotice)
            this.closeDialogs_();
        });
  },

  /**
   * Closes the clear browsing data or important site dialog if they are open.
   * @private
   */
  closeDialogs_: function() {
    if (this.$.clearBrowsingDataDialog.open)
      this.$.clearBrowsingDataDialog.close();
    if (this.showImportantSitesDialog_)
      this.$$('#importantSitesDialog').close();
  },

  /** @private */
  onCancelTap_: function() {
    this.$.clearBrowsingDataDialog.cancel();
  },

  /**
   * Handles the tap confirm button in important sites.
   * @private
   */
  onImportantSitesConfirmTap_: function() {
    this.clearBrowsingData_();
  },

  /** @private */
  onImportantSitesCancelTap_: function() {
    /** @type {!CrDialogElement} */ (this.$$('#importantSitesDialog')).cancel();
  },

  /**
   * Handles the closing of the notice about other forms of browsing history.
   * @private
   */
  onHistoryDeletionDialogClose_: function() {
    this.showHistoryDeletionDialog_ = false;
    this.closeDialogs_();
  },

  /**
   * Records an action when the user changes between the basic and advanced tab.
   * @param {!Event} event
   * @private
   */
  recordTabChange_: function(event) {
    if (event.detail.value == 0) {
      chrome.metricsPrivate.recordUserAction(
          'ClearBrowsingData_SwitchTo_BasicTab');
    } else {
      chrome.metricsPrivate.recordUserAction(
          'ClearBrowsingData_SwitchTo_AdvancedTab');
    }
  },
});
