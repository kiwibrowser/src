// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * @fileoverview
 * 'site-entry' is an element representing a single ETLD+1 site entity.
 */

Polymer({
  is: 'site-entry',

  behaviors: [SiteSettingsBehavior],

  properties: {
    /**
     * TODO(https://crbug.com/835712): This should be an object representing a
     * ETLD+1 site.
     */
    site: Object,
  },

  /** @override */
  ready: function() {},

  /**
   * A handler for selecting a site (by clicking on the origin).
   * @param {!{model: !{item: !SiteException}}} event
   * @private
   */
  onOriginTap_: function(event) {
    settings.navigateTo(
        settings.routes.SITE_SETTINGS_SITE_DETAILS,
        new URLSearchParams('site=' + this.site.origin));
  },

});
