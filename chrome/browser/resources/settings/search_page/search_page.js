// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * @fileoverview
 * 'settings-search-page' is the settings page containing search settings.
 */
Polymer({
  is: 'settings-search-page',

  behaviors: [I18nBehavior],

  properties: {
    prefs: Object,

    // <if expr="chromeos">
    arcEnabled: Boolean,

    voiceInteractionValuePropAccepted: Boolean,
    // </if>

    /**
     * List of default search engines available.
     * @private {!Array<!SearchEngine>}
     */
    searchEngines_: {
      type: Array,
      value: function() {
        return [];
      }
    },

    /** @private Filter applied to search engines. */
    searchEnginesFilter_: String,

    /** @type {?Map<string, string>} */
    focusConfig_: Object,

    // <if expr="chromeos">
    /** @private */
    voiceInteractionFeatureEnabled_: {
      type: Boolean,
      value: function() {
        return loadTimeData.getBoolean('enableVoiceInteraction');
      },
    },

    /** @private */
    assistantOn_: {
      type: Boolean,
      computed:
          'isAssistantTurnedOn_(arcEnabled, voiceInteractionValuePropAccepted)',
    }
    // </if>
  },

  /** @private {?settings.SearchEnginesBrowserProxy} */
  browserProxy_: null,

  /** @override */
  created: function() {
    this.browserProxy_ = settings.SearchEnginesBrowserProxyImpl.getInstance();
  },

  /** @override */
  ready: function() {
    // Omnibox search engine
    const updateSearchEngines = searchEngines => {
      this.set('searchEngines_', searchEngines.defaults);
    };
    this.browserProxy_.getSearchEnginesList().then(updateSearchEngines);
    cr.addWebUIListener('search-engines-changed', updateSearchEngines);

    this.focusConfig_ = new Map();
    if (settings.routes.SEARCH_ENGINES) {
      this.focusConfig_.set(
          settings.routes.SEARCH_ENGINES.path,
          '#engines-subpage-trigger .subpage-arrow');
    }
    // <if expr="chromeos">
    if (settings.routes.GOOGLE_ASSISTANT) {
      this.focusConfig_.set(
          settings.routes.GOOGLE_ASSISTANT.path,
          '#assistant-subpage-trigger .subpage-arrow');
    }
    // </if>
  },

  /** @private */
  onChange_: function() {
    const select = /** @type {!HTMLSelectElement} */ (this.$$('select'));
    const searchEngine = this.searchEngines_[select.selectedIndex];
    this.browserProxy_.setDefaultSearchEngine(searchEngine.modelIndex);
  },

  /** @private */
  onDisableExtension_: function() {
    this.fire('refresh-pref', 'default_search_provider.enabled');
  },

  /** @private */
  onManageSearchEnginesTap_: function() {
    settings.navigateTo(settings.routes.SEARCH_ENGINES);
  },

  // <if expr="chromeos">
  /** @private */
  onGoogleAssistantTap_: function() {
    assert(this.voiceInteractionFeatureEnabled_);

    if (!this.assistantOn_) {
      return;
    }

    settings.navigateTo(settings.routes.GOOGLE_ASSISTANT);
  },

  /** @private */
  onAssistantTurnOnTap_: function(event) {
    this.browserProxy_.turnOnGoogleAssistant();
  },
  // </if>

  // <if expr="chromeos">
  /**
   * @param {boolean} toggleValue
   * @return {string}
   * @private
   */
  getAssistantEnabledDisabledLabel_: function(toggleValue) {
    return this.i18n(
        toggleValue ? 'searchGoogleAssistantEnabled' :
                      'searchGoogleAssistantDisabled');
  },

  /** @private
   *  @param {boolean} arcEnabled
   *  @param {boolean} valuePropAccepted
   *  @return {boolean}
   */
  isAssistantTurnedOn_: function(arcEnabled, valuePropAccepted) {
    return arcEnabled && valuePropAccepted;
  },
  // </if>

  /**
   * @param {!Event} event
   * @private
   */
  doNothing_: function(event) {
    event.stopPropagation();
  },

  /**
   * @param {!chrome.settingsPrivate.PrefObject} pref
   * @return {boolean}
   * @private
   */
  isDefaultSearchControlledByPolicy_: function(pref) {
    return pref.controlledBy == chrome.settingsPrivate.ControlledBy.USER_POLICY;
  },

  /**
   * @param {!chrome.settingsPrivate.PrefObject} pref
   * @return {boolean}
   * @private
   */
  isDefaultSearchEngineEnforced_: function(pref) {
    return pref.enforcement == chrome.settingsPrivate.Enforcement.ENFORCED;
  },
});
