// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

cr.define('print_preview', function() {
  'use strict';

  /**
   * Encapsulated handling of a search bubble.
   * @constructor
   * @extends {HTMLDivElement}
   */
  function SearchBubble(text) {
    const el = cr.doc.createElement('div');
    SearchBubble.decorate(el);
    el.content = text;
    return el;
  }

  SearchBubble.decorate = function(el) {
    el.__proto__ = SearchBubble.prototype;
    el.decorate();
  };

  SearchBubble.prototype = {
    __proto__: HTMLDivElement.prototype,

    decorate: function() {
      this.className = 'search-bubble';

      this.innards_ = cr.doc.createElement('div');
      this.innards_.className = 'search-bubble-innards';
      this.appendChild(this.innards_);

      // We create a timer to periodically update the position of the bubbles.
      // While this isn't all that desirable, it's the only sure-fire way of
      // making sure the bubbles stay in the correct location as sections
      // may dynamically change size at any time.
      this.intervalId = setInterval(this.updatePosition.bind(this), 250);
    },

    /**
     * Sets the text message in the bubble.
     * @param {string} text The text the bubble will show.
     */
    set content(text) {
      this.innards_.textContent = text;
    },

    /** Attach the bubble to the element. */
    attachTo: function(element) {
      const parent = element.parentElement;
      if (!parent)
        return;
      if (parent.tagName == 'TD') {
        // To make absolute positioning work inside a table cell we need
        // to wrap the bubble div into another div with position:relative.
        // This only works properly if the element is the first child of the
        // table cell which is true for all options pages (the only place
        // it is used on tables).
        this.wrapper = cr.doc.createElement('div');
        this.wrapper.className = 'search-bubble-wrapper';
        this.wrapper.appendChild(this);
        parent.insertBefore(this.wrapper, element);
      } else {
        parent.insertBefore(this, element);
      }
      this.updatePosition();
    },

    /** Clear the interval timer and remove the element from the page. */
    dispose: function() {
      clearInterval(this.intervalId);

      const child = this.wrapper || this;
      const parent = child.parentNode;
      if (parent)
        parent.removeChild(child);
    },

    /**
     * Update the position of the bubble. Called at creation time and then
     * periodically while the bubble remains visible.
     */
    updatePosition: function() {
      // This bubble is 'owned' by the next sibling.
      const owner = (this.wrapper || this).nextSibling;

      // If there isn't an offset parent, we have nothing to do.
      if (!owner.offsetParent)
        return;

      // Position the bubble below the location of the owner.
      const left =
          owner.offsetLeft + owner.offsetWidth / 2 - this.offsetWidth / 2;
      const top = owner.offsetTop + owner.offsetHeight;

      // Update the position in the CSS.  Cache the last values for
      // best performance.
      if (left != this.lastLeft) {
        this.style.left = left + 'px';
        this.lastLeft = left;
      }
      if (top != this.lastTop) {
        this.style.top = top + 'px';
        this.lastTop = top;
      }
    },
  };

  // Export
  return {SearchBubble: SearchBubble};
});
