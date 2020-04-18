// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

////////////////////////////////////////////////////////////////////////////////
// DirectoryTreeBase

/**
 * Implementation of methods for DirectoryTree and DirectoryItem. These classes
 * inherits cr.ui.Tree/TreeItem so we can't make them inherit this class.
 * Instead, we separate their implementations to this separate object and call
 * it with setting 'this' from DirectoryTree/Item.
 */
var DirectoryItemTreeBaseMethods = {};

/**
 * Finds an item by entry and returns it.
 * @param {!Entry} entry
 * @return {DirectoryItem} null is returned if it's not found.
 * @this {(DirectoryItem|DirectoryTree)}
 */
DirectoryItemTreeBaseMethods.getItemByEntry = function(entry) {
  for (var i = 0; i < this.items.length; i++) {
    var item = this.items[i];
    if (!item.entry)
      continue;
    if (util.isSameEntry(item.entry, entry)) {
      // The Drive root volume item "Google Drive" and its child "My Drive" have
      // the same entry. When we look for a tree item of Drive's root directory,
      // "My Drive" should be returned, as we use "Google Drive" for grouping
      // "My Drive", "Shared with me", "Recent", and "Offline".
      // Therefore, we have to skip "Google Drive" here.
      if (item instanceof DriveVolumeItem)
        return item.getItemByEntry(entry);

      return item;
    }
    // Team drives are descendants of the Drive root volume item "Google Drive".
    // When we looking for an item in team drives, recursively search inside the
    // "Google Drive" root item.
    if (util.isTeamDriveEntry(entry) && item instanceof DriveVolumeItem)
      return item.getItemByEntry(entry);

    if (util.isDescendantEntry(item.entry, entry))
      return item.getItemByEntry(entry);
  }
  return null;
};

/**
 * Finds a parent directory of the {@code entry} in {@code this}, and
 * invokes the DirectoryItem.selectByEntry() of the found directory.
 *
 * @param {!DirectoryEntry|!FakeEntry} entry The entry to be searched for. Can
 *     be a fake.
 * @return {boolean} True if the parent item is found.
 * @this {(DirectoryItem|VolumeItem|DirectoryTree)}
 */
DirectoryItemTreeBaseMethods.searchAndSelectByEntry = function(entry) {
  for (var i = 0; i < this.items.length; i++) {
    var item = this.items[i];
    if (!item.entry)
      continue;

    // Team drives are descendants of the Drive root volume item "Google Drive".
    // When we looking for an item in team drives, recursively search inside the
    // "Google Drive" root item.
    if (util.isTeamDriveEntry(entry) && item instanceof DriveVolumeItem) {
      item.selectByEntry(entry);
      return true;
    }

    if (util.isDescendantEntry(item.entry, entry) ||
        util.isSameEntry(item.entry, entry)) {
      item.selectByEntry(entry);
      return true;
    }
  }
  return false;
};
/**
 * Records UMA for the selected entry at {@code location}. Records slightly
 * differently if the expand icon is selected and {@code expandIconSelected} is
 * true.
 *
 * @param {Event} e The click event.
 * @param {VolumeManagerCommon.RootType} rootType The root type to record.
 * @param {boolean} isRootEntry Whether the entry selected was a root entry.
 * @return
 */
DirectoryItemTreeBaseMethods.recordUMASelectedEntry = function(
    e, rootType, isRootEntry) {
  var expandIconSelected = e.target.classList.contains('expand-icon');
  var metricName = 'Location.OnEntrySelected.TopLevel';
  if (!expandIconSelected && isRootEntry) {
    metricName = 'Location.OnEntrySelected.TopLevel';
  } else if (!expandIconSelected && !isRootEntry) {
    metricName = 'Location.OnEntrySelected.NonTopLevel';
  } else if (expandIconSelected && isRootEntry) {
    metricName = 'Location.OnEntryExpandedOrCollapsed.TopLevel';
  } else if (expandIconSelected && !isRootEntry) {
    metricName = 'Location.OnEntryExpandedOrCollapsed.NonTopLevel';
  }

  metrics.recordEnum(metricName, rootType, VolumeManagerCommon.RootTypesForUMA);
};

Object.freeze(DirectoryItemTreeBaseMethods);

var TREE_ITEM_INNER_HTML =
    '<div class="tree-row">' +
    ' <paper-ripple fit class="recenteringTouch"></paper-ripple>' +
    ' <span class="expand-icon"></span>' +
    ' <span class="icon"></span>' +
    ' <span class="label entry-name"></span>' +
    '</div>' +
    '<div class="tree-children"></div>';

var MENU_TREE_ITEM_INNER_HTML =
    '<div class="tree-row">' +
    ' <paper-ripple fit class="recenteringTouch"></paper-ripple>' +
    ' <span class="expand-icon"></span>' +
    ' <div class="button">' +
    '  <span class="icon item-icon"></span>' +
    '  <span class="label entry-name"></span>' +
    ' </div>' +
    '</div>' +
    '<div class="tree-children"></div>';

////////////////////////////////////////////////////////////////////////////////
// DirectoryItem

/**
 * An expandable directory in the tree. Each element represents one folder (sub
 * directory) or one volume (root directory).
 *
 * @param {string} label Label for this item.
 * @param {DirectoryTree} tree Current tree, which contains this item.
 * @extends {cr.ui.TreeItem}
 * @constructor
 */
function DirectoryItem(label, tree) {
  var item = /** @type {DirectoryItem} */ (new cr.ui.TreeItem());
  // Get the original label id defined by TreeItem, before overwriting
  // prototype.
  var labelId = item.labelElement.id;
  item.__proto__ = DirectoryItem.prototype;
  item.parentTree_ = tree;
  item.directoryModel_ = tree.directoryModel;
  item.fileFilter_ = tree.directoryModel.getFileFilter();

  item.innerHTML = TREE_ITEM_INNER_HTML;
  item.labelElement.id = labelId;
  item.addEventListener('expand', item.onExpand_.bind(item), false);

  // Listen for collapse because for the delayed expansion case all
  // children are also collapsed.
  item.addEventListener('collapse', item.onCollapse_.bind(item), false);

  // Default delayExpansion to false. Volumes will set it to true for
  // provided file systems. SubDirectories will inherit from their
  // parent.
  item.delayExpansion = false;

  // Sets hasChildren=false tentatively. This will be overridden after
  // scanning sub-directories in updateSubElementsFromList().
  item.hasChildren = false;

  item.label = label;

  return item;
}

DirectoryItem.prototype = {
  __proto__: cr.ui.TreeItem.prototype,

  /**
   * The DirectoryEntry corresponding to this DirectoryItem. This may be
   * a dummy DirectoryEntry.
   * @type {DirectoryEntry|Object}
   */
  get entry() {
    return null;
  },

  /**
   * The element containing the label text and the icon.
   * @type {!HTMLElement}
   * @override
   */
  get labelElement() {
    return this.firstElementChild.querySelector('.label');
  }
};

