// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * @fileoverview 'settings-search-engines-list' is a component for showing a
 * list of search engines.
 */
Polymer({
  is: 'settings-search-engines-list',

  properties: {
    /** @type {!Array<!SearchEngine>} */
    engines: Array,

    /**
     * The scroll target that this list should use.
     * @type {?HTMLElement}
     */
    scrollTarget: Object,

    /** Used to fix scrolling glitch when list is not top most element. */
    scrollOffset: Number,

    /** @private {Object}*/
    lastFocused_: Object,

    fixedHeight: {
      type: Boolean,
      value: false,
      reflectToAttribute: true,
    },
  },
});
