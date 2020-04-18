// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * @fileoverview
 * 'I18nBehavior' is a behavior to mix in loading of internationalization
 * strings.
 */

/** @polymerBehavior */
var I18nBehavior = {
  properties: {
    /**
     * The language the UI is presented in. Used to signal dynamic language
     * change.
     */
    locale: {
      type: String,
      value: '',
    },
  },

  /**
   * Returns a translated string where $1 to $9 are replaced by the given
   * values.
   * @param {string} id The ID of the string to translate.
   * @param {...string} var_args Values to replace the placeholders $1 to $9
   *     in the string.
   * @return {string} A translated, substituted string.
   * @private
   */
  i18nRaw_: function(id, var_args) {
    return arguments.length == 1 ?
        loadTimeData.getString(id) :
        loadTimeData.getStringF.apply(loadTimeData, arguments);
  },

  /**
   * Returns a translated string where $1 to $9 are replaced by the given
   * values. Also sanitizes the output to filter out dangerous HTML/JS.
   * Use with Polymer bindings that are *not* inner-h-t-m-l.
   * @param {string} id The ID of the string to translate.
   * @param {...string} var_args Values to replace the placeholders $1 to $9
   *     in the string.
   * @return {string} A translated, sanitized, substituted string.
   */
  i18n: function(id, var_args) {
    var rawString = this.i18nRaw_.apply(this, arguments);
    return parseHtmlSubset('<b>' + rawString + '</b>').firstChild.textContent;
  },

  /**
   * Similar to 'i18n', returns a translated, sanitized, substituted string.
   * It receives the string ID and a dictionary containing the substitutions
   * as well as optional additional allowed tags and attributes. Use with
   * Polymer bindings that are inner-h-t-m-l, for example.
   * @param {string} id The ID of the string to translate.
   * @param {SanitizeInnerHtmlOpts=} opts
   * @return {string}
   */
  i18nAdvanced: function(id, opts) {
    opts = opts || {};
    var args = [id].concat(opts.substitutions || []);
    var rawString = this.i18nRaw_.apply(this, args);
    return loadTimeData.sanitizeInnerHtml(rawString, opts);
  },

  /**
   * Similar to 'i18n', with an unused |locale| parameter used to trigger
   * updates when |this.locale| changes.
   * @param {string} locale The UI language used.
   * @param {string} id The ID of the string to translate.
   * @param {...string} var_args Values to replace the placeholders $1 to $9
   *     in the string.
   * @return {string} A translated, sanitized, substituted string.
   */
  i18nDynamic: function(locale, id, var_args) {
    return this.i18n.apply(this, Array.prototype.slice.call(arguments, 1));
  },

  /**
   * Returns true if a translation exists for |id|.
   * @param {string} id
   * @return {boolean}
   */
  i18nExists: function(id) {
    return loadTimeData.valueExists(id);
  },

  /**
   * Call this when UI strings may have changed. This will send an update to
   * any data bindings to i18nDynamic(locale, ...).
   */
  i18nUpdateLocale: function() {
    this.locale = loadTimeData.getString('language');
  },
};

/**
 * TODO(stevenjb): Replace with an interface. b/24294625
 * @typedef {{
 *   i18n: function(string, ...string): string,
 *   i18nAdvanced: function(string, SanitizeInnerHtmlOpts=): string,
 *   i18nDynamic: function(string, string, ...string): string,
 *   i18nExists: function(string),
 *   i18nUpdateLocale: function()
 * }}
 */
I18nBehavior.Proto;
