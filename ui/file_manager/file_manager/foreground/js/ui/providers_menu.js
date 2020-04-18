// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * Fills out the menu for mounting or installing new providers.
 *
 * @param {!ProvidersModel} model
 * @param {!cr.ui.Menu} menu
 * @constructor
 * @struct
 */
function ProvidersMenu(model, menu) {
  /**
   * @private {!ProvidersModel}}
   * @const
   */
  this.model_ = model;

  /**
   * @private {!cr.ui.Menu}
   * @const
   */
  this.menu_ = menu;

  this.menu_.addSeparator();

  /**
   * @private {!Element}
   * @const
   */
  this.separator_ = assert(this.menu_.firstElementChild);

  var installItem = this.addMenuItem_();
  installItem.command = '#install-new-extension';

  this.menu_.addEventListener('update', this.onUpdate_.bind(this));
}

/**
 * @private
 */
ProvidersMenu.prototype.clearProviders_ = function() {
  var childNode = this.menu_.firstElementChild;
  while (childNode !== this.separator_) {
    var node = childNode;
    childNode = childNode.nextElementSibling;
    this.menu_.removeChild(node);
  }
};

/**
 * @return {!cr.ui.FilesMenuItem}
 * @private
 */
ProvidersMenu.prototype.addMenuItem_ = function() {
  var menuItem = this.menu_.addMenuItem({});
  cr.ui.decorate(/** @type {!Element} */ (menuItem), cr.ui.FilesMenuItem);
  return /** @type {!cr.ui.FilesMenuItem} */ (menuItem);
};

/**
 * @param {string} providerId ID of the provider.
 * @param {!IconSet} iconSet Set of icons for the provider.
 * @param {string} name Already localized name of the provider.
 * @private
 */
ProvidersMenu.prototype.addProvider_ = function(providerId, iconSet, name) {
  var item = this.addMenuItem_();
  item.label = name;

  var iconImage = util.iconSetToCSSBackgroundImageValue(iconSet);
  item.iconStartImage = iconImage;

  item.addEventListener(
      'activate', this.onItemActivate_.bind(this, providerId));

  // Move the element before the separator.
  this.menu_.insertBefore(item, this.separator_);
};

/**
 * @param {!Event} event
 * @private
 */
ProvidersMenu.prototype.onUpdate_ = function(event) {
  this.model_.getMountableProviders().then(function(providers) {
    this.clearProviders_();
    providers.forEach(function(provider) {
      this.addProvider_(provider.providerId, provider.iconSet, provider.name);
    }.bind(this));

    // Reposition the menu, so all items are always visible.
    cr.ui.positionPopupAroundElement(event.menuButton, this.menu_,
        event.menuButton.anchorType, event.menuButton.invertLeftRight);
  }.bind(this));
};

/**
 * @param {string} providerId
 * @param {!Event} event
 * @private
 */
ProvidersMenu.prototype.onItemActivate_ = function(providerId, event) {
  this.model_.requestMount(providerId);
};
