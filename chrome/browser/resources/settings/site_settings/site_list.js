// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * @fileoverview
 * 'site-list' shows a list of Allowed and Blocked sites for a given
 * category.
 */
Polymer({

  is: 'site-list',

  behaviors: [SiteSettingsBehavior, WebUIListenerBehavior],

  properties: {
    /** @private */
    enableSiteSettings_: {
      type: Boolean,
      value: function() {
        return loadTimeData.getBoolean('enableSiteSettings');
      },
    },

    /**
     * Some content types (like Location) do not allow the user to manually
     * edit the exception list from within Settings.
     * @private
     */
    readOnlyList: {
      type: Boolean,
      value: false,
    },

    /**
     * The site serving as the model for the currently open action menu.
     * @private {?SiteException}
     */
    actionMenuSite_: Object,

    /**
     * Whether the "edit exception" dialog should be shown.
     * @private
     */
    showEditExceptionDialog_: Boolean,

    /**
     * Array of sites to display in the widget.
     * @type {!Array<SiteException>}
     */
    sites: {
      type: Array,
      value: function() {
        return [];
      },
    },

    /**
     * The type of category this widget is displaying data for. Normally
     * either 'allow' or 'block', representing which sites are allowed or
     * blocked respectively.
     */
    categorySubtype: {
      type: String,
      value: settings.INVALID_CATEGORY_SUBTYPE,
    },

    /**
     * Whether to show the Allow action in the action menu.
     * @private
     */
    showAllowAction_: Boolean,

    /**
     * Whether to show the Block action in the action menu.
     * @private
     */
    showBlockAction_: Boolean,

    /**
     * Whether to show the 'Clear on exit' action in the action
     * menu.
     * @private
     */
    showSessionOnlyAction_: Boolean,

    /**
     * All possible actions in the action menu.
     * @private
     */
    actions_: {
      readOnly: true,
      type: Object,
      values: {
        ALLOW: 'Allow',
        BLOCK: 'Block',
        RESET: 'Reset',
        SESSION_ONLY: 'SessionOnly',
      }
    },
  },

  /**
   * The element to return focus to, when the currently active dialog is closed.
   * @private {?HTMLElement}
   */
  activeDialogAnchor_: null,

  observers: ['configureWidget_(category, categorySubtype)'],

  /** @override */
  ready: function() {
    this.addWebUIListener(
        'contentSettingSitePermissionChanged',
        this.siteWithinCategoryChanged_.bind(this));
    this.addWebUIListener(
        'onIncognitoStatusChanged', this.onIncognitoStatusChanged_.bind(this));
  },

  /**
   * Called when a site changes permission.
   * @param {string} category The category of the site that changed.
   * @param {string} site The site that changed.
   * @private
   */
  siteWithinCategoryChanged_: function(category, site) {
    if (category == this.category)
      this.configureWidget_();
  },

  /**
   * Called for each site list when incognito is enabled or disabled. Only
   * called on change (opening N incognito windows only fires one message).
   * Another message is sent when the *last* incognito window closes.
   * @private
   */
  onIncognitoStatusChanged_: function() {
    // The SESSION_ONLY list won't have any incognito exceptions. (Minor
    // optimization, not required).
    if (this.categorySubtype == settings.ContentSetting.SESSION_ONLY)
      return;

    // A change notification is not sent for each site. So we repopulate the
    // whole list when the incognito profile is created or destroyed.
    this.populateList_();
  },

  /**
   * Configures the action menu, visibility of the widget and shows the list.
   * @private
   */
  configureWidget_: function() {
    // The observer for All Sites fires before the attached/ready event, so
    // initialize this here.
    if (this.browserProxy_ === undefined) {
      this.browserProxy_ =
          settings.SiteSettingsPrefsBrowserProxyImpl.getInstance();
    }

    this.setUpActionMenu_();
    this.populateList_();

    // The Session permissions are only for cookies.
    if (this.categorySubtype == settings.ContentSetting.SESSION_ONLY) {
      this.$.category.hidden =
          this.category != settings.ContentSettingsTypes.COOKIES;
    }
  },

  /**
   * Whether there are any site exceptions added for this content setting.
   * @return {boolean}
   * @private
   */
  hasSites_: function() {
    return !!this.sites.length;
  },

  /**
   * @param {!SiteException} exception The content setting exception.
   * @param {boolean} readOnlyList Whether the site exception list is read-only.
   * @return {boolean}
   * @private
   */
  shouldHideResetButton_: function(exception, readOnlyList) {
    return exception.enforcement ==
        chrome.settingsPrivate.Enforcement.ENFORCED ||
        !(readOnlyList || !!exception.embeddingOrigin);
  },

  /**
   * @param {!SiteException} exception The content setting exception.
   * @param {boolean} readOnlyList Whether the site exception list is read-only.
   * @return {boolean}
   * @private
   */
  shouldHideActionMenu_: function(exception, readOnlyList) {
    return exception.enforcement ==
        chrome.settingsPrivate.Enforcement.ENFORCED ||
        readOnlyList || !!exception.embeddingOrigin;
  },

  /**
   * A handler for the Add Site button.
   * @param {!Event} e
   * @private
   */
  onAddSiteTap_: function(e) {
    assert(!this.readOnlyList);
    e.preventDefault();
    const dialog = document.createElement('add-site-dialog');
    dialog.category = this.category;
    dialog.contentSetting = this.categorySubtype;
    this.shadowRoot.appendChild(dialog);

    dialog.open(this.categorySubtype);

    dialog.addEventListener('close', () => {
      cr.ui.focusWithoutInk(assert(this.$.addSite));
      dialog.remove();
    });
  },

  /**
   * Populate the sites list for display.
   * @private
   */
  populateList_: function() {
    this.browserProxy_.getExceptionList(this.category).then(exceptionList => {
      this.processExceptions_([exceptionList]);
      this.closeActionMenu_();
    });
  },

  /**
   * Process the exception list returned from the native layer.
   * @param {!Array<!Array<RawSiteException>>} data List of sites (exceptions)
   *     to process.
   * @private
   */
  processExceptions_: function(data) {
    const sites = /** @type {!Array<RawSiteException>} */ ([]);
    for (let i = 0; i < data.length; ++i) {
      const exceptionList = data[i];
      for (let k = 0; k < exceptionList.length; ++k) {
        if (exceptionList[k].setting == settings.ContentSetting.DEFAULT ||
            exceptionList[k].setting != this.categorySubtype) {
          continue;
        }

        sites.push(exceptionList[k]);
      }
    }
    this.sites = this.toSiteArray_(sites);
  },

  /**
   * Converts a list of exceptions received from the C++ handler to
   * full SiteException objects.
   * @param {!Array<RawSiteException>} sites A list of sites to convert.
   * @return {!Array<SiteException>} A list of full SiteExceptions.
   * @private
   */
  toSiteArray_: function(sites) {
    const results = /** @type {!Array<SiteException>} */ ([]);
    let lastOrigin = '';
    let lastEmbeddingOrigin = '';
    for (let i = 0; i < sites.length; ++i) {
      /** @type {!SiteException} */
      const siteException = this.expandSiteException(sites[i]);

      results.push(siteException);
      lastOrigin = siteException.origin;
      lastEmbeddingOrigin = siteException.embeddingOrigin;
    }
    return results;
  },

  /**
   * Set up the values to use for the action menu.
   * @private
   */
  setUpActionMenu_: function() {
    this.showAllowAction_ =
        this.categorySubtype != settings.ContentSetting.ALLOW;
    this.showBlockAction_ =
        this.categorySubtype != settings.ContentSetting.BLOCK;
    this.showSessionOnlyAction_ =
        this.categorySubtype != settings.ContentSetting.SESSION_ONLY &&
        this.category == settings.ContentSettingsTypes.COOKIES;
  },

  /**
   * @return {boolean} Whether to show the "Session Only" menu item for the
   *     currently active site.
   * @private
   */
  showSessionOnlyActionForSite_: function() {
    // It makes no sense to show "clear on exit" for exceptions that only apply
    // to incognito. It gives the impression that they might under some
    // circumstances not be cleared on exit, which isn't true.
    if (!this.actionMenuSite_ || this.actionMenuSite_.incognito)
      return false;

    return this.showSessionOnlyAction_;
  },

  /**
   * A handler for selecting a site (by clicking on the origin).
   * @param {!{model: !{item: !SiteException}}} event
   * @private
   */
  onOriginTap_: function(event) {
    if (!this.enableSiteSettings_)
      return;
    settings.navigateTo(
        settings.routes.SITE_SETTINGS_SITE_DETAILS,
        new URLSearchParams('site=' + event.model.item.origin));
  },

  /**
   * @param {?SiteException} site
   * @private
   */
  resetPermissionForOrigin_: function(site) {
    assert(site);
    this.browserProxy.resetCategoryPermissionForPattern(
        site.origin, site.embeddingOrigin, this.category, site.incognito);
  },

  /**
   * @param {!settings.ContentSetting} contentSetting
   * @private
   */
  setContentSettingForActionMenuSite_: function(contentSetting) {
    assert(this.actionMenuSite_);
    this.browserProxy.setCategoryPermissionForPattern(
        this.actionMenuSite_.origin, this.actionMenuSite_.embeddingOrigin,
        this.category, contentSetting, this.actionMenuSite_.incognito);
  },

  /** @private */
  onAllowTap_: function() {
    this.setContentSettingForActionMenuSite_(settings.ContentSetting.ALLOW);
    this.closeActionMenu_();
  },

  /** @private */
  onBlockTap_: function() {
    this.setContentSettingForActionMenuSite_(settings.ContentSetting.BLOCK);
    this.closeActionMenu_();
  },

  /** @private */
  onSessionOnlyTap_: function() {
    this.setContentSettingForActionMenuSite_(
        settings.ContentSetting.SESSION_ONLY);
    this.closeActionMenu_();
  },

  /** @private */
  onEditTap_: function() {
    // Close action menu without resetting |this.actionMenuSite_| since it is
    // bound to the dialog.
    /** @type {!CrActionMenuElement} */ (this.$$('cr-action-menu')).close();
    this.showEditExceptionDialog_ = true;
  },

  /** @private */
  onEditExceptionDialogClosed_: function() {
    this.showEditExceptionDialog_ = false;
    this.actionMenuSite_ = null;
    if (this.activeDialogAnchor_) {
      this.activeDialogAnchor_.focus();
      this.activeDialogAnchor_ = null;
    }
  },

  /** @private */
  onResetTap_: function() {
    this.resetPermissionForOrigin_(this.actionMenuSite_);
    this.closeActionMenu_();
  },

  /**
   * Returns the appropriate site description to display. This can, for example,
   * be blank, an 'embedded on <site>' or 'Current incognito session' (or a
   * mix of the last two).
   * @param {SiteException} item The site exception entry.
   * @return {string} The site description.
   */
  computeSiteDescription_: function(item) {
    let displayName = '';
    if (item.embeddingOrigin) {
      displayName = loadTimeData.getStringF(
          'embeddedOnHost', this.sanitizePort(item.embeddingOrigin));
    } else if (this.category == settings.ContentSettingsTypes.GEOLOCATION) {
      displayName = loadTimeData.getString('embeddedOnAnyHost');
    }

    if (item.incognito) {
      if (displayName.length > 0)
        return loadTimeData.getStringF('embeddedIncognitoSite', displayName);
      return loadTimeData.getString('incognitoSite');
    }
    return displayName;
  },

  /**
   * @param {!{model: !{item: !SiteException}}} e
   * @private
   */
  onResetButtonTap_: function(e) {
    this.resetPermissionForOrigin_(e.model.item);
  },

  /**
   * @param {!{model: !{item: !SiteException}}} e
   * @private
   */
  onShowActionMenuTap_: function(e) {
    this.activeDialogAnchor_ = /** @type {!HTMLElement} */ (
        Polymer.dom(/** @type {!Event} */ (e)).localTarget);

    this.actionMenuSite_ = e.model.item;
    /** @type {!CrActionMenuElement} */ (this.$$('cr-action-menu'))
        .showAt(this.activeDialogAnchor_);
  },

  /** @private */
  closeActionMenu_: function() {
    this.actionMenuSite_ = null;
    this.activeDialogAnchor_ = null;
    const actionMenu =
        /** @type {!CrActionMenuElement} */ (this.$$('cr-action-menu'));
    if (actionMenu.open)
      actionMenu.close();
  },
});
