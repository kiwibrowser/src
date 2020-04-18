// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

(function() {

/**
 * Names of the radio buttons which allow the user to choose their encryption
 * mechanism.
 * @enum {string}
 */
const RadioButtonNames = {
  ENCRYPT_WITH_GOOGLE: 'encrypt-with-google',
  ENCRYPT_WITH_PASSPHRASE: 'encrypt-with-passphrase',
};

/**
 * Names of the individual data type properties to be cached from
 * settings.SyncPrefs when the user checks 'Sync All'.
 * @type {!Array<string>}
 */
const SyncPrefsIndividualDataTypes = [
  'appsSynced',
  'extensionsSynced',
  'preferencesSynced',
  'autofillSynced',
  'typedUrlsSynced',
  'themesSynced',
  'bookmarksSynced',
  'passwordsSynced',
  'tabsSynced',
  'paymentsIntegrationEnabled',
];

/**
 * @fileoverview
 * 'settings-sync-page' is the settings page containing sync settings.
 */
Polymer({
  is: 'settings-sync-page',

  behaviors: [
    WebUIListenerBehavior,
    settings.RouteObserverBehavior,
  ],

  properties: {
    /**
     * Preferences state.
     */
    prefs: {
      type: Object,
      notify: true,
    },

    /** @private */
    pages_: {
      type: Object,
      value: settings.PageStatus,
      readOnly: true,
    },

    /**
     * The current page status. Defaults to |CONFIGURE| such that the searching
     * algorithm can search useful content when the page is not visible to the
     * user.
     * @private {?settings.PageStatus}
     */
    pageStatus_: {
      type: String,
      value: settings.PageStatus.CONFIGURE,
    },

    /**
     * Dictionary defining page visibility.
     * @type {!PrivacyPageVisibility}
     */
    pageVisibility: Object,

    /**
     * The current sync preferences, supplied by SyncBrowserProxy.
     * @type {settings.SyncPrefs|undefined}
     */
    syncPrefs: {
      type: Object,
    },

    /** @type {settings.SyncStatus} */
    syncStatus: {
      type: Object,
      observer: 'onSyncStatusChanged_',
    },

    /**
     * Whether the "create passphrase" inputs should be shown. These inputs
     * give the user the opportunity to use a custom passphrase instead of
     * authenticating with their Google credentials.
     * @private
     */
    creatingNewPassphrase_: {
      type: Boolean,
      value: false,
    },

    /**
     * The passphrase input field value.
     * @private
     */
    passphrase_: {
      type: String,
      value: '',
    },

    /**
     * The passphrase confirmation input field value.
     * @private
     */
    confirmation_: {
      type: String,
      value: '',
    },

    /**
     * The existing passphrase input field value.
     * @private
     */
    existingPassphrase_: {
      type: String,
      value: '',
    },

    /** @private */
    syncSectionDisabled_: {
      type: Boolean,
      value: false,
      computed: 'computeSyncSectionDisabled_(' +
          'unifiedConsentEnabled, syncStatus.signedIn)',
    },

    /** @private */
    personalizeSectionOpened_: {
      type: Boolean,
      value: true,
    },

    /** @private */
    syncSectionOpened_: {
      type: Boolean,
      value: true,
    },

    // <if expr="not chromeos">
    diceEnabled: Boolean,
    // </if>

    unifiedConsentEnabled: Boolean,
  },

  /** @private {?settings.SyncBrowserProxy} */
  browserProxy_: null,

  /**
   * The unload callback is needed because the sign-in flow needs to know
   * if the user has closed the tab with the sync settings. This property is
   * non-null if the user is currently navigated on the sync settings route.
   *
   * TODO(scottchen): We had to change from unload to beforeunload due to
   *     crbug.com/501292. Change back to unload once it's fixed.
   *
   * @private {?Function}
   */
  beforeunloadCallback_: null,

  /**
   * Whether the user decided to abort sync.
   * @private {boolean}
   */
  didAbort_: false,

  /**
   * Caches the individually selected synced data types. This is used to
   * be able to restore the selections after checking and unchecking Sync All.
   * @private {?Object}
   */
  cachedSyncPrefs_: null,

  /** @override */
  created: function() {
    this.browserProxy_ = settings.SyncBrowserProxyImpl.getInstance();
  },

  /** @override */
  attached: function() {
    this.addWebUIListener(
        'page-status-changed', this.handlePageStatusChanged_.bind(this));
    this.addWebUIListener(
        'sync-prefs-changed', this.handleSyncPrefsChanged_.bind(this));

    if (settings.getCurrentRoute() == settings.routes.SYNC)
      this.onNavigateToPage_();
  },

  /** @override */
  detached: function() {
    if (settings.getCurrentRoute() == settings.routes.SYNC)
      this.onNavigateAwayFromPage_();

    if (this.beforeunloadCallback_) {
      window.removeEventListener('beforeunload', this.beforeunloadCallback_);
      this.beforeunloadCallback_ = null;
    }
  },

  /**
   * @return {boolean}
   * @private
   */
  computeSyncSectionDisabled_() {
    return this.unifiedConsentEnabled && !this.syncStatus.signedIn;
  },

  /** @protected */
  currentRouteChanged: function() {
    if (settings.getCurrentRoute() == settings.routes.SYNC)
      this.onNavigateToPage_();
    else
      this.onNavigateAwayFromPage_();
  },

  /**
   * @param {!settings.PageStatus} expectedPageStatus
   * @return {boolean}
   * @private
   */
  isStatus_: function(expectedPageStatus) {
    return expectedPageStatus == this.pageStatus_;
  },

  /** @private */
  onNavigateToPage_: function() {
    assert(settings.getCurrentRoute() == settings.routes.SYNC);

    if (this.beforeunloadCallback_)
      return;

    // Display loading page until the settings have been retrieved.
    this.pageStatus_ = settings.PageStatus.SPINNER;

    this.browserProxy_.didNavigateToSyncPage();

    this.beforeunloadCallback_ = this.onNavigateAwayFromPage_.bind(this);
    window.addEventListener('beforeunload', this.beforeunloadCallback_);
  },

  /** @private */
  onNavigateAwayFromPage_: function() {
    if (!this.beforeunloadCallback_)
      return;

    // Reset the status to CONFIGURE such that the searching algorithm can
    // search useful content when the page is not visible to the user.
    this.pageStatus_ = settings.PageStatus.CONFIGURE;

    this.browserProxy_.didNavigateAwayFromSyncPage(this.didAbort_);
    this.didAbort_ = false;

    window.removeEventListener('beforeunload', this.beforeunloadCallback_);
    this.beforeunloadCallback_ = null;
  },

  /**
   * Handler for when the sync preferences are updated.
   * @private
   */
  handleSyncPrefsChanged_: function(syncPrefs) {
    this.syncPrefs = syncPrefs;
    this.pageStatus_ = settings.PageStatus.CONFIGURE;

    // If autofill is not registered or synced, force Payments integration off.
    if (!this.syncPrefs.autofillRegistered || !this.syncPrefs.autofillSynced)
      this.set('syncPrefs.paymentsIntegrationEnabled', false);

    // Hide the new passphrase box if the sync data has been encrypted.
    if (this.syncPrefs.encryptAllData)
      this.creatingNewPassphrase_ = false;

    // Focus the password input box if password is needed to start sync.
    if (this.syncPrefs.passphraseRequired) {
      // Wait for the dom-if templates to render and subpage to become visible.
      listenOnce(document, 'show-container', () => {
        const input = /** @type {!PaperInputElement} */ (
            this.$$('#existingPassphraseInput'));
        input.focus();
      });
    }
  },

  /**
   * Handler for when the sync all data types checkbox is changed.
   * @param {!Event} event
   * @private
   */
  onSyncAllDataTypesChanged_: function(event) {
    if (event.target.checked) {
      this.set('syncPrefs.syncAllDataTypes', true);

      // Cache the previously selected preference before checking every box.
      this.cachedSyncPrefs_ = {};
      for (const dataType of SyncPrefsIndividualDataTypes) {
        // These are all booleans, so this shallow copy is sufficient.
        this.cachedSyncPrefs_[dataType] = this.syncPrefs[dataType];

        this.set(['syncPrefs', dataType], true);
      }
    } else if (this.cachedSyncPrefs_) {
      // Restore the previously selected preference.
      for (const dataType of SyncPrefsIndividualDataTypes) {
        this.set(['syncPrefs', dataType], this.cachedSyncPrefs_[dataType]);
      }
    }

    this.onSingleSyncDataTypeChanged_();
  },

  /**
   * Handler for when any sync data type checkbox is changed (except autofill).
   * @private
   */
  onSingleSyncDataTypeChanged_: function() {
    assert(this.syncPrefs);
    this.browserProxy_.setSyncDatatypes(this.syncPrefs)
        .then(this.handlePageStatusChanged_.bind(this));
  },

  /** @private */
  onActivityControlsTap_: function() {
    this.browserProxy_.openActivityControlsUrl();
  },

  /**
   * Handler for when the autofill data type checkbox is changed.
   * @private
   */
  onAutofillDataTypeChanged_: function() {
    this.set(
        'syncPrefs.paymentsIntegrationEnabled', this.syncPrefs.autofillSynced);

    this.onSingleSyncDataTypeChanged_();
  },

  /**
   * @param {string} passphrase The passphrase input field value
   * @param {string} confirmation The passphrase confirmation input field value.
   * @return {boolean} Whether the passphrase save button should be enabled.
   * @private
   */
  isSaveNewPassphraseEnabled_: function(passphrase, confirmation) {
    return passphrase !== '' && confirmation !== '';
  },

  /**
   * Sends the newly created custom sync passphrase to the browser.
   * @private
   * @param {!Event} e
   */
  onSaveNewPassphraseTap_: function(e) {
    assert(this.creatingNewPassphrase_);

    // Ignore events on irrelevant elements or with irrelevant keys.
    if (e.target.tagName != 'PAPER-BUTTON' && e.target.tagName != 'PAPER-INPUT')
      return;
    if (e.type == 'keypress' && e.key != 'Enter')
      return;

    // If a new password has been entered but it is invalid, do not send the
    // sync state to the API.
    if (!this.validateCreatedPassphrases_())
      return;

    this.syncPrefs.encryptAllData = true;
    this.syncPrefs.setNewPassphrase = true;
    this.syncPrefs.passphrase = this.passphrase_;

    this.browserProxy_.setSyncEncryption(this.syncPrefs)
        .then(this.handlePageStatusChanged_.bind(this));
  },

  /**
   * Sends the user-entered existing password to re-enable sync.
   * @private
   * @param {!Event} e
   */
  onSubmitExistingPassphraseTap_: function(e) {
    if (e.type == 'keypress' && e.key != 'Enter')
      return;

    assert(!this.creatingNewPassphrase_);

    this.syncPrefs.setNewPassphrase = false;

    this.syncPrefs.passphrase = this.existingPassphrase_;
    this.existingPassphrase_ = '';

    this.browserProxy_.setSyncEncryption(this.syncPrefs)
        .then(this.handlePageStatusChanged_.bind(this));
  },

  /**
   * Called when the page status updates.
   * @param {!settings.PageStatus} pageStatus
   * @private
   */
  handlePageStatusChanged_: function(pageStatus) {
    switch (pageStatus) {
      case settings.PageStatus.SPINNER:
      case settings.PageStatus.TIMEOUT:
      case settings.PageStatus.CONFIGURE:
        this.pageStatus_ = pageStatus;
        return;
      case settings.PageStatus.DONE:
        if (settings.getCurrentRoute() == settings.routes.SYNC)
          settings.navigateTo(settings.routes.PEOPLE);
        return;
      case settings.PageStatus.PASSPHRASE_FAILED:
        if (this.pageStatus_ == this.pages_.CONFIGURE && this.syncPrefs &&
            this.syncPrefs.passphraseRequired) {
          this.$$('#existingPassphraseInput').invalid = true;
        }
        return;
    }

    assertNotReached();
  },

  /**
   * Called when the encryption
   * @private
   */
  onEncryptionRadioSelectionChanged_: function(event) {
    this.creatingNewPassphrase_ =
        event.target.selected == RadioButtonNames.ENCRYPT_WITH_PASSPHRASE;
  },

  /**
   * Computed binding returning the selected encryption radio button.
   * @private
   */
  selectedEncryptionRadio_: function() {
    return this.syncPrefs.encryptAllData || this.creatingNewPassphrase_ ?
        RadioButtonNames.ENCRYPT_WITH_PASSPHRASE :
        RadioButtonNames.ENCRYPT_WITH_GOOGLE;
  },

  /**
   * Computed binding returning text of the prompt for entering the passphrase.
   * @private
   */
  enterPassphrasePrompt_: function() {
    if (this.syncPrefs && this.syncPrefs.passphraseTypeIsCustom)
      return this.syncPrefs.enterPassphraseBody;

    return this.syncPrefs.enterGooglePassphraseBody;
  },

  /**
   * @param {boolean} syncAllDataTypes
   * @param {boolean} enforced
   * @return {boolean} Whether the sync checkbox should be disabled.
   */
  shouldSyncCheckboxBeDisabled_: function(syncAllDataTypes, enforced) {
    return syncAllDataTypes || enforced;
  },

  /**
   * @param {boolean} syncAllDataTypes
   * @param {boolean} autofillSynced
   * @return {boolean} Whether the sync checkbox should be disabled.
   */
  shouldPaymentsCheckboxBeDisabled_: function(
      syncAllDataTypes, autofillSynced) {
    return syncAllDataTypes || !autofillSynced;
  },

  /**
   * Checks the supplied passphrases to ensure that they are not empty and that
   * they match each other. Additionally, displays error UI if they are invalid.
   * @return {boolean} Whether the check was successful (i.e., that the
   *     passphrases were valid).
   * @private
   */
  validateCreatedPassphrases_: function() {
    const emptyPassphrase = !this.passphrase_;
    const mismatchedPassphrase = this.passphrase_ != this.confirmation_;

    this.$$('#passphraseInput').invalid = emptyPassphrase;
    this.$$('#passphraseConfirmationInput').invalid =
        !emptyPassphrase && mismatchedPassphrase;

    return !emptyPassphrase && !mismatchedPassphrase;
  },

  /**
   * @param {!Event} event
   * @private
   */
  onLearnMoreTap_: function(event) {
    if (event.target.tagName == 'A') {
      // Stop the propagation of events, so that clicking on links inside
      // checkboxes or radio buttons won't change the value.
      event.stopPropagation();
    }
  },

  /** @private */
  onCancelSyncClick_: function() {
    this.didAbort_ = true;
    settings.navigateTo(settings.routes.BASIC);
  },

  /** @private */
  onSyncStatusChanged_: function() {
    this.syncSectionOpened_ = !!this.syncStatus.signedIn;
  },

  /**
   * Toggles the expand button within the element being listened to.
   * @param {!Event} e
   * @private
   */
  toggleExpandButton_: function(e) {
    // The expand button handles toggling itself.
    const expandButtonTag = 'CR-EXPAND-BUTTON';
    if (e.target.tagName == expandButtonTag)
      return;

    if (!e.currentTarget.hasAttribute('actionable'))
      return;

    /** @type {!CrExpandButtonElement} */
    const expandButton = e.currentTarget.querySelector(expandButtonTag);
    assert(expandButton);
    expandButton.expanded = !expandButton.expanded;
  },

  /**
   * When unified-consent enabled, the non-toggle items on the bottom of sync
   * section should be wrapped with 'list-frame' in order to be indented
   * correctly.
   * @return {string}
   * @private
   */
  getListFrameClass_: function() {
    return this.unifiedConsentEnabled ? 'list-frame' : '';
  },

  /**
   * When unified-consent enabled, the non-toggle items on the bottom of sync
   * section will be wrapped with 'list-frame', and should have the 'list-item'
   * instead of 'settings-box' in order to be indented correctly.
   * @return {string}
   * @private
   */
  getListItemClass_: function() {
    return this.unifiedConsentEnabled ? 'list-item' : 'settings-box';
  },

  // <if expr="not chromeos">
  /**
   * @return {boolean}
   * @private
   */
  shouldShowSyncAccountControl_: function() {
    return !!this.diceEnabled && !!this.unifiedConsentEnabled &&
        !!this.syncStatus.syncSystemEnabled && !!this.syncStatus.signinAllowed;
  },
  // </if>
});

})();
