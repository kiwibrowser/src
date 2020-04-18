// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
cr.define('extensions', function() {
  const Sidebar = Polymer({
    is: 'extensions-sidebar',

    properties: {
      isSupervised: Boolean,
    },

    hostAttributes: {
      role: 'navigation',
    },

    /** @override */
    attached: function() {
      this.$.sectionMenu.select(
          extensions.navigation.getCurrentPage().page == Page.SHORTCUTS ? 1 :
                                                                          0);
    },

    /**
     * @param {!Event} e
     * @private
     */
    onLinkTap_: function(e) {
      e.preventDefault();
      extensions.navigation.navigateTo({page: e.target.dataset.path});
      this.fire('close-drawer');
    },

    /** @private */
    onMoreExtensionsTap_: function() {
      assert(!this.isSupervised);
      chrome.metricsPrivate.recordUserAction('Options_GetMoreExtensions');
    },
  });

  return {Sidebar: Sidebar};
});