/**
 * Updates sub-elements of {@code this} reading {@code DirectoryEntry}.
 * The list of {@code DirectoryEntry} are not updated by this method.
 *
 * @param {boolean} recursive True if the all visible sub-directories are
 *     updated recursively including left arrows. If false, the update walks
 *     only immediate child directories without arrows.
 * @this {DirectoryItem}
 */
DirectoryItem.prototype.updateSubElementsFromList = function(recursive) {
  var index = 0;
  var tree = this.parentTree_;
  while (this.entries_[index]) {
    var currentEntry = this.entries_[index];
    var currentElement = this.items[index];
    var label = util.getEntryLabel(
        tree.volumeManager_.getLocationInfo(currentEntry),
        currentEntry) || '';

    if (index >= this.items.length) {
      var item = new SubDirectoryItem(label, currentEntry, this, tree);
      this.add(item);
      index++;
    } else if (util.isSameEntry(currentEntry, currentElement.entry)) {
      currentElement.updateSharedStatusIcon();
      if (recursive && this.expanded) {
        if (this.delayExpansion) {
          // Only update deeper on expanded children.
          if (currentElement.expanded) {
            currentElement.updateSubDirectories(true /* recursive */);
          }

          // Show the expander even without knowing if there are children.
          currentElement.mayHaveChildren_ = true;
        } else {
          currentElement.updateSubDirectories(true /* recursive */);
        }
      }
      index++;
    } else if (currentEntry.toURL() < currentElement.entry.toURL()) {
      var item = new SubDirectoryItem(label, currentEntry, this, tree);
      this.addAt(item, index);
      index++;
    } else if (currentEntry.toURL() > currentElement.entry.toURL()) {
      this.remove(currentElement);
    }
  }

  var removedChild;
  while (removedChild = this.items[index]) {
    this.remove(removedChild);
  }

  if (index === 0) {
    this.hasChildren = false;
    this.expanded = false;
  } else {
    this.hasChildren = true;
  }
};

/**
 * Calls DirectoryItemTreeBaseMethods.getItemByEntry().
 * @param {!Entry} entry
 * @return {DirectoryItem}
 */
DirectoryItem.prototype.getItemByEntry = function(entry) {
  return DirectoryItemTreeBaseMethods.getItemByEntry.call(this, entry);
};

/**
 * Calls DirectoryItemTreeBaseMethods.updateSubElementsFromList().
 *
 * @param {!DirectoryEntry|!FakeEntry} entry The entry to be searched for. Can
 *     be a fake.
 * @return {boolean} True if the parent item is found.
 */
DirectoryItem.prototype.searchAndSelectByEntry = function(entry) {
  return DirectoryItemTreeBaseMethods.searchAndSelectByEntry.call(this, entry);
};

/**
 * Overrides WebKit's scrollIntoViewIfNeeded, which doesn't work well with
 * a complex layout. This call is not necessary, so we are ignoring it.
 *
 * @param {boolean=} opt_unused Unused.
 * @override
 */
DirectoryItem.prototype.scrollIntoViewIfNeeded = function(opt_unused) {
};

/**
 * Removes the child node, but without selecting the parent item, to avoid
 * unintended changing of directories. Removing is done externally, and other
 * code will navigate to another directory.
 *
 * @param {!cr.ui.TreeItem=} child The tree item child to remove.
 * @override
 */
DirectoryItem.prototype.remove = function(child) {
  this.lastElementChild.removeChild(/** @type {!cr.ui.TreeItem} */(child));
  if (this.items.length == 0)
    this.hasChildren = false;
};


/**
 * Removes the has-children attribute which allows returning
 * to the ambiguous may-have-children state.
 */
DirectoryItem.prototype.clearHasChildren = function() {
  var rowItem = this.firstElementChild;
  this.removeAttribute('has-children');
  rowItem.removeAttribute('has-children');
};


/**
 * Invoked when the item is being expanded.
 * @param {!Event} e Event.
 * @private
 */
DirectoryItem.prototype.onExpand_ = function(e) {
  this.updateSubDirectories(
      true /* recursive */,
      function() {},
      function() {
        this.expanded = false;
      }.bind(this));

  e.stopPropagation();
};


/**
 * Invoked when the item is being collapsed.
 * @param {!Event} e Event.
 * @private
 */
DirectoryItem.prototype.onCollapse_ = function(e) {
  if (this.delayExpansion) {
    // For file systems where it is performance intensive
    // to update recursively when items expand this proactively
    // collapses all children to avoid having to traverse large
    // parts of the tree when reopened.
    for (var i = 0; i < this.items.length; i++) {
      var item = this.items[i];

      if (item.expanded) {
        item.expanded = false;
      }
    }
  }

  e.stopPropagation();
};

/**
 * Invoked when the tree item is clicked.
 *
 * @param {Event} e Click event.
 * @override
 */
DirectoryItem.prototype.handleClick = function(e) {
  cr.ui.TreeItem.prototype.handleClick.call(this, e);

  if (!this.entry || e.button === 2) {
    return;
  }

  if (!e.target.classList.contains('expand-icon')) {
    this.directoryModel_.activateDirectoryEntry(this.entry);
  }

  // If this is DriveVolumeItem, the UMA has already been recorded.
  if (!(this instanceof DriveVolumeItem)) {
    var location = this.tree.volumeManager.getLocationInfo(this.entry);
    DirectoryItemTreeBaseMethods.recordUMASelectedEntry.call(
        this, e, location.rootType, location.isRootEntry);
  }
};

/**
 * Retrieves the latest subdirectories and update them on the tree.
 * @param {boolean} recursive True if the update is recursively.
 * @param {function()=} opt_successCallback Callback called on success.
 * @param {function()=} opt_errorCallback Callback called on error.
 */
DirectoryItem.prototype.updateSubDirectories = function(
    recursive, opt_successCallback, opt_errorCallback) {
  if (!this.entry || util.isFakeEntry(this.entry)) {
    if (opt_errorCallback)
      opt_errorCallback();
    return;
  }

  var sortEntries = function(fileFilter, entries) {
    entries.sort(util.compareName);
    return entries.filter(fileFilter.filter.bind(fileFilter));
  };

  var onSuccess = function(entries) {
    this.entries_ = entries;
    this.updateSubElementsFromList(recursive);
    opt_successCallback && opt_successCallback();
  }.bind(this);

  var reader = this.entry.createReader();
  var entries = [];
  var readEntry = function() {
    reader.readEntries(function(results) {
      if (!results.length) {
        onSuccess(sortEntries(this.fileFilter_, entries));
        return;
      }

      for (var i = 0; i < results.length; i++) {
        var entry = results[i];
        if (entry.isDirectory)
          entries.push(entry);
      }
      readEntry();
    }.bind(this));
  }.bind(this);
  readEntry();
};

