// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * @fileoverview
 * 'site-details' show the details (permissions and usage) for a given origin
 * under Site Settings.
 */
Polymer({
  is: 'site-details',

  behaviors: [
    SiteSettingsBehavior, settings.RouteObserverBehavior, WebUIListenerBehavior
  ],

  properties: {
    /**
     * The origin that this widget is showing details for.
     * @private
     */
    origin: {
      type: String,
      observer: 'onOriginChanged_',
    },

    /**
     * Use the string representing the origin or extension name as the page
     * title of the settings-subpage parent.
     */
    pageTitle: {
      type: String,
      notify: true,
    },

    /**
     * The amount of data stored for the origin.
     * @private
     */
    storedData_: {
      type: String,
      value: '',
    },

    /** @private */
    enableSiteSettings_: {
      type: Boolean,
      value: function() {
        return loadTimeData.getBoolean('enableSiteSettings');
      },
    },

    /** @private */
    enableSafeBrowsingSubresourceFilter_: {
      type: Boolean,
      value: function() {
        return loadTimeData.getBoolean('enableSafeBrowsingSubresourceFilter');
      },
    },

    /** @private */
    enableSoundContentSetting_: {
      type: Boolean,
      value: function() {
        return loadTimeData.getBoolean('enableSoundContentSetting');
      },
    },

    /** @private */
    enableClipboardContentSetting_: {
      type: Boolean,
      value: function() {
        return loadTimeData.getBoolean('enableClipboardContentSetting');
      },
    },

    /** @private */
    enableSensorsContentSetting_: {
      type: Boolean,
      readOnly: true,
      value: function() {
        return loadTimeData.getBoolean('enableSensorsContentSetting');
      },
    },

    /** @private */
    enablePaymentHandlerContentSetting_: {
      type: Boolean,
      value: function() {
        return loadTimeData.getBoolean('enablePaymentHandlerContentSetting');
      },
    },

    /**
     * The type of storage for the origin.
     * @private
     */
    storageType_: Number,
  },

  listeners: {
    'usage-deleted': 'onUsageDeleted_',
  },

  /** @override */
  attached: function() {
    this.addWebUIListener(
        'contentSettingSitePermissionChanged',
        this.onPermissionChanged_.bind(this));

    // <if expr="chromeos">
    this.addWebUIListener(
        'prefEnableDrmChanged', this.prefEnableDrmChanged_.bind(this));
    // </if>
  },

  /** @override */
  ready: function() {
    this.ContentSettingsTypes = settings.ContentSettingsTypes;
  },

  /**
   * settings.RouteObserverBehavior
   * @param {!settings.Route} route
   * @protected
   */
  currentRouteChanged: function(route) {
    const site = settings.getQueryParameters().get('site');
    if (!site)
      return;
    this.origin = site;
  },

  /**
   * Handler for when the origin changes.
   * @private
   */
  onOriginChanged_: function() {
    this.browserProxy.isOriginValid(this.origin).then((valid) => {
      if (!valid) {
        settings.navigateToPreviousRoute();
      } else {
        if (this.enableSiteSettings_)
          this.$.usageApi.fetchUsageTotal(this.toUrl(this.origin).hostname);

        this.updatePermissions_(this.getCategoryList_());
      }
    });
  },

  /**
   * Called when a site within a category has been changed.
   * @param {!settings.ContentSettingsTypes} category The category that changed.
   * @param {string} origin The origin of the site that changed.
   * @param {string} embeddingOrigin The embedding origin of the site that
   *     changed.
   * @private
   */
  onPermissionChanged_: function(category, origin, embeddingOrigin) {
    if (this.origin === undefined || this.origin == '' ||
        origin === undefined || origin == '') {
      return;
    }
    if (!this.getCategoryList_().includes(category))
      return;

    // Site details currently doesn't support embedded origins, so ignore it and
    // just check whether the origins are the same.
    if (this.toUrl(origin).origin == this.toUrl(this.origin).origin)
      this.updatePermissions_([category]);
  },

  // <if expr="chromeos">
  prefEnableDrmChanged_: function() {
    this.updatePermissions_([settings.ContentSettingsTypes.PROTECTED_CONTENT]);
  },
  // </if>

  /**
   * Retrieves the permissions listed in |categoryList| from the backend for
   * |this.origin|.
   * @param {!Array<!settings.ContentSettingsTypes>} categoryList The list of
   *     categories to update permissions for.
   * @private
   */
  updatePermissions_: function(categoryList) {
    const permissionsMap =
        /** @type {!Object<!settings.ContentSettingsTypes,
         *         !SiteDetailsPermissionElement>} */
        (Array.prototype.reduce.call(
            this.root.querySelectorAll('site-details-permission'),
            (map, element) => {
              if (categoryList.includes(element.category))
                map[element.category] = element;
              return map;
            },
            {}));

    this.browserProxy.getOriginPermissions(this.origin, categoryList)
        .then((exceptionList) => {
          exceptionList.forEach((exception, i) => {
            // |exceptionList| should be in the same order as |categoryList|.
            permissionsMap[categoryList[i]].site = exception;
          });

          // The displayName won't change, so just use the first exception.
          assert(exceptionList.length > 0);
          this.pageTitle = exceptionList[0].displayName;
        });
  },

  /** @private */
  onCloseDialog_: function() {
    this.$.confirmDeleteDialog.close();
  },

  /**
   * Confirms the deletion of storage for a site.
   * @param {!Event} e
   * @private
   */
  onConfirmClearSettings_: function(e) {
    e.preventDefault();
    this.$.confirmDeleteDialog.showModal();
  },

  /**
   * Clears all data stored for the current origin.
   * @private
   */
  onClearStorage_: function() {
    // Since usage is only shown when "Site Settings" is enabled, don't clear it
    // when it's not shown.
    if (this.enableSiteSettings_)
      this.$.usageApi.clearUsage(
          this.toUrl(this.origin).href, this.storageType_);
  },

  /**
   * Called when usage has been deleted for an origin.
   * @param {!{detail: !{origin: string}}} event
   * @private
   */
  onUsageDeleted_: function(event) {
    if (event.detail.origin == this.toUrl(this.origin).href)
      this.storedData_ = '';
  },

  /**
   * Resets all permissions and clears all data stored for the current origin.
   * @private
   */
  onClearAndReset_: function() {
    this.browserProxy.setOriginPermissions(
        this.origin, this.getCategoryList_(), settings.ContentSetting.DEFAULT);
    if (this.getCategoryList_().includes(settings.ContentSettingsTypes.PLUGINS))
      this.browserProxy.clearFlashPref(this.origin);

    if (this.storedData_ != '')
      this.onClearStorage_();

    this.onCloseDialog_();
  },

  /**
   * Returns list of categories for each permission displayed in <site-details>.
   * @return {!Array<!settings.ContentSettingsTypes>}
   * @private
   */
  getCategoryList_: function() {
    const categoryList = [];
    this.root.querySelectorAll('site-details-permission').forEach((element) => {
      if (!element.hidden)
        categoryList.push(element.category);
    });
    return categoryList;
  },

  /**
   * Checks whether the permission list is standalone or has a heading.
   * @return {string} CSS class applied when the permission list has no heading.
   * @private
   */
  permissionListClass_: function(hasHeading) {
    return hasHeading ? '' : 'without-heading';
  },

  /**
   * Checks whether this site has any usage information to show.
   * @return {boolean} Whether there is any usage information to show (e.g.
   *     disk or battery).
   * @private
   */
  hasUsage_: function(storage) {
    return storage != '';
  },

});
