// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * @enum {string}
 */
var NavigationModelItemType = {
  SHORTCUT: 'shortcut',
  VOLUME: 'volume',
  MENU: 'menu',
  RECENT: 'recent',
  CROSTINI: 'crostini',
};

/**
 * Base item of NavigationListModel. Should not be created directly.
 * @param {string} label
 * @param {NavigationModelItemType} type
 * @constructor
 * @struct
 */
function NavigationModelItem(label, type) {
  this.label_ = label;
  this.type_ = type;
}

NavigationModelItem.prototype = /** @struct */ {
  get label() { return this.label_; },
  get type() { return this.type_; }
};

/**
 * Item of NavigationListModel for shortcuts.
 *
 * @param {string} label Label.
 * @param {!DirectoryEntry} entry Entry. Cannot be null.
 * @constructor
 * @extends {NavigationModelItem}
 * @struct
 */
function NavigationModelShortcutItem(label, entry) {
  NavigationModelItem.call(this, label, NavigationModelItemType.SHORTCUT);
  this.entry_ = entry;
}

NavigationModelShortcutItem.prototype = /** @struct */ {
  __proto__: NavigationModelItem.prototype,
  get entry() { return this.entry_; }
};

/**
 * Item of NavigationListModel for volumes.
 *
 * @param {string} label Label.
 * @param {!VolumeInfo} volumeInfo Volume info for the volume. Cannot be null.
 * @constructor
 * @extends {NavigationModelItem}
 */
function NavigationModelVolumeItem(label, volumeInfo) {
  NavigationModelItem.call(this, label, NavigationModelItemType.VOLUME);
  this.volumeInfo_ = volumeInfo;
  // Start resolving the display root because it is used
  // for determining executability of commands.
  this.volumeInfo_.resolveDisplayRoot(
      function() {}, function() {});
}

NavigationModelVolumeItem.prototype = /** @struct */ {
  __proto__: NavigationModelItem.prototype,
  get volumeInfo() { return this.volumeInfo_; }
};

/**
 * Item of NavigationListModel for a menu button.
 *
 * @param {string} label Label on the menu button.
 * @param {string} menu Selector for the menu element.
 * @param {string} icon Name of an icon on the menu button.
 * @constructor
 * @extends {NavigationModelItem}
 * @struct
 */
function NavigationModelMenuItem(label, menu, icon) {
  NavigationModelItem.call(this, label, NavigationModelItemType.MENU);

  /**
   * @private {string}
   * @const
   */
  this.menu_ = menu;

  /**
   * @private {string}
   * @const
   */
  this.icon_ = icon;
}

NavigationModelMenuItem.prototype = /** @struct */ {
  __proto__: NavigationModelItem.prototype,
  /**
   * @return {string}
   */
  get menu() { return this.menu_; },

  /**
   * @return {string}
   */
  get icon() { return this.icon_; }
};

/**
 * Item of NavigationListModel for a fake item such as Recent or Linux Files.
 *
 * @param {string} label Label on the menu button.
 * @param {NavigationModelItemType} type
 * @param {!FakeEntry} entry Fake entry for the root folder.
 * @constructor
 * @extends {NavigationModelItem}
 * @struct
 */
function NavigationModelFakeItem(label, type, entry) {
  NavigationModelItem.call(this, label, type);
  this.entry_ = entry;
}

NavigationModelFakeItem.prototype = /** @struct */ {
  __proto__: NavigationModelItem.prototype,
  get entry() {
    return this.entry_;
  }
};

/**
 * A navigation list model. This model combines multiple models.
 * @param {!VolumeManagerWrapper} volumeManager VolumeManagerWrapper instance.
 * @param {(!cr.ui.ArrayDataModel|!FolderShortcutsDataModel)} shortcutListModel
 *     The list of folder shortcut.
 * @param {NavigationModelFakeItem} recentModelItem Recent folder.
 * @param {NavigationModelMenuItem} addNewServicesItem Add new services item.
 * @constructor
 * @extends {cr.EventTarget}
 */
