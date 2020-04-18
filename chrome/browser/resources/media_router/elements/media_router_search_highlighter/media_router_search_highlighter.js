// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// This Polymer element displays text that needs sections of it highlighted.
// This is useful, for example, for displaying which portions of a string were
// matched by some filter text.
Polymer({
  is: 'media-router-search-highlighter',

  properties: {
    /**
     * The text that this element should display, split it into highlighted and
     * normal text. The displayed text will alternate between plainText and
     * highlightedText.
     *
     * Example: You have a sink with the name 'living room'.
     * When your seach text is 'living', the resulting arrays will be:
     *     plainText: [null, ' room'], highlightedText: ['living', null]
     *
     * When your search text is 'room', the resulting arrays will be:
     *     plainText: ['living ', null], highlightedText: [null, 'room']
     *
     * null corresponds to an empty string when the arrays are being combined.
     * So both examples reproduce the text 'living room', but with different
     * words highlighted.
     * @type {{highlightedText: !Array<?string>,
     *         plainText: !Array<?string>}|undefined}
     */
    data: {
      type: Object,
      observer: 'dataChanged_',
    },

    /**
     * The text that this element is displaying as a plain string. The primary
     * purpose for this property is to make getting this element's textContent
     * easy for testing.
     * @type {string|undefined}
     */
    text: {
      type: String,
      readOnly: true,
      notify: false,
    },
  },

  /**
   * Update the element text if |data| changes.
   *
   * The order the strings are combined is plainText[0] highlightedText[0]
   * plainText[1] highlightedText[1] etc.
   *
   * @param {{highlightedText: !Array<?string>, plainText: !Array<?string>}}
   *    data
   * Parameters in |data|:
   *   highlightedText - Array of strings that should be displayed highlighted.
   *   plainText - Array of strings that should be displayed normally.
   */
  dataChanged_: function(data) {
    if (!data || !data.highlightedText || !data.plainText) {
      return;
    }

    var text = '';
    for (var i = 0; i < data.highlightedText.length; ++i) {
      if (data.plainText[i]) {
        text += HTMLEscape(/** @type {!string} */ (data.plainText[i]));
      }
      if (data.highlightedText[i]) {
        text += '<span class="highlight">' +
            HTMLEscape(/** @type {!string} */ (data.highlightedText[i])) +
            '</span>';
      }
    }
    this.$.text.innerHTML = text;
    this._setText(this.$.text.textContent);
  },
});
