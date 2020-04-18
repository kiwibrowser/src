// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * @fileoverview 'settings-search-engines-page' is the settings page
 * containing search engines settings.
 */
Polymer({
  is: 'settings-search-engines-page',

  behaviors: [settings.GlobalScrollTargetBehavior, WebUIListenerBehavior],

  properties: {
    /** @type {!Array<!SearchEngine>} */
    defaultEngines: Array,

    /** @type {!Array<!SearchEngine>} */
    otherEngines: Array,

    /** @type {!Array<!SearchEngine>} */
    extensions: Array,

    /**
     * Needed by GlobalScrollTargetBehavior.
     * @override
     */
    subpageRoute: {
      type: Object,
      value: settings.routes.SEARCH_ENGINES,
    },

    /** @private {boolean} */
    showExtensionsList_: {
      type: Boolean,
      computed: 'computeShowExtensionsList_(extensions)',
    },

    /** Filters out all search engines that do not match. */
    filter: {
      type: String,
      value: '',
    },

    /** @private {!Array<!SearchEngine>} */
    matchingDefaultEngines_: {
      type: Array,
      computed: 'computeMatchingEngines_(defaultEngines, filter)',
    },

    /** @private {!Array<!SearchEngine>} */
    matchingOtherEngines_: {
      type: Array,
      computed: 'computeMatchingEngines_(otherEngines, filter)',
    },

    /** @private {!Array<!SearchEngine>} */
    matchingExtensions_: {
      type: Array,
      computed: 'computeMatchingEngines_(extensions, filter)',
    },

    /** @private {HTMLElement} */
    omniboxExtensionlastFocused_: Object,

    /** @private {?SearchEngine} */
    dialogModel_: {
      type: Object,
      value: null,
    },

    /** @private {?HTMLElement} */
    dialogAnchorElement_: {
      type: Object,
      value: null,
    },

    /** @private */
    showDialog_: {
      type: Boolean,
      value: false,
    },
  },

  // Since the iron-list for extensions is enclosed in a dom-if, observe both
  // |extensions| and |showExtensionsList_|.
  observers: ['extensionsChanged_(extensions, showExtensionsList_)'],

  listeners: {
    'edit-search-engine': 'onEditSearchEngine_',
  },

  /** @override */
  ready: function() {
    settings.SearchEnginesBrowserProxyImpl.getInstance()
        .getSearchEnginesList()
        .then(this.enginesChanged_.bind(this));
    this.addWebUIListener(
        'search-engines-changed', this.enginesChanged_.bind(this));

    // Sets offset in iron-list that uses the page as a scrollTarget.
    Polymer.RenderStatus.afterNextRender(this, function() {
      this.$.otherEngines.scrollOffset = this.$.otherEngines.offsetTop;
    });
  },

  /**
   * @param {?SearchEngine} searchEngine
   * @param {!HTMLElement} anchorElement
   * @private
   */
  openDialog_: function(searchEngine, anchorElement) {
    this.dialogModel_ = searchEngine;
    this.dialogAnchorElement_ = anchorElement;
    this.showDialog_ = true;
  },

  /** @private */
  onCloseDialog_: function() {
    this.showDialog_ = false;
    const anchor = /** @type {!HTMLElement} */ (this.dialogAnchorElement_);
    cr.ui.focusWithoutInk(anchor);
    this.dialogModel_ = null;
    this.dialogAnchorElement_ = null;
  },

  /**
   * @param {!CustomEvent} e
   * @private
   */
  onEditSearchEngine_: function(e) {
    const params =
        /** @type {!{engine: !SearchEngine, anchorElement: !HTMLElement}} */ (
            e.detail);
    this.openDialog_(params.engine, params.anchorElement);
  },

  /** @private */
  extensionsChanged_: function() {
    if (this.showExtensionsList_ && this.$.extensions)
      this.$.extensions.notifyResize();
  },

  /**
   * @param {!SearchEnginesInfo} searchEnginesInfo
   * @private
   */
  enginesChanged_: function(searchEnginesInfo) {
    this.defaultEngines = searchEnginesInfo.defaults;

    // Sort |otherEngines| in alphabetical order.
    this.otherEngines = searchEnginesInfo.others.sort(
        (a, b) => a.name.toLocaleLowerCase().localeCompare(
            b.name.toLocaleLowerCase()));

    this.extensions = searchEnginesInfo.extensions;
  },

  /**
   * @param {!Event} e
   * @private
   */
  onAddSearchEngineTap_: function(e) {
    e.preventDefault();
    this.openDialog_(null, assert(this.$.addSearchEngine));
  },

  /** @private */
  computeShowExtensionsList_: function() {
    return this.extensions.length > 0;
  },

  /**
   * Filters the given list based on the currently existing filter string.
   * @param {!Array<!SearchEngine>} list
   * @return {!Array<!SearchEngine>}
   * @private
   */
  computeMatchingEngines_: function(list) {
    if (this.filter == '')
      return list;

    const filter = this.filter.toLowerCase();
    return list.filter(e => {
      return [e.displayName, e.name, e.keyword, e.url].some(
          term => term.toLowerCase().includes(filter));
    });
  },

  /**
   * @param {!Array<!SearchEngine>} list The original list.
   * @param {!Array<!SearchEngine>} filteredList The filtered list.
   * @return {boolean} Whether to show the "no results" message.
   * @private
   */
  showNoResultsMessage_: function(list, filteredList) {
    return list.length > 0 && filteredList.length == 0;
  },
});