/**
 * Searches for the changed directory in the current subtree, and if it is found
 * then updates it.
 *
 * @param {!DirectoryEntry} changedDirectoryEntry The entry ot the changed
 *     directory.
 */
DirectoryItem.prototype.updateItemByEntry = function(changedDirectoryEntry) {
  if (util.isSameEntry(changedDirectoryEntry, this.entry)) {
    this.updateSubDirectories(false /* recursive */);
    return;
  }

  // Traverse the entire subtree to find the changed element.
  for (var i = 0; i < this.items.length; i++) {
    var item = this.items[i];
    if (!item.entry)
      continue;
    if (util.isDescendantEntry(item.entry, changedDirectoryEntry) ||
        util.isSameEntry(item.entry, changedDirectoryEntry)) {
      item.updateItemByEntry(changedDirectoryEntry);
      break;
    }
  }
};

/**
 * Update the icon based on whether the folder is shared on Drive.
 */
DirectoryItem.prototype.updateSharedStatusIcon = function() {
};

/**
 * Select the item corresponding to the given {@code entry}.
 * @param {!DirectoryEntry|!FakeEntry} entry The entry to be selected. Can be a
 *     fake.
 */
DirectoryItem.prototype.selectByEntry = function(entry) {
  if (util.isSameEntry(entry, this.entry)) {
    this.selected = true;
    return;
  }

  if (this.searchAndSelectByEntry(entry))
    return;

  // If the entry doesn't exist, updates sub directories and tries again.
  this.updateSubDirectories(
      false /* recursive */,
      this.searchAndSelectByEntry.bind(this, entry));
};

/**
 * Executes the assigned action as a drop target.
 */
DirectoryItem.prototype.doDropTargetAction = function() {
  this.expanded = true;
};

/**
 * Change current directory to the entry of this item.
 */
DirectoryItem.prototype.activate = function() {
  if (this.entry)
    this.parentTree_.directoryModel.activateDirectoryEntry(this.entry);
};

////////////////////////////////////////////////////////////////////////////////
// SubDirectoryItem

/**
 * A sub directory in the tree. Each element represents a directory which is not
 * a volume's root.
 *
 * @param {string} label Label for this item.
 * @param {DirectoryEntry} dirEntry DirectoryEntry of this item.
 * @param {DirectoryItem|ShortcutItem|DirectoryTree} parentDirItem
 *     Parent of this item.
 * @param {DirectoryTree} tree Current tree, which contains this item.
 * @extends {DirectoryItem}
 * @constructor
 */
function SubDirectoryItem(label, dirEntry, parentDirItem, tree) {
  var item = new DirectoryItem(label, tree);
  item.__proto__ = SubDirectoryItem.prototype;

  item.entry = dirEntry;
  item.delayExpansion = parentDirItem.delayExpansion;

  if (item.delayExpansion) {
    item.clearHasChildren();
    item.mayHaveChildren_ = true;
  }

  // Sets up icons of the item.
  var icon = item.querySelector('.icon');
  icon.classList.add('item-icon');
  var location = tree.volumeManager.getLocationInfo(item.entry);
  if (location && location.rootType && location.isRootEntry) {
    icon.setAttribute('volume-type-icon', location.rootType);
  } else {
    icon.setAttribute('file-type-icon', 'folder');
    item.updateSharedStatusIcon();
  }

  // Sets up context menu of the item.
  if (tree.contextMenuForSubitems && !util.isTeamDriveRoot(dirEntry))
    cr.ui.contextMenuHandler.setContextMenu(item, tree.contextMenuForSubitems);

  // Populates children now if needed.
  if (parentDirItem.expanded)
    item.updateSubDirectories(false /* recursive */);

  return item;
}

SubDirectoryItem.prototype = {
  __proto__: DirectoryItem.prototype,

  get entry() {
    return this.dirEntry_;
  },

  set entry(value) {
    this.dirEntry_ = value;

    // Set helper attribute for testing.
    if (window.IN_TEST)
      this.setAttribute('full-path-for-testing', this.dirEntry_.fullPath);
  }
};

/**
 * Update the icon based on whether the folder is shared on Drive.
 * @override
 */
SubDirectoryItem.prototype.updateSharedStatusIcon = function() {
  var icon = this.querySelector('.icon');
  this.parentTree_.metadataModel.notifyEntriesChanged([this.dirEntry_]);
  this.parentTree_.metadataModel.get([this.dirEntry_], ['shared']).then(
      function(metadata) {
        icon.classList.toggle('shared', !!(metadata[0] && metadata[0].shared));
      });
};

////////////////////////////////////////////////////////////////////////////////
// VolumeItem

/**
 * A TreeItem which represents a volume. Volume items are displayed as
 * top-level children of DirectoryTree.
 *
 * @param {!NavigationModelVolumeItem} modelItem NavigationModelItem of this
 *     volume.
 * @param {!DirectoryTree} tree Current tree, which contains this item.
 * @extends {DirectoryItem}
 * @constructor
 */
function VolumeItem(modelItem, tree) {
  var item = /** @type {VolumeItem} */ (
      new DirectoryItem(modelItem.volumeInfo.label, tree));
  item.__proto__ = VolumeItem.prototype;

  item.modelItem_ = modelItem;
  item.volumeInfo_ = modelItem.volumeInfo;

  // Provided volumes should delay the expansion of child nodes
  // for performance reasons.
  item.delayExpansion = (item.volumeInfo.volumeType === 'provided');

  // Set helper attribute for testing.
  if (window.IN_TEST)
    item.setAttribute('volume-type-for-testing', item.volumeInfo_.volumeType);

  item.setupIcon_(item.querySelector('.icon'), item.volumeInfo_);

  // Attach a placeholder for rename input text box and the eject icon if the
  // volume is ejectable
  if ((modelItem.volumeInfo_.source === VolumeManagerCommon.Source.DEVICE &&
       modelItem.volumeInfo_.volumeType !==
           VolumeManagerCommon.VolumeType.MTP) ||
      modelItem.volumeInfo_.source === VolumeManagerCommon.Source.FILE) {
    // This placeholder is added to allow to put textbox before eject button
    // while executing renaming action on external drive.
    item.setupRenamePlaceholder_(item.rowElement);
    item.setupEjectButton_(item.rowElement);
  }

  // Sets up context menu of the item.
  if (tree.contextMenuForRootItems)
    item.setContextMenu_(tree.contextMenuForRootItems);

  // Populate children of this volume using resolved display root.
  item.volumeInfo_.resolveDisplayRoot(function(displayRoot) {
    item.updateSubDirectories(false /* recursive */);
  });

  return item;
}

