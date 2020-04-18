// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * @fileoverview CrContainerShadowBehavior holds logic for showing a drop shadow
 * near the top of a container element, when the content has scrolled.
 *
 * Elements using this behavior are expected to define a #container element,
 * which is the element being scrolled.
 *
 * The behavior will take care of inserting an element with ID
 * 'cr-container-shadow' which holds the drop shadow effect. A 'has-shadow'
 * CSS class is automatically added/removed while scrolling, as necessary.
 *
 * Clients should either use the existing shared styling in
 * shared_styles_css.html, '#cr-container-shadow' and
 * '#cr-container-shadow.has-shadow', or define their own styles.
 */

/** @polymerBehavior */
var CrContainerShadowBehavior = {
  /** @private {?IntersectionObserver} */
  intersectionObserver_: null,

  /** @override */
  attached: function() {
    // The element holding the drop shadow effect to be shown.
    var dropShadow = document.createElement('div');
    // This ID should match the CSS rules in shared_styles_css.html.
    dropShadow.id = 'cr-container-shadow';
    this.shadowRoot.insertBefore(dropShadow, this.$.container);

    // Dummy element used to detect scrolling. Has a 0px height intentionally.
    var intersectionProbe = document.createElement('div');
    this.$.container.prepend(intersectionProbe);

    // Setup drop shadow logic.
    var callback = entries => {
      dropShadow.classList.toggle(
          'has-shadow', entries[entries.length - 1].intersectionRatio == 0);
    };

    this.intersectionObserver_ = new IntersectionObserver(
        callback,
        /** @type {IntersectionObserverInit} */ ({
          root: this.$.container,
          threshold: 0,
        }));

    // Need to register the observer within a setTimeout() callback, otherwise
    // the drop shadow flashes once on startup, because of the DOM modifications
    // earlier in this function causing a relayout.
    window.setTimeout(() => {
      if (this.intersectionObserver_)  // In case this is already detached.
        this.intersectionObserver_.observe(intersectionProbe);
    });
  },

  /** @override */
  detached: function() {
    this.intersectionObserver_.disconnect();
    this.intersectionObserver_ = null;
  },
};
