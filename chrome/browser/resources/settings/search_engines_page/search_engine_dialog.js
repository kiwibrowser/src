// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * @fileoverview 'settings-search-engine-dialog' is a component for adding
 * or editing a search engine entry.
 */
Polymer({
  is: 'settings-search-engine-dialog',

  behaviors: [WebUIListenerBehavior],

  properties: {
    /**
     * The search engine to be edited. If not populated a new search engine
     * should be added.
     * @type {?SearchEngine}
     */
    model: Object,

    /** @private {string} */
    searchEngine_: String,

    /** @private {string} */
    keyword_: String,

    /** @private {string} */
    queryUrl_: String,

    /** @private {string} */
    dialogTitle_: String,

    /** @private {string} */
    actionButtonText_: String,
  },

  /** @private {settings.SearchEnginesBrowserProxy} */
  browserProxy_: null,

  /**
   * The |modelIndex| to use when a new search engine is added. Must match with
   * kNewSearchEngineIndex constant specified at
   * chrome/browser/ui/webui/settings/search_engines_handler.cc
   * @type {number}
   */
  DEFAULT_MODEL_INDEX: -1,

  /** @override */
  created: function() {
    this.browserProxy_ = settings.SearchEnginesBrowserProxyImpl.getInstance();
  },

  /** @override */
  ready: function() {
    if (this.model) {
      this.dialogTitle_ =
          loadTimeData.getString('searchEnginesEditSearchEngine');
      this.actionButtonText_ = loadTimeData.getString('save');

      // If editing an existing search engine, pre-populate the input fields.
      this.searchEngine_ = this.model.name;
      this.keyword_ = this.model.keyword;
      this.queryUrl_ = this.model.url;
    } else {
      this.dialogTitle_ =
          loadTimeData.getString('searchEnginesAddSearchEngine');
      this.actionButtonText_ = loadTimeData.getString('add');
    }

    this.addEventListener('cancel', () => {
      this.browserProxy_.searchEngineEditCancelled();
    });

    this.addWebUIListener(
        'search-engines-changed', this.enginesChanged_.bind(this));
  },

  /** @override */
  attached: function() {
    this.async(this.updateActionButtonState_.bind(this));
    this.browserProxy_.searchEngineEditStarted(
        this.model ? this.model.modelIndex : this.DEFAULT_MODEL_INDEX);
    this.$.dialog.showModal();
  },

  /**
   * @param {!SearchEnginesInfo} searchEnginesInfo
   * @private
   */
  enginesChanged_: function(searchEnginesInfo) {
    if (this.model) {
      const engineWasRemoved = ['defaults', 'others', 'extensions'].every(
          engineType =>
              searchEnginesInfo[engineType].every(e => e.id != this.model.id));
      if (engineWasRemoved) {
        this.cancel_();
        return;
      }
    }

    [this.$.searchEngine, this.$.keyword, this.$.queryUrl].forEach(
        element => this.validateElement_(element));
  },

  /** @private */
  cancel_: function() {
    this.$.dialog.cancel();
  },

  /** @private */
  onActionButtonTap_: function() {
    this.browserProxy_.searchEngineEditCompleted(
        this.searchEngine_, this.keyword_, this.queryUrl_);
    this.$.dialog.close();
  },

  /**
   * @param {!Element} inputElement
   * @private
   */
  validateElement_: function(inputElement) {
    // If element is empty, disable the action button, but don't show the red
    // invalid message.
    if (inputElement.value == '') {
      inputElement.invalid = false;
      this.updateActionButtonState_();
      return;
    }

    this.browserProxy_
        .validateSearchEngineInput(inputElement.id, inputElement.value)
        .then(isValid => {
          inputElement.invalid = !isValid;
          this.updateActionButtonState_();
        });
  },

  /**
   * @param {!Event} event
   * @private
   */
  validate_: function(event) {
    const inputElement = /** @type {!Element} */ (event.target);
    this.validateElement_(inputElement);
  },

  /** @private */
  updateActionButtonState_: function() {
    const allValid = [
      this.$.searchEngine, this.$.keyword, this.$.queryUrl
    ].every(function(inputElement) {
      return !inputElement.invalid && inputElement.value.length > 0;
    });
    this.$.actionButton.disabled = !allValid;
  },
});
