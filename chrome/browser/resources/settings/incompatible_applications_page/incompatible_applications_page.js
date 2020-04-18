// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * @fileoverview
 * 'settings-incompatible-applications-page' is the settings subpage containing
 * the list of incompatible applications.
 *
 * Example:
 *
 *    <iron-animated-pages>
 *      <settings-incompatible-applications-page">
 *      </settings-incompatible-applications-page>
 *      ... other pages ...
 *    </iron-animated-pages>
 */

Polymer({
  is: 'settings-incompatible-applications-page',

  behaviors: [I18nBehavior, WebUIListenerBehavior],

  properties: {
    /**
     * Indicates if the current user has administrator rights.
     * @private
     */
    hasAdminRights_: {
      type: Boolean,
      value: function() {
        return loadTimeData.getBoolean('hasAdminRights');
      },
    },

    /**
     * The list of all the incompatible applications.
     * @private {Array<settings.IncompatibleApplication>}
     */
    applications_: Array,

    /**
     * Determines if the user has finished with this page.
     * @private
     */
    isDone_: {
      type: Boolean,
      computed: 'computeIsDone_(applications_.*)',
    },

    /**
     * The text for the subtitle of the subpage.
     * @private
     */
    subtitleText_: {
      type: String,
      value: '',
    },

    /**
     * The text for the subtitle of the subpage, when the user does not have
     * administrator rights.
     * @private
     */
    subtitleNoAdminRightsText_: {
      type: String,
      value: '',
    },

    /**
     * The text for the title of the list of incompatible applications.
     * @private
     */
    listTitleText_: {
      type: String,
      value: '',
    },
  },

  /** @override */
  ready: function() {
    this.addWebUIListener(
        'incompatible-application-removed',
        this.onIncompatibleApplicationRemoved_.bind(this));

    settings.IncompatibleApplicationsBrowserProxyImpl.getInstance()
        .requestIncompatibleApplicationsList()
        .then(list => {
          this.applications_ = list;
          this.updatePluralStrings_();
        });
  },

  /**
   * @return {boolean}
   * @private
   */
  computeIsDone_: function() {
    return this.applications_.length === 0;
  },

  /**
   * Removes a single incompatible application from the |applications_| list.
   * @private
   */
  onIncompatibleApplicationRemoved_: function(applicationName) {
    // Find the index of the element.
    let index = this.applications_.findIndex(function(application) {
      return application.name == applicationName;
    });

    assert(index !== -1);

    this.splice('applications_', index, 1);
  },

  /**
   * Updates the texts of the Incompatible Applications subpage that depends on
   * the length of |applications_|.
   * @private
   */
  updatePluralStrings_: function() {
    const browserProxy =
        settings.IncompatibleApplicationsBrowserProxyImpl.getInstance();
    const numApplications = this.applications_.length;
    Promise
        .all([
          browserProxy.getSubtitlePluralString(numApplications),
          browserProxy.getSubtitleNoAdminRightsPluralString(numApplications),
          browserProxy.getListTitlePluralString(numApplications),
        ])
        .then(strings => {
          this.subtitleText_ = strings[0];
          this.subtitleNoAdminRightsText_ = strings[1];
          this.listTitleText_ = strings[2];
        });
  },
});