VolumeItem.prototype = {
  __proto__: DirectoryItem.prototype,
  /**
   * Directory entry for the display root, whose initial value is null.
   * @type {DirectoryEntry}
   * @override
   */
  get entry() {
    return this.volumeInfo_.displayRoot;
  },
  /**
   * @type {!VolumeInfo}
   */
  get volumeInfo() {
    return this.volumeInfo_;
  },
  /**
   * @type {!NavigationModelVolumeItem}
   */
  get modelItem() {
    return this.modelItem_;
  }
};

/**
 * Sets the context menu for volume items.
 * @param {!cr.ui.Menu} menu Menu to be set.
 * @private
 */
VolumeItem.prototype.setContextMenu_ = function(menu) {
  cr.ui.contextMenuHandler.setContextMenu(this, menu);
};

/**
 * Change current entry to this volume's root directory.
 * @override
 */
VolumeItem.prototype.activate = function() {
  var directoryModel = this.parentTree_.directoryModel;
  var onEntryResolved = function(entry) {
    // Changes directory to the model item's root directory if needed.
    if (!util.isSameEntry(directoryModel.getCurrentDirEntry(), entry)) {
      metrics.recordUserAction('FolderShortcut.Navigate');
      directoryModel.changeDirectoryEntry(entry);
    }
    // In case of failure in resolveDisplayRoot() in the volume's constructor,
    // update the volume's children here.
    this.updateSubDirectories(false);
  }.bind(this);

  this.volumeInfo_.resolveDisplayRoot(
      onEntryResolved,
      function() {
        // Error, the display root is not available. It may happen on Drive.
        this.parentTree_.dataModel.onItemNotFoundError(this.modelItem);
      }.bind(this));
};

/**
 * Set up icon of this volume item.
 * @param {Element} icon Icon element to be setup.
 * @param {VolumeInfo} volumeInfo VolumeInfo determines the icon type.
 * @private
 */
VolumeItem.prototype.setupIcon_ = function(icon, volumeInfo) {
  icon.classList.add('item-icon');
  var backgroundImage =
      util.iconSetToCSSBackgroundImageValue(volumeInfo.iconSet);
  if (backgroundImage !== 'none') {
    // The icon div is not yet added to DOM, therefore it is impossible to
    // use style.backgroundImage.
    icon.setAttribute(
        'style', 'background-image: ' + backgroundImage);
  }
  icon.setAttribute('volume-type-icon', volumeInfo.volumeType);
  if (volumeInfo.volumeType === VolumeManagerCommon.VolumeType.MEDIA_VIEW) {
    icon.setAttribute(
        'volume-subtype',
        VolumeManagerCommon.getMediaViewRootTypeFromVolumeId(
            volumeInfo.volumeId));
  } else {
    icon.setAttribute('volume-subtype', volumeInfo.deviceType || '');
  }
};

/**
 * Set up eject button if needed.
 * @param {HTMLElement} rowElement The parent element for eject button.
 * @private
 */
VolumeItem.prototype.setupEjectButton_ = function(rowElement) {
  var ejectButton = cr.doc.createElement('button');
  // Block other mouse handlers.
  ejectButton.addEventListener('mouseup', function(event) {
    event.stopPropagation();
  });
  ejectButton.addEventListener('mousedown', function(event) {
    event.stopPropagation();
  });
  ejectButton.className = 'root-eject';
  ejectButton.setAttribute('aria-label', str('UNMOUNT_DEVICE_BUTTON_LABEL'));
  ejectButton.setAttribute('tabindex', '0');
  ejectButton.addEventListener('click', function(event) {
    event.stopPropagation();
    var unmountCommand = cr.doc.querySelector('command#unmount');
    // Let's make sure 'canExecute' state of the command is properly set for
    // the root before executing it.
    unmountCommand.canExecuteChange(this);
    unmountCommand.execute(this);
  }.bind(this));
  rowElement.appendChild(ejectButton);

  // Add paper-ripple effect on the eject button.
  var ripple = cr.doc.createElement('paper-ripple');
  ripple.setAttribute('fit', '');
  ripple.className = 'circle recenteringTouch';
  ejectButton.appendChild(ripple);
};

/**
 * Set up rename input textbox placeholder if needed.
 * @param {HTMLElement} rowElement The parent element for placeholder.
 * @private
 */
VolumeItem.prototype.setupRenamePlaceholder_ = function(rowElement) {
  var placeholder = cr.doc.createElement('span');
  placeholder.className = 'rename-placeholder';
  rowElement.appendChild(placeholder);
};

////////////////////////////////////////////////////////////////////////////////
// DriveVolumeItem

/**
 * A TreeItem which represents a Drive volume. Drive volume has fake entries
 * such as Team Drives, Shared with me, and Offline in it.
 *
 * @param {!NavigationModelVolumeItem} modelItem NavigationModelItem of this
 *     volume.
 * @param {!DirectoryTree} tree Current tree, which contains this item.
 * @extends {VolumeItem}
 * @constructor
 */
function DriveVolumeItem(modelItem, tree) {
  var item = new VolumeItem(modelItem, tree);
  item.__proto__ = DriveVolumeItem.prototype;
  item.classList.add('drive-volume');
  return item;
}

DriveVolumeItem.prototype = {
  __proto__: VolumeItem.prototype,
  // Overrides the property 'expanded' to prevent Drive volume from shrinking.
  get expanded() {
    return Object.getOwnPropertyDescriptor(
        cr.ui.TreeItem.prototype, 'expanded').get.call(this);
  },
  set expanded(b) {
    Object.getOwnPropertyDescriptor(
        cr.ui.TreeItem.prototype, 'expanded').set.call(this, b);
    // When Google Drive is expanded while it is selected, select the My Drive.
    if (b) {
      if (this.selected && this.entry)
        this.selectByEntry(this.entry);
    }
  }
};

/**
 * Invoked when the tree item is clicked.
 *
 * @param {Event} e Click event.
 * @override
 */
DriveVolumeItem.prototype.handleClick = function(e) {
  VolumeItem.prototype.handleClick.call(this, e);

  if (!e.target.classList.contains('expand-icon')) {
    // If the Drive volume is clicked, select one of the children instead of
    // this item itself.
    this.volumeInfo_.resolveDisplayRoot(function(displayRoot) {
      this.searchAndSelectByEntry(displayRoot);
    }.bind(this));
  }

  DirectoryItemTreeBaseMethods.recordUMASelectedEntry.call(
      this, e, VolumeManagerCommon.RootType.DRIVE_FAKE_ROOT, true);
};

/**
 * Checks whether the Team Drives grand root should be shown.
 * @param {function(boolean)} callback to receive the result. The paramter is
 *     true if the Files app. should show the Team Drives grand root and its
 *     subtree.
 * @private
 */