function NavigationListModel(
    volumeManager, shortcutListModel, recentModelItem, addNewServicesItem) {
  cr.EventTarget.call(this);

  /**
   * @private {!VolumeManagerWrapper}
   * @const
   */
  this.volumeManager_ = volumeManager;

  /**
   * @private {(!cr.ui.ArrayDataModel|!FolderShortcutsDataModel)}
   * @const
   */
  this.shortcutListModel_ = shortcutListModel;

  /**
   * @private {NavigationModelFakeItem}
   * @const
   */
  this.recentModelItem_ = recentModelItem;

  /**
   * Root folder for crostini Linux Files.
   * This field will be set asynchronously after calling
   * chrome.fileManagerPrivate.isCrostiniEnabled.
   * @private {NavigationModelFakeItem}
   */
  this.linuxFilesItem_ = null;

  /**
   * @private {NavigationModelMenuItem}
   * @const
   */
  this.addNewServicesItem_ = addNewServicesItem;

  /**
   * All root navigation items in display order.
   * @private {!Array<!NavigationModelItem>}
   */
  this.navigationItems_ = [];

  var volumeInfoToModelItem = function(volumeInfo) {
    return new NavigationModelVolumeItem(
        volumeInfo.label,
        volumeInfo);
  }.bind(this);

  var entryToModelItem = function(entry) {
    var item = new NavigationModelShortcutItem(
        entry.name,
        entry);
    return item;
  }.bind(this);

  /**
   * Type of updated list.
   * @enum {number}
   * @const
   */
  var ListType = {
    VOLUME_LIST: 1,
    SHORTCUT_LIST: 2
  };
  Object.freeze(ListType);

  // Generates this.volumeList_ and this.shortcutList_ from the models.
  this.volumeList_ =
      this.volumeManager_.volumeInfoList.slice().map(volumeInfoToModelItem);

  this.shortcutList_ = [];
  for (var i = 0; i < this.shortcutListModel_.length; i++) {
    var shortcutEntry = /** @type {!Entry} */ (this.shortcutListModel_.item(i));
    var volumeInfo = this.volumeManager_.getVolumeInfo(shortcutEntry);
    this.shortcutList_.push(entryToModelItem(shortcutEntry));
  }

  // Reorder volumes, shortcuts, and optional items for initial display.
  this.reorderNavigationItems_();

  // Generates a combined 'permuted' event from an event of either volumeList or
  // shortcutList.
  var permutedHandler = function(listType, event) {
    var permutation;

    // Build the volumeList.
    if (listType == ListType.VOLUME_LIST) {
      // The volume is mounted or unmounted.
      var newList = [];

      // Use the old instances if they just move.
      for (var i = 0; i < event.permutation.length; i++) {
        if (event.permutation[i] >= 0)
          newList[event.permutation[i]] = this.volumeList_[i];
      }

      // Create missing instances.
      for (var i = 0; i < event.newLength; i++) {
        if (!newList[i]) {
          newList[i] = volumeInfoToModelItem(
              this.volumeManager_.volumeInfoList.item(i));
        }
      }
      this.volumeList_ = newList;

      permutation = event.permutation.slice();

      // shortcutList part has not been changed, so the permutation should be
      // just identity mapping with a shift.
      for (var i = 0; i < this.shortcutList_.length; i++) {
        permutation.push(i + this.volumeList_.length);
      }
    } else {
      // Build the shortcutList.

      // volumeList part has not been changed, so the permutation should be
      // identity mapping.

      permutation = [];
      for (var i = 0; i < this.volumeList_.length; i++) {
        permutation[i] = i;
      }

      var modelIndex = 0;
      var oldListIndex = 0;
      var newList = [];
      while (modelIndex < this.shortcutListModel_.length &&
             oldListIndex < this.shortcutList_.length) {
        var shortcutEntry = this.shortcutListModel_.item(modelIndex);
        var cmp = this.shortcutListModel_.compare(
            /** @type {Entry} */ (shortcutEntry),
            this.shortcutList_[oldListIndex].entry);
        if (cmp > 0) {
          // The shortcut at shortcutList_[oldListIndex] is removed.
          permutation.push(-1);
          oldListIndex++;
          continue;
        }

        if (cmp === 0) {
          // Reuse the old instance.
          permutation.push(newList.length + this.volumeList_.length);
          newList.push(this.shortcutList_[oldListIndex]);
          oldListIndex++;
        } else {
          // We needs to create a new instance for the shortcut entry.
          newList.push(entryToModelItem(shortcutEntry));
        }
        modelIndex++;
      }

      // Add remaining (new) shortcuts if necessary.
      for (; modelIndex < this.shortcutListModel_.length; modelIndex++) {
        var shortcutEntry = this.shortcutListModel_.item(modelIndex);
        newList.push(entryToModelItem(shortcutEntry));
      }

      // Fill remaining permutation if necessary.
      for (; oldListIndex < this.shortcutList_.length; oldListIndex++)
        permutation.push(-1);

      this.shortcutList_ = newList;
    }

    // Reorder items after permutation.
    this.reorderNavigationItems_();

    // Dispatch permuted event.
    var permutedEvent = new Event('permuted');
    permutedEvent.newLength =
        this.volumeList_.length + this.shortcutList_.length;
    permutedEvent.permutation = permutation;
    this.dispatchEvent(permutedEvent);
  };

  this.volumeManager_.volumeInfoList.addEventListener(
      'permuted', permutedHandler.bind(this, ListType.VOLUME_LIST));
  this.shortcutListModel_.addEventListener(
      'permuted', permutedHandler.bind(this, ListType.SHORTCUT_LIST));

  // 'change' event is just ignored, because it is not fired neither in
  // the folder shortcut list nor in the volume info list.
  // 'splice' and 'sorted' events are not implemented, since they are not used
  // in list.js.
}

