// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * @fileoverview 'settings-startup-urls-page' is the settings page
 * containing the urls that will be opened when chrome is started.
 */

Polymer({
  is: 'settings-startup-urls-page',

  behaviors: [CrScrollableBehavior, WebUIListenerBehavior],

  properties: {
    prefs: Object,

    /**
     * Pages to load upon browser startup.
     * @private {!Array<!StartupPageInfo>}
     */
    startupPages_: Array,

    /** @private */
    showStartupUrlDialog_: Boolean,

    /** @private {?StartupPageInfo} */
    startupUrlDialogModel_: Object,

    /** @private {Object}*/
    lastFocused_: Object,
  },

  /** @private {?settings.StartupUrlsPageBrowserProxy} */
  browserProxy_: null,

  /**
   * The element to return focus to, when the startup-url-dialog is closed.
   * @private {?HTMLElement}
   */
  startupUrlDialogAnchor_: null,

  /** @override */
  attached: function() {
    this.browserProxy_ = settings.StartupUrlsPageBrowserProxyImpl.getInstance();
    this.addWebUIListener('update-startup-pages', startupPages => {
      // If an "edit" URL dialog was open, close it, because the underlying page
      // might have just been removed (and model indices have changed anyway).
      if (this.startupUrlDialogModel_)
        this.destroyUrlDialog_();
      this.startupPages_ = startupPages;
      this.updateScrollableContents();
    });
    this.browserProxy_.loadStartupPages();

    this.addEventListener(settings.EDIT_STARTUP_URL_EVENT, event => {
      this.startupUrlDialogModel_ = event.detail.model;
      this.startupUrlDialogAnchor_ = event.detail.anchor;
      this.showStartupUrlDialog_ = true;
      event.stopPropagation();
    });
  },

  /**
   * @param {!Event} e
   * @private
   */
  onAddPageTap_: function(e) {
    e.preventDefault();
    this.showStartupUrlDialog_ = true;
    this.startupUrlDialogAnchor_ =
        /** @type {!HTMLElement} */ (this.$$('#addPage a[is=action-link]'));
  },

  /** @private */
  destroyUrlDialog_: function() {
    this.showStartupUrlDialog_ = false;
    this.startupUrlDialogModel_ = null;
    if (this.startupUrlDialogAnchor_) {
      cr.ui.focusWithoutInk(assert(this.startupUrlDialogAnchor_));
      this.startupUrlDialogAnchor_ = null;
    }
  },

  /** @private */
  onUseCurrentPagesTap_: function() {
    this.browserProxy_.useCurrentPages();
  },

  /**
   * @return {boolean} Whether "Add new page" and "Use current pages" are
   *     allowed.
   * @private
   */
  shouldAllowUrlsEdit_: function() {
    return this.get('prefs.session.startup_urls.enforcement') !=
        chrome.settingsPrivate.Enforcement.ENFORCED;
  },
});