DriveVolumeItem.prototype.shouldShowTeamDrives_ = function(callback) {
  var teamDriveEntry = this.volumeInfo_.teamDriveDisplayRoot;
  if (!teamDriveEntry) {
    callback(false);
  } else {
    var reader = teamDriveEntry.createReader();
    reader.readEntries(function(results) {
      callback(results.length > 0);
    });
  }
};

/**
 * Retrieves the latest subdirectories and update them on the tree.
 * @param {boolean} recursive True if the update is recursively.
 * @override
 */
DriveVolumeItem.prototype.updateSubDirectories = function(recursive) {
  if (!this.entry || this.hasChildren)
    return;
  this.shouldShowTeamDrives_(function(shouldShowTeamDrives) {
    var entries = [this.entry];
    if (shouldShowTeamDrives)
      entries.push(this.volumeInfo_.teamDriveDisplayRoot);
    // Drive volume has children including fake entries (offline, recent, ...)
    var fakeEntries = [];
    if (this.parentTree_.fakeEntriesVisible_) {
      for (var key in this.volumeInfo_.fakeEntries)
        fakeEntries.push(this.volumeInfo_.fakeEntries[key]);
      // This list is sorted by URL on purpose.
      fakeEntries.sort(function(a, b) {
        if (a.toURL() === b.toURL())
          return 0;
        return b.toURL() > a.toURL() ? 1 : -1;
      });
      entries = entries.concat(fakeEntries);
    }

    for (var i = 0; i < entries.length; i++) {
      var item = new SubDirectoryItem(
          util.getEntryLabel(
              this.parentTree_.volumeManager_.getLocationInfo(entries[i]),
              entries[i]) || '',
          entries[i], this, this.parentTree_);
      this.add(item);
      item.updateSubDirectories(false);
    }
    this.expanded = true;
  }.bind(this));
};

/**
 * Searches for the changed directory in the current subtree, and if it is found
 * then updates it.
 *
 * @param {!DirectoryEntry} changedDirectoryEntry The entry ot the changed
 *     directory.
 * @override
 */
DriveVolumeItem.prototype.updateItemByEntry = function(changedDirectoryEntry) {
  // The first item is My Drive, and the second item is Team Drives.
  // Keep in sync with |fixedEntries| in |updateSubDirectories|.
  var index = util.isTeamDriveEntry(changedDirectoryEntry) ? 1 : 0;
  this.items[index].updateItemByEntry(changedDirectoryEntry);
};

/**
 * Select the item corresponding to the given entry.
 * @param {!DirectoryEntry|!FakeEntry} entry The directory entry to be selected.
 *     Can be a fake.
 * @override
 */
DriveVolumeItem.prototype.selectByEntry = function(entry) {
  // Find the item to be selected among children.
  this.searchAndSelectByEntry(entry);
};

////////////////////////////////////////////////////////////////////////////////
// ShortcutItem

/**
 * A TreeItem which represents a shortcut for Drive folder.
 * Shortcut items are displayed as top-level children of DirectoryTree.
 *
 * @param {!NavigationModelShortcutItem} modelItem NavigationModelItem of this
 *     volume.
 * @param {!DirectoryTree} tree Current tree, which contains this item.
 * @extends {cr.ui.TreeItem}
 * @constructor
 */
function ShortcutItem(modelItem, tree) {
  var item = /** @type {ShortcutItem} */ (new cr.ui.TreeItem());
  // Get the original label id defined by TreeItem, before overwriting
  // prototype.
  var labelId = item.labelElement.id;
  item.__proto__ = ShortcutItem.prototype;

  item.parentTree_ = tree;
  item.dirEntry_ = modelItem.entry;
  item.modelItem_ = modelItem;

  item.innerHTML = TREE_ITEM_INNER_HTML;
  item.labelElement.id = labelId;

  var icon = item.querySelector('.icon');
  icon.classList.add('item-icon');
  icon.setAttribute('volume-type-icon', VolumeManagerCommon.VolumeType.DRIVE);

  if (tree.contextMenuForRootItems)
    item.setContextMenu_(tree.contextMenuForRootItems);

  item.label = modelItem.entry.name;
  return item;
}

ShortcutItem.prototype = {
  __proto__: cr.ui.TreeItem.prototype,
  get entry() {
    return this.dirEntry_;
  },
  get modelItem() {
    return this.modelItem_;
  },
  get labelElement() {
    return this.firstElementChild.querySelector('.label');
  }
};

/**
 * Finds a parent directory of the {@code entry} in {@code this}, and
 * invokes the DirectoryItem.selectByEntry() of the found directory.
 *
 * @param {!DirectoryEntry|!FakeEntry} entry The entry to be searched for. Can
 *     be a fake.
 * @return {boolean} True if the parent item is found.
 */
ShortcutItem.prototype.searchAndSelectByEntry = function(entry) {
  // Always false as shortcuts have no children.
  return false;
};

/**
 * Invoked when the tree item is clicked.
 *
 * @param {Event} e Click event.
 * @override
 */
ShortcutItem.prototype.handleClick = function(e) {
  cr.ui.TreeItem.prototype.handleClick.call(this, e);

  // Do not activate with right click.
  if (e.button === 2)
    return;
  this.activate();

  // Resets file selection when a volume is clicked.
  this.parentTree_.directoryModel.clearSelection();

  var location = this.tree.volumeManager.getLocationInfo(this.entry);
  DirectoryItemTreeBaseMethods.recordUMASelectedEntry.call(
      this, e, location.rootType, location.isRootEntry);
};

/**
 * Select the item corresponding to the given entry.
 * @param {!DirectoryEntry} entry The directory entry to be selected.
 */
ShortcutItem.prototype.selectByEntry = function(entry) {
  if (util.isSameEntry(entry, this.entry))
    this.selected = true;
};

/**
 * Sets the context menu for shortcut items.
 * @param {!cr.ui.Menu} menu Menu to be set.
 * @private
 */
ShortcutItem.prototype.setContextMenu_ = function(menu) {
  cr.ui.contextMenuHandler.setContextMenu(this, menu);
};

/**
 * Change current entry to the entry corresponding to this shortcut.
 */
ShortcutItem.prototype.activate = function() {
  var directoryModel = this.parentTree_.directoryModel;
  var onEntryResolved = function(entry) {
    // Changes directory to the model item's root directory if needed.
    if (!util.isSameEntry(directoryModel.getCurrentDirEntry(), entry)) {
      metrics.recordUserAction('FolderShortcut.Navigate');
      directoryModel.changeDirectoryEntry(entry);
    }
  }.bind(this);

  // For shortcuts we already have an Entry, but it has to be resolved again
  // in case, it points to a non-existing directory.
  window.webkitResolveLocalFileSystemURL(
      this.entry.toURL(),
      onEntryResolved,
      function() {
        // Error, the entry can't be re-resolved. It may happen for shortcuts
        // which targets got removed after resolving the Entry during
        // initialization.
        this.parentTree_.dataModel.onItemNotFoundError(this.modelItem);
      }.bind(this));
};

