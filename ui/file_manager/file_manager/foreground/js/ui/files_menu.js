// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

cr.define('cr.ui', function() {
  /**
   * Menu item with ripple animation.
   * @constructor
   * @extends {cr.ui.MenuItem}
   *
   * TODO(mtomasz): Upstream to cr.ui.MenuItem.
   */
  var FilesMenuItem = cr.ui.define(cr.ui.MenuItem);

  FilesMenuItem.prototype = {
    __proto__: cr.ui.MenuItem.prototype,

    /**
     * @private {boolean}
     */
    animating_: false,

    /**
     * @private {(boolean|undefined)}
     */
    hidden_: undefined,

    /**
     * @private {HTMLElement}
     */
    label_: null,

    /**
     * @private {HTMLElement}
     */
    iconStart_: null,

    /**
     * @private {HTMLElement}
     */
    ripple_: null,

    /**
     * @override
     */
    decorate: function() {
      // Custom menu item can have sophisticated content (elements).
      if (!this.children.length) {
        this.label_ =
            assertInstanceof(document.createElement('span'), HTMLElement);
        this.label_.textContent = this.textContent;

        this.iconStart_ =
            assertInstanceof(document.createElement('div'), HTMLElement);
        this.iconStart_.classList.add('icon', 'start');

        // Override with standard menu item elements.
        this.textContent = '';
        this.appendChild(this.iconStart_);
        this.appendChild(this.label_);
      }

      this.ripple_ =
          assertInstanceof(document.createElement('paper-ripple'), HTMLElement);
      this.appendChild(this.ripple_);

      this.addEventListener('activate', this.onActivated_.bind(this));
    },

    /**
     * Handles activate event.
     * @param {Event} event
     * @private
     */
    onActivated_: function(event) {
      // Perform ripple animation if it's activated by keyboard.
      if (event.originalEvent instanceof KeyboardEvent)
        this.ripple_.simulatedRipple();

      // Perform fade out animation.
      var menu = assertInstanceof(this.parentNode, cr.ui.Menu);
      this.setMenuAsAnimating_(menu, true /* animating */);

      var player = menu.animate([{
        opacity: 1,
        offset: 0
      }, {
        opacity: 0,
        offset: 1
      }], 300);

      player.addEventListener('finish',
          this.setMenuAsAnimating_.bind(this, menu, false /* not animating */));
    },

    /**
     * Sets menu as animating.
     * @param {!cr.ui.Menu} menu
     * @param {boolean} value True to set it as animating.
     * @private
     */
    setMenuAsAnimating_: function(menu, value) {
      menu.classList.toggle('animating', value);

      for (var i = 0; i < menu.menuItems.length; i++) {
        var menuItem = menu.menuItems[i];
        if (menuItem instanceof cr.ui.FilesMenuItem)
          menuItem.setAnimating_(value);
      }
    },

    /**
     * Sets thie menu item as animating.
     * @param {boolean} value True to set this as animating.
     * @private
     */
    setAnimating_: function(value) {
      this.animating_ = value;

      if (this.animating_)
        return;

      // Update hidden property if there is a pending change.
      if (this.hidden_ !== undefined) {
        this.hidden = this.hidden_;
        this.hidden_ = undefined;
      }
    },

    /**
     * @return {boolean}
     */
    get hidden() {
      if (this.hidden_ !== undefined)
        return this.hidden_;

      return Object.getOwnPropertyDescriptor(
          HTMLElement.prototype, 'hidden').get.call(this);
    },

    /**
     * Overrides hidden property to block the change of hidden property while
     * menu is animating.
     * @param {boolean} value
     */
    set hidden(value) {
      if (this.animating_) {
        this.hidden_ = value;
        return;
      }

      Object.getOwnPropertyDescriptor(
          HTMLElement.prototype, 'hidden').set.call(this, value);
    },

    /**
     * @return {string}
     */
    get label() {
      return this.label_.textContent;
    },

    /**
     * @param {string} value
     */
    set label(value) {
      this.label_.textContent = value;
    },

    /**
     * @return {string}
     */
    get iconStartImage() {
      return this.iconStart_.style.backgroundImage;
    },

    /**
     * @param {string} value
     */
    set iconStartImage(value) {
      this.iconStart_.setAttribute('style', 'background-image: ' + value);
    }
  };

  return {
    FilesMenuItem: FilesMenuItem
  };
});
