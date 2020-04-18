// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

cr.exportPath('extensions');

cr.define('extensions', function() {
  /** @interface */
  class ToolbarDelegate {
    /**
     * Toggles whether or not the profile is in developer mode.
     * @param {boolean} inDevMode
     */
    setProfileInDevMode(inDevMode) {}

    /**
     * Opens the dialog to load unpacked extensions.
     * @return {!Promise}
     */
    loadUnpacked() {}

    /**
     * Updates all extensions.
     * @return {!Promise}
     */
    updateAllExtensions() {}
  }

  const Toolbar = Polymer({
    is: 'extensions-toolbar',

    properties: {
      /** @type {extensions.ToolbarDelegate} */
      delegate: Object,

      inDevMode: {
        type: Boolean,
        value: false,
        observer: 'onInDevModeChanged_',
      },

      devModeControlledByPolicy: Boolean,

      isSupervised: Boolean,

      // <if expr="chromeos">
      kioskEnabled: Boolean,
      // </if>

      canLoadUnpacked: Boolean,

      /** @private */
      expanded_: Boolean,

      /**
       * Text to display in update toast
       * @private
       */
      toastLabel_: String,

      /**
       * Prevents initiating update while update is in progress.
       * @private
       */
      isUpdating_: {type: Boolean, value: false}
    },

    behaviors: [I18nBehavior],

    hostAttributes: {
      role: 'banner',
    },

    /** @override */
    detached: function() {
      const openToastElement = this.$$('cr-toast[open]');
      if (openToastElement)
        openToastElement.hide();
    },

    /**
     * @return {boolean}
     * @private
     */
    shouldDisableDevMode_: function() {
      return this.devModeControlledByPolicy || this.isSupervised;
    },

    /**
     * @param {!CustomEvent} e
     * @private
     */
    onDevModeToggleChange_: function(e) {
      this.delegate.setProfileInDevMode(/** @type {boolean} */ (e.detail));
      chrome.metricsPrivate.recordUserAction(
          'Options_ToggleDeveloperMode_' + (e.detail ? 'Enabled' : 'Disabled'));
    },

    /**
     * @param {boolean} current
     * @param {boolean} previous
     * @private
     */
    onInDevModeChanged_: function(current, previous) {
      const drawer = this.$.devDrawer;
      if (this.inDevMode) {
        if (drawer.hidden) {
          drawer.hidden = false;
          // Requesting the offsetTop will cause a reflow (to account for
          // hidden).
          /** @suppress {suspiciousCode} */ drawer.offsetTop;
        }
      } else {
        if (previous == undefined) {
          drawer.hidden = true;
          return;
        }

        listenOnce(drawer, 'transitionend', e => {
          if (!this.inDevMode)
            drawer.hidden = true;
        });
      }
      this.expanded_ = !this.expanded_;
    },

    /** @private */
    onLoadUnpackedTap_: function() {
      this.delegate.loadUnpacked().catch(loadError => {
        this.fire('load-error', loadError);
      });
      chrome.metricsPrivate.recordUserAction('Options_LoadUnpackedExtension');
    },

    /** @private */
    onPackTap_: function() {
      this.fire('pack-tap');
      chrome.metricsPrivate.recordUserAction('Options_PackExtension');
    },

    // <if expr="chromeos">
    /** @private */
    onKioskTap_: function() {
      this.fire('kiosk-tap');
    },
    // </if>

    /** @private */
    onUpdateNowTap_: function() {
      // If already updating, do not initiate another update.
      if (this.isUpdating_)
        return;

      this.isUpdating_ = true;
      const toastElement = this.$$('cr-toast');
      this.toastLabel_ = this.i18n('toolbarUpdatingToast');

      // Keep the toast open indefinitely.
      toastElement.duration = 0;
      toastElement.show();
      this.delegate.updateAllExtensions()
          .then(() => {
            Polymer.IronA11yAnnouncer.requestAvailability();
            const doneText = this.i18n('toolbarUpdateDone');
            this.fire('iron-announce', {text: doneText});
            this.toastLabel_ = doneText;
            toastElement.show(3000);
            this.isUpdating_ = false;
          })
          .catch(function() {
            toastElement.hide();
            this.isUpdating_ = false;
          });
    },
  });

  return {
    Toolbar: Toolbar,
    ToolbarDelegate: ToolbarDelegate,
  };
});