////////////////////////////////////////////////////////////////////////////////
// MenuItem

/**
 * A TreeItem which represents a command button.
 * Command items are displayed as top-level children of DirectoryTree.
 *
 * @param {!NavigationModelMenuItem} modelItem
 * @param {!DirectoryTree} tree Current tree, which contains this item.
 * @extends {cr.ui.TreeItem}
 * @constructor
 */
function MenuItem(modelItem, tree) {
  var item = new cr.ui.TreeItem();
  // Get the original label id defined by TreeItem, before overwriting
  // prototype.
  var labelId = item.labelElement.id;
  item.__proto__ = MenuItem.prototype;

  item.parentTree_ = tree;
  item.modelItem_ = modelItem;
  item.innerHTML = MENU_TREE_ITEM_INNER_HTML;
  item.labelElement.id = labelId;
  item.label = modelItem.label;

  item.menuButton_ = /** @type {!cr.ui.MenuButton} */(queryRequiredElement(
        '.button', assert(item.firstElementChild)));
  item.menuButton_.setAttribute('menu', item.modelItem_.menu);
  cr.ui.MenuButton.decorate(item.menuButton_);

  var icon = queryRequiredElement('.icon', item);
  icon.setAttribute('menu-button-icon', item.modelItem_.icon);

  return item;
}

MenuItem.prototype = {
  __proto__: cr.ui.TreeItem.prototype,
  get entry() {
    return null;
  },
  get modelItem() {
    return this.modelItem_;
  },
  get labelElement() {
    return this.firstElementChild.querySelector('.label');
  }
};

/**
 * @param {!DirectoryEntry|!FakeEntry} entry
 * @return {boolean} True if the parent item is found.
 */
MenuItem.prototype.searchAndSelectByEntry = function(entry) {
  return false;
};

/**
 * @override
 */
MenuItem.prototype.handleClick = function(e) {
  this.activate();

  DirectoryItemTreeBaseMethods.recordUMASelectedEntry.call(
      this, e, VolumeManagerCommon.RootType.ADD_NEW_SERVICES_MENU, true);
};

/**
 * @param {!DirectoryEntry} entry
 */
MenuItem.prototype.selectByEntry = function(entry) {
};

/**
 * Executes the command.
 */
MenuItem.prototype.activate = function() {
  // Dispatch an event to update the menu (if updatable).
  var updateEvent = /** @type {MenuItemUpdateEvent} */ (new Event('update'));
  updateEvent.menuButton = this.menuButton_;
  this.menuButton_.menu.dispatchEvent(updateEvent);

  this.menuButton_.showMenu();
};

////////////////////////////////////////////////////////////////////////////////
// FakeItem

/**
 * FakeItem is used by Recent and Linux Files.
 * @param {!VolumeManagerCommon.RootType} rootType root type.
 * @param {!NavigationModelFakeItem} modelItem
 * @param {!DirectoryTree} tree Current tree, which contains this item.
 * @extends {cr.ui.TreeItem}
 * @constructor
 */
function FakeItem(rootType, modelItem, tree) {
  var item = new cr.ui.TreeItem();
  // Get the original label id defined by TreeItem, before overwriting
  // prototype.
  var labelId = item.labelElement.id;
  item.__proto__ = FakeItem.prototype;

  item.rootType_ = rootType;
  item.parentTree_ = tree;
  item.modelItem_ = modelItem;
  item.dirEntry_ = modelItem.entry;
  item.innerHTML = TREE_ITEM_INNER_HTML;
  item.labelElement.id = labelId;
  item.label = modelItem.label;

  var icon = queryRequiredElement('.icon', item);
  icon.classList.add('item-icon');
  icon.setAttribute('root-type-icon', rootType);

  return item;
}

FakeItem.prototype = {
  __proto__: cr.ui.TreeItem.prototype,
  get entry() {
    return this.dirEntry_;
  },
  get modelItem() {
    return this.modelItem_;
  },
  get labelElement() {
    return this.firstElementChild.querySelector('.label');
  }
};

/**
 * @param {!DirectoryEntry|!FakeEntry} entry
 * @return {boolean} True if the parent item is found.
 */
FakeItem.prototype.searchAndSelectByEntry = function(entry) {
  return false;
};

/**
 * @override
 */
FakeItem.prototype.handleClick = function(e) {
  this.activate();

  DirectoryItemTreeBaseMethods.recordUMASelectedEntry.call(
      this, e, this.rootType_, true);
};

/**
 * @param {!DirectoryEntry} entry
 */
FakeItem.prototype.selectByEntry = function(entry) {
  if (util.isSameEntry(entry, this.entry))
    this.selected = true;
};

/**
 * Executes the command.
 */
FakeItem.prototype.activate = function() {
  this.parentTree_.directoryModel.activateDirectoryEntry(this.entry);
};

////////////////////////////////////////////////////////////////////////////////
// DirectoryTree

/**
 * Tree of directories on the middle bar. This element is also the root of
 * items, in other words, this is the parent of the top-level items.
 *
 * @constructor
 * @extends {cr.ui.Tree}
 */
function DirectoryTree() {}

/**
 * Decorates an element.
 * @param {HTMLElement} el Element to be DirectoryTree.
 * @param {!DirectoryModel} directoryModel Current DirectoryModel.
 * @param {!VolumeManagerWrapper} volumeManager VolumeManager of the system.
 * @param {!MetadataModel} metadataModel Shared MetadataModel instance.
 * @param {!FileOperationManager} fileOperationManager
 * @param {boolean} fakeEntriesVisible True if it should show the fakeEntries.
 */
DirectoryTree.decorate = function(
    el, directoryModel, volumeManager, metadataModel, fileOperationManager,
    fakeEntriesVisible) {
  el.__proto__ = DirectoryTree.prototype;
  /** @type {DirectoryTree} */ (el).decorateDirectoryTree(
      directoryModel, volumeManager, metadataModel, fileOperationManager,
      fakeEntriesVisible);
};

