// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * @fileoverview 'settings-import-data-dialog' is a component for importing
 * bookmarks and other data from other sources.
 */
Polymer({
  is: 'settings-import-data-dialog',

  behaviors: [I18nBehavior, WebUIListenerBehavior, PrefsBehavior],

  properties: {
    /** @private {!Array<!settings.BrowserProfile>} */
    browserProfiles_: Array,

    /** @private {!settings.BrowserProfile} */
    selected_: Object,

    /**
     * Whether none of the import data categories is selected.
     * @private
     */
    noImportDataTypeSelected_: {
      type: Boolean,
      value: false,
    },

    /** @private */
    importStatus_: {
      type: String,
      value: settings.ImportDataStatus.INITIAL,
    },

    /**
     * Mirroring the enum so that it can be used from HTML bindings.
     * @private
     */
    importStatusEnum_: {
      type: Object,
      value: settings.ImportDataStatus,
    },
  },

  observers: [
    'prefsChanged_(selected_, prefs.*)',
  ],

  /** @private {?settings.ImportDataBrowserProxy} */
  browserProxy_: null,

  /** @override */
  attached: function() {
    this.browserProxy_ = settings.ImportDataBrowserProxyImpl.getInstance();
    this.browserProxy_.initializeImportDialog().then(data => {
      this.browserProfiles_ = data;
      this.selected_ = this.browserProfiles_[0];

      // Show the dialog only after the browser profiles data is populated
      // to avoid UI flicker.
      this.$.dialog.showModal();
    });

    this.addWebUIListener('import-data-status-changed', importStatus => {
      this.importStatus_ = importStatus;
      if (this.hasImportStatus_(settings.ImportDataStatus.FAILED))
        this.closeDialog_();
    });
  },

  /** @private */
  prefsChanged_: function() {
    this.noImportDataTypeSelected_ =
        !(this.getPref('import_dialog_history').value &&
          this.selected_.history) &&
        !(this.getPref('import_dialog_bookmarks').value &&
          this.selected_.favorites) &&
        !(this.getPref('import_dialog_saved_passwords').value &&
          this.selected_.passwords) &&
        !(this.getPref('import_dialog_search_engine').value &&
          this.selected_.search) &&
        !(this.getPref('import_dialog_autofill_form_data').value &&
          this.selected_.autofillFormData);
  },

  /**
   * @param {!settings.ImportDataStatus} status
   * @return {boolean} Whether |status| is the current status.
   * @private
   */
  hasImportStatus_: function(status) {
    return this.importStatus_ == status;
  },

  /** @private */
  isImportFromFileSelected_: function() {
    // The last entry in |browserProfiles_| always refers to dummy profile for
    // importing from a bookmarks file.
    return this.selected_.index == this.browserProfiles_.length - 1;
  },

  /**
   * @return {string}
   * @private
   */
  getActionButtonText_: function() {
    return this.i18n(
        this.isImportFromFileSelected_() ? 'importChooseFile' : 'importCommit');
  },

  /** @private */
  onBrowserProfileSelectionChange_: function() {
    this.selected_ = this.browserProfiles_[this.$.browserSelect.selectedIndex];
  },

  /** @private */
  onActionButtonTap_: function() {
    if (this.isImportFromFileSelected_())
      this.browserProxy_.importFromBookmarksFile();
    else
      this.browserProxy_.importData(this.$.browserSelect.selectedIndex);
  },

  /** @private */
  closeDialog_: function() {
    this.$.dialog.close();
  },

  /**
   * @return {boolean} Whether the import button should be disabled.
   * @private
   */
  shouldDisableImport_: function() {
    return this.hasImportStatus_(settings.ImportDataStatus.IN_PROGRESS) ||
        this.noImportDataTypeSelected_;
  },
});
