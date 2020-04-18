// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/** @polymerBehavior */
const UiPageContainerBehaviorImpl = {
  properties: {
    /**
     * ID for forward button label, which must be translated for display.
     *
     * Undefined if the visible page has no forward-navigation button.
     *
     * @type {string|undefined}
     */
    forwardButtonTextId: String,

    /**
     * ID for backward button label, which must be translated for display.
     *
     * Undefined if the visible page has no backward-navigation button.
     *
     * @type {string|undefined}
     */
    backwardButtonTextId: String,

    /**
     * ID for text of main UI Page heading.
     *
     * @type {string}
     */
    headerId: String,

    /**
     * ID for text of main UI Page message body.
     *
     * @type {string}
     */
    messageId: String,

    /**
     * Translated text to display on the forward-naviation button.
     *
     * Undefined if the visible page has no forward-navigation button.
     *
     * @type {string|undefined}
     */
    forwardButtonText: {
      type: String,
      computed: 'computeLocalizedText_(forwardButtonTextId)',
    },

    /**
     * Translated text to display on the backward-naviation button.
     *
     * Undefined if the visible page has no backward-navigation button.
     *
     * @type {string|undefined}
     */
    backwardButtonText: {
      type: String,
      computed: 'computeLocalizedText_(backwardButtonTextId)',
    },

    /**
     * Translated text of main UI Page heading.
     *
     * @type {string|undefined}
     */
    headerText: {
      type: String,
      computed: 'computeLocalizedText_(headerId)',
    },

    /**
     * Translated text of main UI Page heading. In general this can include
     * some markup.
     *
     * @type {string|undefined}
     */
    messageHtml: {
      type: String,
      computed: 'computeLocalizedText_(messageId)',
    },
  },

  /**
   * @param {string} textId Key for the localized string to appear on a
   *     button.
   * @return {string|undefined} The localized string corresponding to the key
   *     textId. Return value is undefined if textId is not a key
   *     for any localized string. Note: this includes the case in which
   *     textId is undefined.
   * @private
   */
  computeLocalizedText_: function(textId) {
    if (!this.i18nExists(textId))
      return;
    return this.i18nAdvanced(
        textId, {attrs: {'id': (node, value) => node.tagName == 'A'}});
  },
};

/** @polymerBehavior */
const UiPageContainerBehavior = [
  I18nBehavior,
  UiPageContainerBehaviorImpl,
];