DirectoryTree.prototype = {
  __proto__: cr.ui.Tree.prototype,

  // DirectoryTree is always expanded.
  get expanded() { return true; },
  /**
   * @param {boolean} value Not used.
   */
  set expanded(value) {},

  /**
   * The DirectoryEntry corresponding to this DirectoryItem. This may be
   * a dummy DirectoryEntry.
   * @type {DirectoryEntry|Object}
   */
  get entry() {
    return this.dirEntry_;
  },

  /**
   * The DirectoryModel this tree corresponds to.
   * @type {DirectoryModel}
   */
  get directoryModel() {
    return this.directoryModel_;
  },

  /**
   * The VolumeManager instance of the system.
   * @type {VolumeManager}
   */
  get volumeManager() {
    return this.volumeManager_;
  },

  /**
   * The reference to shared MetadataModel instance.
   * @type {!MetadataModel}
   */
  get metadataModel() {
    return this.metadataModel_;
  },

  set dataModel(dataModel) {
    if (!this.onListContentChangedBound_)
      this.onListContentChangedBound_ = this.onListContentChanged_.bind(this);

    if (this.dataModel_) {
      this.dataModel_.removeEventListener(
          'change', this.onListContentChangedBound_);
      this.dataModel_.removeEventListener(
          'permuted', this.onListContentChangedBound_);
    }
    this.dataModel_ = dataModel;
    dataModel.addEventListener('change', this.onListContentChangedBound_);
    dataModel.addEventListener('permuted', this.onListContentChangedBound_);
  },

  get dataModel() {
    return this.dataModel_;
  }
};

cr.defineProperty(DirectoryTree, 'contextMenuForSubitems', cr.PropertyKind.JS);
cr.defineProperty(DirectoryTree, 'contextMenuForRootItems', cr.PropertyKind.JS);

/**
 * Updates and selects new directory.
 * @param {!DirectoryEntry} parentDirectory Parent directory of new directory.
 * @param {!DirectoryEntry} newDirectory
 */
DirectoryTree.prototype.updateAndSelectNewDirectory = function(
    parentDirectory, newDirectory) {
  // Expand parent directory.
  var parentItem = DirectoryItemTreeBaseMethods.getItemByEntry.call(
      this, parentDirectory);
  parentItem.expanded = true;

  // If new directory is already added to the tree, just select it.
  for (var i = 0; i < parentItem.items.length; i++) {
    var item = parentItem.items[i];
    if (util.isSameEntry(item.entry, newDirectory)) {
      this.selectedItem = item;
      return;
    }
  }

  // Create new item, and add it.
  var newDirectoryItem = new SubDirectoryItem(
      newDirectory.name, newDirectory, parentItem, this);

  var addAt = 0;
  while (addAt < parentItem.items.length &&
      parentItem.items[addAt].entry.name < newDirectory.name) {
    addAt++;
  }

  parentItem.addAt(newDirectoryItem, addAt);
  this.selectedItem = newDirectoryItem;
};

/**
 * Calls DirectoryItemTreeBaseMethods.updateSubElementsFromList().
 *
 * @param {boolean} recursive True if the all visible sub-directories are
 *     updated recursively including left arrows. If false, the update walks
 *     only immediate child directories without arrows.
 */
DirectoryTree.prototype.updateSubElementsFromList = function(recursive) {
  // First, current items which is not included in the dataModel should be
  // removed.
  for (var i = 0; i < this.items.length;) {
    var found = false;
    for (var j = 0; j < this.dataModel.length; j++) {
      // Comparison by references, which is safe here, as model items are long
      // living.
      if (this.items[i].modelItem === this.dataModel.item(j)) {
        found = true;
        break;
      }
    }
    if (!found) {
      if (this.items[i].selected)
        this.items[i].selected = false;
      this.remove(this.items[i]);
    } else {
      i++;
    }
  }

  // Next, insert items which is in dataModel but not in current items.
  var modelIndex = 0;
  var itemIndex = 0;
  while (modelIndex < this.dataModel.length) {
    if (itemIndex < this.items.length &&
        this.items[itemIndex].modelItem === this.dataModel.item(modelIndex)) {
      if (recursive && this.items[itemIndex] instanceof VolumeItem)
        this.items[itemIndex].updateSubDirectories(true);
    } else {
      var modelItem = this.dataModel.item(modelIndex);
      switch (modelItem.type) {
        case NavigationModelItemType.VOLUME:
          if (modelItem.volumeInfo.volumeType ===
              VolumeManagerCommon.VolumeType.DRIVE) {
            this.addAt(new DriveVolumeItem(modelItem, this), itemIndex);
          } else {
            this.addAt(new VolumeItem(modelItem, this), itemIndex);
          }
          break;
        case NavigationModelItemType.SHORTCUT:
          this.addAt(new ShortcutItem(modelItem, this), itemIndex);
          break;
        case NavigationModelItemType.MENU:
          this.addAt(new MenuItem(modelItem, this), itemIndex);
          break;
        case NavigationModelItemType.RECENT:
          this.addAt(
              new FakeItem(
                  VolumeManagerCommon.RootType.RECENT, modelItem, this),
              itemIndex);
          break;
        case NavigationModelItemType.CROSTINI:
          this.addAt(
              new FakeItem(
                  VolumeManagerCommon.RootType.CROSTINI, modelItem, this),
              itemIndex);
          break;
      }
    }
    itemIndex++;
    modelIndex++;
  }

  if (itemIndex !== 0)
    this.hasChildren = true;
};

/**
 * Finds a parent directory of the {@code entry} in {@code this}, and
 * invokes the DirectoryItem.selectByEntry() of the found directory.
 *
 * @param {!DirectoryEntry|!FakeEntry} entry The entry to be searched for. Can
 *     be a fake.
 * @return {boolean} True if the parent item is found.
 */
DirectoryTree.prototype.searchAndSelectByEntry = function(entry) {
  // If the |entry| is same as one of volumes or shortcuts, select it.
  for (var i = 0; i < this.items.length; i++) {
    // Skips the Drive root volume. For Drive entries, one of children of Drive
    // root or shortcuts should be selected.
    var item = this.items[i];
    if (item instanceof DriveVolumeItem)
      continue;

    if (util.isSameEntry(item.entry, entry)) {
      item.selectByEntry(entry);
      return true;
    }
  }
  // Otherwise, search whole tree.
  var found = DirectoryItemTreeBaseMethods.searchAndSelectByEntry.call(
      this, entry);
  return found;
};

/**
 * Decorates an element.
 * @param {!DirectoryModel} directoryModel Current DirectoryModel.
 * @param {!VolumeManagerWrapper} volumeManager VolumeManager of the system.
 * @param {!MetadataModel} metadataModel Shared MetadataModel instance.
 * @param {!FileOperationManager} fileOperationManager
 * @param {boolean} fakeEntriesVisible True if it should show the fakeEntries.
 */