/**
 * NavigationList inherits cr.EventTarget.
 */
NavigationListModel.prototype = {
  __proto__: cr.EventTarget.prototype,
  get length() {
    return this.length_();
  },
  get folderShortcutList() {
    return this.shortcutList_;
  },
  /**
   * Set the crostini Linux Files root and reorder items.
   * This setter is provided separate to the constructor since
   * this field is set async after calling fileManagerPrivate.isCrostiniEnabled.
   * @param {NavigationModelFakeItem} item Linux Files root.
   */
  set linuxFilesItem(item) {
    this.linuxFilesItem_ = item;
    this.reorderNavigationItems_();
  },
};

/**
 * Reorder navigation items in the following order:
 *  1. Volumes.
 *  2. If Downloads exists, then immediately after Downloads should be:
 *  2a. Recent if it exists.
 *  2b. Linux Files if it exists and is not mounted.
 *      When mounted, it will be located in Volumes at this position.
 *  3. Shortcuts.
 *  4. Add new services if it exists.
 * @private
 */
NavigationListModel.prototype.reorderNavigationItems_ = function() {
  // Check if Linux files already mounted.
  let linuxFilesMounted = false;
  for (let i = 0; i < this.volumeList_.length; i++) {
    if (this.volumeList_[i].volumeInfo.volumeType ===
        VolumeManagerCommon.VolumeType.CROSTINI) {
      linuxFilesMounted = true;
      break;
    }
  }

  // Items as per required order.
  this.navigationItems_ = this.volumeList_.slice();
  var downloadsVolumeIndex = this.findDownloadsVolumeIndex_();
  if (this.linuxFilesItem_ && !linuxFilesMounted && downloadsVolumeIndex >= 0)
    this.navigationItems_.splice(
        downloadsVolumeIndex + 1, 0, this.linuxFilesItem_);
  if (this.recentModelItem_ && downloadsVolumeIndex >= 0)
    this.navigationItems_.splice(
        downloadsVolumeIndex + 1, 0, this.recentModelItem_);
  Array.prototype.push.apply(this.navigationItems_, this.shortcutList_);
  if (this.addNewServicesItem_)
    this.navigationItems_.push(this.addNewServicesItem_);
};

/**
 * Returns the item at the given index.
 * @param {number} index The index of the entry to get.
 * @return {NavigationModelItem|undefined} The item at the given index.
 */
NavigationListModel.prototype.item = function(index) {
  return this.navigationItems_[index];
};

/**
 * Returns the number of items in the model.
 * @return {number} The length of the model.
 * @private
 */
NavigationListModel.prototype.length_ = function() {
  return this.navigationItems_.length;
};

/**
 * Returns the first matching item.
 * @param {NavigationModelItem} modelItem The entry to find.
 * @param {number=} opt_fromIndex If provided, then the searching start at
 *     the {@code opt_fromIndex}.
 * @return {number} The index of the first found element or -1 if not found.
 */
NavigationListModel.prototype.indexOf = function(modelItem, opt_fromIndex) {
  for (var i = opt_fromIndex || 0; i < this.length; i++) {
    if (modelItem === this.item(i))
      return i;
  }
  return -1;
};

/**
 * Called externally when one of the items is not found on the filesystem.
 * @param {!NavigationModelItem} modelItem The entry which is not found.
 */
NavigationListModel.prototype.onItemNotFoundError = function(modelItem) {
  if (modelItem.type ===  NavigationModelItemType.SHORTCUT)
    this.shortcutListModel_.onItemNotFoundError(
        /** @type {!NavigationModelShortcutItem} */(modelItem).entry);
};

/**
 * Get the index of Downloads volume in the volume list. Returns -1 if there is
 * not the Downloads volume in the list.
 * @returns {number} Index of the Downloads volume.
 */
NavigationListModel.prototype.findDownloadsVolumeIndex_ = function() {
  for (var i = 0; i < this.volumeList_.length; i++) {
    if (this.volumeList_[i].volumeInfo.volumeType ==
        VolumeManagerCommon.VolumeType.DOWNLOADS) {
      return i;
    }
  }
  return -1;
};
