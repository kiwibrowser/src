// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * @fileoverview DotList implementation
 */

cr.define('ntp', function() {
  'use strict';

  /**
   * Live list of the navigation dots.
   * @type {!NodeList|undefined}
   */
  var navDots;

  /**
   * Creates a new DotList object.
   * @constructor
   * @extends {HTMLUListElement}
   */
  var DotList = cr.ui.define('ul');

  DotList.prototype = {
    __proto__: HTMLUListElement.prototype,

    decorate: function() {
      this.addEventListener('keydown', this.onKeyDown_.bind(this));
      navDots = this.getElementsByClassName('dot');
    },

    /**
     * Live list of the navigation dots.
     * @type {!NodeList|undefined}
     */
    get dots() {
      return navDots;
    },

    /**
     * Handler for key events on the dot list. These keys will change the focus
     * element.
     * @param {!Event} e The KeyboardEvent.
     */
    onKeyDown_: function(e) {
      if (hasKeyModifiers(e))
        return;

      var direction = 0;
      if (e.key == 'ArrowLeft')
        direction = -1;
      else if (e.key == 'ArrowRight')
        direction = 1;
      else
        return;

      var focusDot = this.querySelector('.dot:focus');
      if (!focusDot)
        return;
      var focusIndex = Array.prototype.indexOf.call(navDots, focusDot);
      var newFocusIndex = focusIndex + direction;
      if (focusIndex == newFocusIndex)
        return;

      newFocusIndex = (newFocusIndex + navDots.length) % navDots.length;
      navDots[newFocusIndex].tabIndex = 3;
      navDots[newFocusIndex].focus();
      focusDot.tabIndex = -1;

      e.stopPropagation();
      e.preventDefault();
    }
  };

  return {DotList: DotList};
});
