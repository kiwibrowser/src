// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * @fileoverview
 * 'all-sites' is the polymer element for showing the list of all sites under
 * Site Settings.
 */
Polymer({
  is: 'all-sites',

  behaviors: [SiteSettingsBehavior, WebUIListenerBehavior],

  properties: {
    /**
     * Array of sites to display in the widget.
     * @type {!Array<!SiteException>}
     */
    sites: {
      type: Array,
      value: function() {
        return [];
      },
    },
  },

  /** @override */
  ready: function() {
    this.browserProxy_ =
        settings.SiteSettingsPrefsBrowserProxyImpl.getInstance();
    this.addWebUIListener(
        'contentSettingSitePermissionChanged', this.populateList_.bind(this));
    this.populateList_();
  },

  /**
   * Retrieves a list of all known sites with site details.
   * @return {!Promise<!Array<!RawSiteException>>}
   * @private
   */
  getAllSitesList_: function() {
    /** @type {!Array<!RawSiteException>} */
    const promiseList = [];

    const types = Object.values(settings.ContentSettingsTypes);
    for (let i = 0; i < types.length; i++) {
      const type = types[i];
      // <if expr="not chromeos">
      if (type == settings.ContentSettingsTypes.PROTECTED_CONTENT)
        continue;
      // </if>
      if (type == settings.ContentSettingsTypes.PROTOCOL_HANDLERS ||
          type == settings.ContentSettingsTypes.ZOOM_LEVELS) {
        // Some categories store their data in a custom way.
        continue;
      }

      promiseList.push(this.browserProxy_.getExceptionList(type));
    }

    return Promise.all(promiseList);
  },

  /** @private */
  populateList_: function() {
    this.getAllSitesList_().then(this.processExceptions_.bind(this));
  },

  /**
   * Process the exception list returned from the native layer.
   * @param {!Array<!RawSiteException>} data List of sites (exceptions)
   *     to process.
   * @private
   */
  processExceptions_: function(data) {
    const sites = /** @type {!Array<!RawSiteException>} */ ([]);
    for (let i = 0; i < data.length; ++i) {
      const exceptionList = data[i];
      for (let k = 0; k < exceptionList.length; ++k) {
        sites.push(exceptionList[k]);
      }
    }
    this.sites = this.toSiteArray_(sites);
  },

  /**
   * TODO(dschuyler): Move this processing to C++ handler.
   * Converts a list of exceptions received from the C++ handler to
   * full SiteException objects. The list is sorted by site name, then protocol
   * and port and de-duped (by origin).
   * @param {!Array<!RawSiteException>} sites A list of sites to convert.
   * @return {!Array<!SiteException>} A list of full SiteExceptions. Sorted and
   *    deduped.
   * @private
   */
  toSiteArray_: function(sites) {
    const self = this;
    sites.sort(function(a, b) {
      const url1 = self.toUrl(a.origin);
      const url2 = self.toUrl(b.origin);
      let comparison = url1.host.localeCompare(url2.host);
      if (comparison == 0) {
        comparison = url1.protocol.localeCompare(url2.protocol);
        if (comparison == 0) {
          comparison = url1.port.localeCompare(url2.port);
          if (comparison == 0) {
            // Compare hosts for the embedding origins.
            let host1 = self.toUrl(a.embeddingOrigin);
            let host2 = self.toUrl(b.embeddingOrigin);
            host1 = (host1 == null) ? '' : host1.host;
            host2 = (host2 == null) ? '' : host2.host;
            return host1.localeCompare(host2);
          }
        }
      }
      return comparison;
    });
    const results = /** @type {!Array<!SiteException>} */ ([]);
    let lastOrigin = '';
    let lastEmbeddingOrigin = '';
    for (let i = 0; i < sites.length; ++i) {
      // Remove duplicates.
      if (sites[i].origin == lastOrigin &&
          sites[i].embeddingOrigin == lastEmbeddingOrigin) {
        continue;
      }
      /** @type {!SiteException} */
      const siteException = this.expandSiteException(sites[i]);
      results.push(siteException);
      lastOrigin = siteException.origin;
      lastEmbeddingOrigin = siteException.embeddingOrigin;
    }
    return results;
  },
});
