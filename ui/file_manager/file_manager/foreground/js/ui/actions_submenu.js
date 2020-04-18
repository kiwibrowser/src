// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * @param {!cr.ui.Menu} menu
 * @constructor
 * @struct
 */
function ActionsSubmenu(menu) {
  /**
   * @private {!cr.ui.Menu}
   * @const
   */
  this.menu_ = menu;

  /**
   * @private {!cr.ui.MenuItem}
   * @const
   */
  this.separator_ = /** @type {!cr.ui.MenuItem} */
      (queryRequiredElement('#actions-separator', this.menu_));

  /**
   * @private {!Array<!cr.ui.MenuItem>}
   */
  this.items_ = [];
}

/**
 * @param {!Object} options
 * @return {cr.ui.MenuItem}
 * @private
 */
ActionsSubmenu.prototype.addMenuItem_ = function(options) {
  var menuItem = this.menu_.addMenuItem(options);
  menuItem.parentNode.insertBefore(menuItem, this.separator_);
  this.items_.push(menuItem);
  return menuItem;
};

/**
 * @param {ActionsModel} actionsModel
 */
ActionsSubmenu.prototype.setActionsModel = function(actionsModel) {
  this.items_.forEach(function(item) {
    item.parentNode.removeChild(item);
  });
  this.items_ = [];

  var remainingActions = {};
  if (actionsModel) {
    var actions = actionsModel.getActions();
     Object.keys(actions).forEach(
        function(key) {
          remainingActions[key] = actions[key];
        });
  }

  // First add the sharing item (if available).
  var shareAction = remainingActions[ActionsModel.CommonActionId.SHARE];
  if (shareAction) {
    var menuItem = this.addMenuItem_({});
    menuItem.command = '#share';
    menuItem.classList.toggle('hide-on-toolbar', true);
    delete remainingActions[ActionsModel.CommonActionId.SHARE];
  }
  util.queryDecoratedElement('#share', cr.ui.Command).canExecuteChange();

  // Then add the Manage in Drive item (if available).
  var manageInDriveAction =
      remainingActions[ActionsModel.InternalActionId.MANAGE_IN_DRIVE];
  if (manageInDriveAction) {
    var menuItem = this.addMenuItem_({});
    menuItem.command = '#manage-in-drive';
    menuItem.classList.toggle('hide-on-toolbar', true);
    delete remainingActions[ActionsModel.InternalActionId.MANAGE_IN_DRIVE];
  }
  util.queryDecoratedElement('#manage-in-drive', cr.ui.Command)
      .canExecuteChange();

  // Managing shortcuts is shown just before custom actions.
  var createFolderShortcutAction = remainingActions[
      ActionsModel.InternalActionId.CREATE_FOLDER_SHORTCUT];
  if (createFolderShortcutAction) {
    var menuItem = this.addMenuItem_({});
    menuItem.command = '#create-folder-shortcut';
    delete remainingActions[
      ActionsModel.InternalActionId.CREATE_FOLDER_SHORTCUT
    ];
  }
  util.queryDecoratedElement(
      '#create-folder-shortcut', cr.ui.Command).canExecuteChange();

  // Removing shortcuts is not rendered in the submenu to keep the previous
  // behavior. Shortcuts can be removed in the left nav using the roots menu.
  // TODO(mtomasz): Consider rendering the menu item here for consistency.
  util.queryDecoratedElement(
      '#remove-folder-shortcut', cr.ui.Command).canExecuteChange();

  // Both save-for-offline and offline-not-necessary are handled by the single
  // #toggle-pinned command.
  var saveForOfflineAction = remainingActions[
      ActionsModel.CommonActionId.SAVE_FOR_OFFLINE];
  var offlineNotNecessaryAction = remainingActions[
      ActionsModel.CommonActionId.OFFLINE_NOT_NECESSARY];
  if (saveForOfflineAction || offlineNotNecessaryAction) {
    var menuItem = this.addMenuItem_({});
    menuItem.command = '#toggle-pinned';
    if (saveForOfflineAction)
      delete remainingActions[ActionsModel.CommonActionId.SAVE_FOR_OFFLINE];
    if (offlineNotNecessaryAction) {
      delete remainingActions[
        ActionsModel.CommonActionId.OFFLINE_NOT_NECESSARY
      ];
    }
  }
  util.queryDecoratedElement(
      '#toggle-pinned', cr.ui.Command).canExecuteChange();

  // Process all the rest as custom actions.
  Object.keys(remainingActions).forEach(function(key) {
    var action = remainingActions[key];
    var options = { label: action.getTitle() };
    var menuItem = this.addMenuItem_(options);

    menuItem.addEventListener('activate', function() {
      action.execute();
    });
  }.bind(this));

  this.separator_.hidden = !this.items_.length;
};
