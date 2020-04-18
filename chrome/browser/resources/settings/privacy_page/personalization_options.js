// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * @fileoverview
 * 'personalization-options' contains several toggles related to
 * personalizations.
 */
(function() {

/**
 * Must be kept in sync with the C++ enum of the same name.
 * @enum {number}
 */
const NetworkPredictionOptions = {
  ALWAYS: 0,
  WIFI_ONLY: 1,
  NEVER: 2,
  DEFAULT: 1,
};

Polymer({
  is: 'settings-personalization-options',

  behaviors: [
    WebUIListenerBehavior,
  ],

  properties: {
    prefs: {
      type: Object,
      notify: true,
    },

    /**
     * Dictionary defining page visibility.
     * @type {!PrivacyPageVisibility}
     */
    pageVisibility: Object,

    /** @private {chrome.settingsPrivate.PrefObject} */
    safeBrowsingExtendedReportingPref_: {
      type: Object,
      value: function() {
        return /** @type {chrome.settingsPrivate.PrefObject} */ ({});
      },
    },

    /**
     * Used for HTML bindings. This is defined as a property rather than within
     * the ready callback, because the value needs to be available before
     * local DOM initialization - otherwise, the toggle has unexpected behavior.
     * @private
     */
    networkPredictionEnum_: {
      type: Object,
      value: NetworkPredictionOptions,
    },

    unifiedConsentEnabled: Boolean,

    // <if expr="_google_chrome and not chromeos">
    // TODO(dbeam): make a virtual.* pref namespace and set/get this normally
    // (but handled differently in C++).
    /** @private {chrome.settingsPrivate.PrefObject} */
    metricsReportingPref_: {
      type: Object,
      value: function() {
        // TODO(dbeam): this is basically only to appease PrefControlBehavior.
        // Maybe add a no-validate attribute instead? This makes little sense.
        return /** @type {chrome.settingsPrivate.PrefObject} */ ({});
      },
    },

    /** @private */
    showRestart_: Boolean,
    // </if>
  },

  /** @override */
  ready: function() {
    this.browserProxy_ = settings.PrivacyPageBrowserProxyImpl.getInstance();

    const setSber = this.setSafeBrowsingExtendedReporting_.bind(this);
    this.addWebUIListener('safe-browsing-extended-reporting-change', setSber);
    this.browserProxy_.getSafeBrowsingExtendedReporting().then(setSber);

    // <if expr="_google_chrome and not chromeos">
    const setMetricsReportingPref = this.setMetricsReportingPref_.bind(this);
    this.addWebUIListener('metrics-reporting-change', setMetricsReportingPref);
    this.browserProxy_.getMetricsReporting().then(setMetricsReportingPref);
    // </if>
  },

  /** @private */
  onSberChange_: function() {
    const enabled = this.$.safeBrowsingExtendedReportingControl.checked;
    this.browserProxy_.setSafeBrowsingExtendedReportingEnabled(enabled);
  },

  /**
   * @param {!Event} e
   * @private
   */
  onLanguageLinkBoxClick_: function(e) {
    if (e.target.tagName === 'A') {
      e.preventDefault();
      settings.navigateTo(settings.routes.LANGUAGES);
    }
  },

  /**
   * @param {!SberPrefState} sberPrefState SBER enabled and managed state.
   * @private
   */
  setSafeBrowsingExtendedReporting_: function(sberPrefState) {
    // Ignore the next change because it will happen when we set the pref.
    const pref = {
      key: '',
      type: chrome.settingsPrivate.PrefType.BOOLEAN,
      value: sberPrefState.enabled,
    };
    if (sberPrefState.managed) {
      pref.enforcement = chrome.settingsPrivate.Enforcement.ENFORCED;
      pref.controlledBy = chrome.settingsPrivate.ControlledBy.USER_POLICY;
    }
    this.safeBrowsingExtendedReportingPref_ = pref;
  },

  // <if expr="_google_chrome and not chromeos">
  /** @private */
  onMetricsReportingChange_: function() {
    const enabled = this.$.metricsReportingControl.checked;
    this.browserProxy_.setMetricsReportingEnabled(enabled);
  },

  /**
   * @param {!MetricsReporting} metricsReporting
   * @private
   */
  setMetricsReportingPref_: function(metricsReporting) {
    const hadPreviousPref = this.metricsReportingPref_.value !== undefined;
    const pref = {
      key: '',
      type: chrome.settingsPrivate.PrefType.BOOLEAN,
      value: metricsReporting.enabled,
    };
    if (metricsReporting.managed) {
      pref.enforcement = chrome.settingsPrivate.Enforcement.ENFORCED;
      pref.controlledBy = chrome.settingsPrivate.ControlledBy.USER_POLICY;
    }

    // Ignore the next change because it will happen when we set the pref.
    this.metricsReportingPref_ = pref;

    // TODO(dbeam): remember whether metrics reporting was enabled when Chrome
    // started.
    if (metricsReporting.managed)
      this.showRestart_ = false;
    else if (hadPreviousPref)
      this.showRestart_ = true;
  },

  /**
   * @param {!Event} e
   * @private
   */
  onRestartTap_: function(e) {
    e.stopPropagation();
    settings.LifetimeBrowserProxyImpl.getInstance().restart();
  },
  // </if>
});
})();