DirectoryTree.prototype.decorateDirectoryTree = function(
    directoryModel, volumeManager, metadataModel, fileOperationManager,
    fakeEntriesVisible) {
  cr.ui.Tree.prototype.decorate.call(this);

  this.sequence_ = 0;
  this.directoryModel_ = directoryModel;
  this.volumeManager_ = volumeManager;
  this.metadataModel_ = metadataModel;
  this.models_ = [];

  this.fileFilter_ = this.directoryModel_.getFileFilter();
  this.fileFilter_.addEventListener('changed',
                                    this.onFilterChanged_.bind(this));

  this.directoryModel_.addEventListener('directory-changed',
      this.onCurrentDirectoryChanged_.bind(this));

  util.addEventListenerToBackgroundComponent(
      fileOperationManager,
      'entries-changed',
      this.onEntriesChanged_.bind(this));

  this.addEventListener('click', (event) => {
    // Chromevox triggers |click| without switching focus, we force the focus
    // here so we can handle further keyboard/mouse events to expand/collapse
    // directories.
    if (document.activeElement === document.body) {
      this.focus();
    }
  });

  this.privateOnDirectoryChangedBound_ =
      this.onDirectoryContentChanged_.bind(this);
  chrome.fileManagerPrivate.onDirectoryChanged.addListener(
      this.privateOnDirectoryChangedBound_);

  /**
   * Flag to show fake entries in the tree.
   * @type {boolean}
   * @private
   */
  this.fakeEntriesVisible_ = fakeEntriesVisible;
};

/**
 * Handles entries changed event.
 * @param {!Event} event
 * @private
 */
DirectoryTree.prototype.onEntriesChanged_ = function(event) {
  var directories = event.entries.filter((entry) => entry.isDirectory);

  if (directories.length === 0)
    return;

  switch (event.kind) {
    case util.EntryChangedKind.CREATED:
      // Handle as change event of parent entry.
      Promise.all(
          directories.map((directory) =>
            new Promise(directory.getParent.bind(directory))))
          .then(function(parentDirectories) {
        parentDirectories.forEach((parentDirectory) =>
            this.updateTreeByEntry_(parentDirectory));
      }.bind(this));
      break;
    case util.EntryChangedKind.DELETED:
      directories.forEach((directory) => this.updateTreeByEntry_(directory));
      break;
    default:
      assertNotReached();
  }
};

/**
 * Select the item corresponding to the given entry.
 * @param {!DirectoryEntry|!FakeEntry} entry The directory entry to be selected.
 *     Can be a fake.
 */
DirectoryTree.prototype.selectByEntry = function(entry) {
  if (this.selectedItem && util.isSameEntry(entry, this.selectedItem.entry))
    return;

  if (this.searchAndSelectByEntry(entry))
    return;

  this.updateSubDirectories(false /* recursive */);
  var currentSequence = ++this.sequence_;
  var volumeInfo = this.volumeManager_.getVolumeInfo(entry);
  if (!volumeInfo)
    return;
  volumeInfo.resolveDisplayRoot(function() {
    if (this.sequence_ !== currentSequence)
      return;
    if (!this.searchAndSelectByEntry(entry))
      this.selectedItem = null;
  }.bind(this));
};

/**
 * Activates the volume or the shortcut corresponding to the given index.
 * @param {number} index 0-based index of the target top-level item.
 * @return {boolean} True if one of the volume items is selected.
 */
DirectoryTree.prototype.activateByIndex = function(index) {
  if (index < 0 || index >= this.items.length)
    return false;

  this.items[index].selected = true;
  this.items[index].activate();
  return true;
};

/**
 * Retrieves the latest subdirectories and update them on the tree.
 *
 * @param {boolean} recursive True if the update is recursively.
 * @param {function()=} opt_callback Called when subdirectories are fully
 *     updated.
 */
DirectoryTree.prototype.updateSubDirectories = function(
    recursive, opt_callback) {
  this.redraw(recursive);
  if (opt_callback)
    opt_callback();
};

/**
 * Redraw the list.
 * @param {boolean} recursive True if the update is recursively. False if the
 *     only root items are updated.
 */
DirectoryTree.prototype.redraw = function(recursive) {
  this.updateSubElementsFromList(recursive);
};

/**
 * Invoked when the filter is changed.
 * @private
 */
DirectoryTree.prototype.onFilterChanged_ = function() {
  // Returns immediately, if the tree is hidden.
  if (this.hidden)
    return;

  this.redraw(true /* recursive */);
};

/**
 * Invoked when a directory is changed.
 * @param {!FileWatchEvent} event Event.
 * @private
 */
DirectoryTree.prototype.onDirectoryContentChanged_ = function(event) {
  if (event.eventType !== 'changed' || !event.entry)
    return;

  this.updateTreeByEntry_(/** @type{!Entry} */ (event.entry));
};

/**
 * Updates tree by entry.
 * @param {!Entry} entry A changed entry. Deleted entry is passed when watched
 *     directory is deleted.
 * @private
 */
DirectoryTree.prototype.updateTreeByEntry_ = function(entry) {
  entry.getDirectory(entry.fullPath, {create: false},
      function() {
        // If entry exists.
        // e.g. /a/b is deleted while watching /a.
        for (var i = 0; i < this.items.length; i++) {
          if (this.items[i] instanceof VolumeItem)
            this.items[i].updateItemByEntry(entry);
        }
      }.bind(this),
      function() {
        // If entry does not exist, try to get parent and update the subtree by
        // it.
        // e.g. /a/b is deleted while watching /a/b. Try to update /a in this
        //     case.
        entry.getParent(function(parentEntry) {
          this.updateTreeByEntry_(parentEntry);
        }.bind(this), function(error) {
          // If it fails to get parent, update the subtree by volume.
          // e.g. /a/b is deleted while watching /a/b/c. getParent of /a/b/c
          //     fails in this case. We falls back to volume update.
          //
          // TODO(yawano): Try to get parent path also in this case by
          //     manipulating path string.
          var volumeInfo = this.volumeManager.getVolumeInfo(entry);
          if (!volumeInfo)
            return;

          for (var i = 0; i < this.items.length; i++) {
            if (this.items[i] instanceof VolumeItem &&
                this.items[i].volumeInfo === volumeInfo) {
              this.items[i].updateSubDirectories(true /* recursive */);
            }
          }
        }.bind(this));
      }.bind(this));
};

/**
 * Invoked when the current directory is changed.
 * @param {!Event} event Event.
 * @private
 */
DirectoryTree.prototype.onCurrentDirectoryChanged_ = function(event) {
  this.selectByEntry(event.newDirEntry);
};

/**
 * Invoked when the volume list or shortcut list is changed.
 * @private
 */
DirectoryTree.prototype.onListContentChanged_ = function() {
  this.updateSubDirectories(false, function() {
    // If no item is selected now, try to select the item corresponding to
    // current directory because the current directory might have been populated
    // in this tree in previous updateSubDirectories().
    if (!this.selectedItem) {
      var currentDir = this.directoryModel_.getCurrentDirEntry();
      if (currentDir)
        this.selectByEntry(currentDir);
    }
  }.bind(this));
};

/**
 * Updates the UI after the layout has changed.
 */
DirectoryTree.prototype.relayout = function() {
  cr.dispatchSimpleEvent(this, 'relayout', true);
};
