// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * @param {!cr.ui.MenuButton} selectionMenuButton
 * @param {!cr.ui.Menu} menu
 * @constructor
 * @struct
 */
function SelectionMenuController(selectionMenuButton, menu) {
  /**
   * @type {!FilesToggleRipple}
   * @const
   * @private
   */
  this.toggleRipple_ =
      /** @type {!FilesToggleRipple} */ (
          queryRequiredElement('files-toggle-ripple', selectionMenuButton));

  /**
   * @type {!cr.ui.Menu}
   * @const
   */
  this.menu_ = menu;

  selectionMenuButton.addEventListener('menushow', this.onShowMenu_.bind(this));
  selectionMenuButton.addEventListener('menuhide', this.onHideMenu_.bind(this));
}

/**
 * Class name to indicate if the menu was opened by the toolbar button or not.
 * @type {string}
 * @const
 */
SelectionMenuController.TOOLBAR_MENU = 'toolbar-menu';

/**
 * @private
 */
SelectionMenuController.prototype.onShowMenu_ = function() {
  this.menu_.classList.toggle(SelectionMenuController.TOOLBAR_MENU, true);
  this.toggleRipple_.activated = true;
};

/**
 * @private
 */
SelectionMenuController.prototype.onHideMenu_ = function() {
  this.menu_.classList.toggle(SelectionMenuController.TOOLBAR_MENU, false);
  this.toggleRipple_.activated = false;
};
