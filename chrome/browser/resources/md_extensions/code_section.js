// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

cr.define('extensions', function() {
  'use strict';

  /**
   * @param {number} totalCount
   * @param {number} oppositeCount
   * @return {number}
   */
  function visibleLineCount(totalCount, oppositeCount) {
    // We limit the number of lines shown for DOM performance.
    const MAX_VISIBLE_LINES = 1000;
    const max =
        Math.max(MAX_VISIBLE_LINES / 2, MAX_VISIBLE_LINES - oppositeCount);
    return Math.min(max, totalCount);
  }

  const CodeSection = Polymer({
    is: 'extensions-code-section',

    behaviors: [I18nBehavior],

    properties: {
      /**
       * The code this object is displaying.
       * @type {?chrome.developerPrivate.RequestFileSourceResponse}
       */
      code: {
        type: Object,
        value: null,
      },

      /** @private Highlighted code. */
      highlighted_: String,

      /** @private Code before the highlighted section. */
      before_: String,

      /** @private Code after the highlighted section. */
      after_: String,

      /** @private Description for the highlighted section. */
      highlightDescription_: String,

      /** @private */
      lineNumbers_: String,

      /** @private */
      truncatedBefore_: Number,

      /** @private */
      truncatedAfter_: Number,

      /**
       * The string to display if no |code| is set (e.g. because we couldn't
       * load the relevant source file).
       * @type {string}
       */
      couldNotDisplayCode: String,
    },

    observers: [
      'onCodeChanged_(code.*)',
    ],

    /**
     * @private
     */
    onCodeChanged_: function() {
      if (!this.code ||
          (!this.code.beforeHighlight && !this.code.highlight &&
           !this.code.afterHighlight)) {
        this.highlighted_ = '';
        this.highlightDescription_ = '';
        this.before_ = '';
        this.after_ = '';
        this.lineNumbers_ = '';
        return;
      }

      const before = this.code.beforeHighlight;
      const highlight = this.code.highlight;
      const after = this.code.afterHighlight;

      const linesBefore = before ? before.split('\n') : [];
      const linesAfter = after ? after.split('\n') : [];
      const visibleLineCountBefore =
          visibleLineCount(linesBefore.length, linesAfter.length);
      const visibleLineCountAfter =
          visibleLineCount(linesAfter.length, linesBefore.length);

      const visibleBefore =
          linesBefore.slice(linesBefore.length - visibleLineCountBefore)
              .join('\n');
      let visibleAfter = linesAfter.slice(0, visibleLineCountAfter).join('\n');
      // If the last character is a \n, force it to be rendered.
      if (visibleAfter.charAt(visibleAfter.length - 1) == '\n')
        visibleAfter += ' ';

      this.highlighted_ = highlight;
      this.highlightDescription_ = this.getAccessibilityHighlightDescription_(
          linesBefore.length, highlight.split('\n').length);
      this.before_ = visibleBefore;
      this.after_ = visibleAfter;
      this.truncatedBefore_ = linesBefore.length - visibleLineCountBefore;
      this.truncatedAfter_ = linesAfter.length - visibleLineCountAfter;

      let visibleCode = visibleBefore + highlight + visibleAfter;

      this.setLineNumbers_(
          this.truncatedBefore_ + 1,
          this.truncatedBefore_ + visibleCode.split('\n').length);
      this.scrollToHighlight_(visibleLineCountBefore);
    },

    /**
     * @param {number} lineCount
     * @param {string} stringSingular
     * @param {string} stringPluralTemplate
     * @return {string}
     * @private
     */
    getLinesNotShownLabel_(lineCount, stringSingular, stringPluralTemplate) {
      return lineCount == 1 ?
          stringSingular :
          loadTimeData.substituteString(stringPluralTemplate, lineCount);
    },

    /**
     * @param {number} start
     * @param {number} end
     * @private
     */
    setLineNumbers_: function(start, end) {
      let lineNumbers = '';
      for (let i = start; i <= end; ++i)
        lineNumbers += i + '\n';

      this.lineNumbers_ = lineNumbers;
    },

    /**
     * @param {number} linesBeforeHighlight
     * @private
     */
    scrollToHighlight_: function(linesBeforeHighlight) {
      const CSS_LINE_HEIGHT = 20;

      // Count how many pixels is above the highlighted code.
      const highlightTop = linesBeforeHighlight * CSS_LINE_HEIGHT;

      // Find the position to show the highlight roughly in the middle.
      const targetTop = highlightTop - this.clientHeight * 0.5;

      this.$['scroll-container'].scrollTo({top: targetTop});
    },

    /**
     * @param {number} lineStart
     * @param {number} numLines
     * @return {string}
     * @private
     */
    getAccessibilityHighlightDescription_: function(lineStart, numLines) {
      if (numLines > 1) {
        return this.i18n(
            'accessibilityErrorMultiLine', lineStart.toString(),
            (lineStart + numLines - 1).toString());
      } else {
        return this.i18n('accessibilityErrorLine', lineStart.toString());
      }
    },
  });

  return {CodeSection: CodeSection};
});
