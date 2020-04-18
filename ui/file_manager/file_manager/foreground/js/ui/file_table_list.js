// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * Namespace for utility functions.
 */
var filelist = {};

/**
 * File table list.
 * @constructor
 * @struct
 * @extends {cr.ui.table.TableList}
 */
function FileTableList() {
  throw new Error('Designed to decorate elements');
}

/**
 * Decorates TableList as FileTableList.
 * @param {!cr.ui.table.TableList} self A tabel list element.
 */
FileTableList.decorate = function(self) {
  self.__proto__ = FileTableList.prototype;
};

FileTableList.prototype.__proto__ = cr.ui.table.TableList.prototype;

/**
 * @type {?function(number, number)}
 */
FileTableList.prototype.onMergeItems_ = null;

/**
 * @param {function(number, number)} onMergeItems callback called from
 *     |mergeItems| with the parameters |beginIndex| and |endIndex|.
 */
FileTableList.prototype.setOnMergeItems = function(onMergeItems) {
  assert(!this.onMergeItems_);
  this.onMergeItems_ = onMergeItems;
};

/** @override */
FileTableList.prototype.mergeItems = function(beginIndex, endIndex) {
  cr.ui.table.TableList.prototype.mergeItems.call(this, beginIndex, endIndex);

  // Make sure that list item's selected attribute is updated just after the
  // mergeItems operation is done. This prevents checkmarks on selected items
  // from being animated unintentionally by redraw.
  for (var i = beginIndex; i < endIndex; i++) {
    var item = this.getListItemByIndex(i);
    if (!item)
      continue;
    var isSelected = this.selectionModel.getIndexSelected(i);
    if (item.selected != isSelected)
      item.selected = isSelected;
  }

  if (this.onMergeItems_) {
    this.onMergeItems_(beginIndex, endIndex);
  }
};

/** @override */
FileTableList.prototype.createSelectionController = function(sm) {
  return new FileListSelectionController(assert(sm));
};

/**
 * Selection controller for the file table list.
 * @param {!cr.ui.ListSelectionModel} selectionModel The selection model to
 *     interact with.
 * @constructor
 * @extends {cr.ui.ListSelectionController}
 * @struct
 */
function FileListSelectionController(selectionModel) {
  cr.ui.ListSelectionController.call(this, selectionModel);

  /**
   * Whether to allow touch-specific interaction.
   * @type {boolean}
   */
  this.enableTouchMode_ = false;
  util.isTouchModeEnabled().then(function(enabled) {
    this.enableTouchMode_ = enabled;
  }.bind(this));

  /**
   * @type {!FileTapHandler}
   * @const
   */
  this.tapHandler_ = new FileTapHandler();
}

FileListSelectionController.prototype = /** @struct */ {
  __proto__: cr.ui.ListSelectionController.prototype
};

/** @override */
FileListSelectionController.prototype.handlePointerDownUp = function(e, index) {
  filelist.handlePointerDownUp.call(this, e, index);
};

/** @override */
FileListSelectionController.prototype.handleTouchEvents = function(e, index) {
  if (!this.enableTouchMode_)
    return;
  if (this.tapHandler_.handleTouchEvents(
          e, index, filelist.handleTap.bind(this)))
    // If a tap event is processed, FileTapHandler cancels the event to prevent
    // triggering click events. Then it results not moving the focus to the
    // list. So we do that here explicitly.
    filelist.focusParentList(e);
};

/** @override */
FileListSelectionController.prototype.handleKeyDown = function(e) {
  filelist.handleKeyDown.call(this, e);
};

/**
 * Common item decoration for table's and grid's items.
 * @param {cr.ui.ListItem} li List item.
 * @param {Entry} entry The entry.
 * @param {!MetadataModel} metadataModel Cache to
 *     retrieve metadada.
 */
filelist.decorateListItem = function(li, entry, metadataModel) {
  li.classList.add(entry.isDirectory ? 'directory' : 'file');
  // The metadata may not yet be ready. In that case, the list item will be
  // updated when the metadata is ready via updateListItemsMetadata. For files
  // not on an external backend, externalProps is not available.
  var externalProps = metadataModel.getCache(
      [entry], ['hosted', 'availableOffline', 'customIconUrl', 'shared'])[0];
  filelist.updateListItemExternalProps(
      li, externalProps, util.isTeamDriveRoot(entry));

  // Overriding the default role 'list' to 'listbox' for better
  // accessibility on ChromeOS.
  li.setAttribute('role', 'option');
  li.setAttribute('aria-describedby', 'more-actions-info');

  Object.defineProperty(li, 'selected', {
    /**
     * @this {cr.ui.ListItem}
     * @return {boolean} True if the list item is selected.
     */
    get: function() {
      return this.hasAttribute('selected');
    },

    /**
     * @this {cr.ui.ListItem}
     */
    set: function(v) {
      if (v)
        this.setAttribute('selected', '');
      else
        this.removeAttribute('selected');
    }
  });
};

/**
 * Render the type column of the detail table.
 * @param {!Document} doc Owner document.
 * @param {!Entry} entry The Entry object to render.
 * @param {string=} opt_mimeType Optional mime type for the file.
 * @return {!HTMLDivElement} Created element.
 */
filelist.renderFileTypeIcon = function(doc, entry, opt_mimeType) {
  var icon = /** @type {!HTMLDivElement} */ (doc.createElement('div'));
  icon.className = 'detail-icon';
  icon.setAttribute('file-type-icon', FileType.getIcon(entry, opt_mimeType));
  return icon;
};

/**
 * Render filename label for grid and list view.
 * @param {!Document} doc Owner document.
 * @param {!Entry} entry The Entry object to render.
 * @return {!HTMLDivElement} The label.
 */
filelist.renderFileNameLabel = function(doc, entry) {
  // Filename need to be in a '.filename-label' container for correct
  // work of inplace renaming.
  var box = /** @type {!HTMLDivElement} */ (doc.createElement('div'));
  box.className = 'filename-label';
  var fileName = doc.createElement('span');
  fileName.className = 'entry-name';
  fileName.textContent = entry.name;
  box.appendChild(fileName);

  return box;
};

/**
 * Updates grid item or table row for the externalProps.
 * @param {cr.ui.ListItem} li List item.
 * @param {Object} externalProps Metadata.
 * @param {boolean} isTeamDriveRoot Whether the item is a team drive root entry.
 */
filelist.updateListItemExternalProps = function(
    li, externalProps, isTeamDriveRoot) {
  if (li.classList.contains('file')) {
    if (externalProps.availableOffline)
      li.classList.remove('dim-offline');
    else
      li.classList.add('dim-offline');
    // TODO(mtomasz): Consider adding some vidual indication for files which
    // are not cached on LTE. Currently we show them as normal files.
    // crbug.com/246611.
  }

  var iconDiv = li.querySelector('.detail-icon');
  if (!iconDiv)
    return;

  if (externalProps.customIconUrl)
    iconDiv.style.backgroundImage = 'url(' + externalProps.customIconUrl + ')';
  else
    iconDiv.style.backgroundImage = '';  // Back to the default image.

  if (li.classList.contains('directory')) {
    iconDiv.classList.toggle('shared', !!externalProps.shared);
    iconDiv.classList.toggle('team-drive-root', !!isTeamDriveRoot);
  }
};

/**
 * Handles tap events on file list to change the selection state.
 *
 * @param {!Event} e The browser mouse event.
 * @param {number} index The index that was under the mouse pointer, -1 if
 *     none.
 * @param {!FileTapHandler.TapEvent} eventType
 * @return True if conducted any action. False when if did nothing special for
 *     tap.
 * @this {cr.ui.ListSelectionController}
 */
filelist.handleTap = function(e, index, eventType) {
  var sm = /** @type {!FileListSelectionModel|!FileListSingleSelectionModel} */
      (this.selectionModel);
  if (eventType == FileTapHandler.TapEvent.TWO_FINGER_TAP) {
    // Prepare to open the context menu in the same manner as the right click.
    // If the target is any of the selected files, open a one for those files.
    // If the target is a non-selected file, cancel current selection and open
    // context menu for the single file.
    // Otherwise (when the target is the background), for the current folder.
    if (index == -1) {
      // Two-finger tap outside the list should be handled here because it does
      // not produce mousedown/click events.
      sm.unselectAll();
    } else {
      var indexSelected = sm.getIndexSelected(index);
      if (!indexSelected) {
        // Prepare to open context menu of the new item by selecting only it.
        if (sm.getCheckSelectMode()) {
          // Unselect all items once to ensure that the check-select mode is
          // terminated.
          sm.unselectAll();
        }
        sm.beginChange();
        sm.selectedIndex = index;
        sm.endChange();
      }
    }

    // Context menu will be opened for the selected files by the following
    // 'contextmenu' event.
    return false;
  }
  if (index == -1) {
    return false;
  }
  var isTap = eventType == FileTapHandler.TapEvent.TAP ||
      eventType == FileTapHandler.TapEvent.LONG_TAP;
  // Revert to click handling for single tap on checkbox or tap during rename.
  // Single tap on the checkbox in the list view mode should toggle select.
  // Single tap on input for rename should focus on input.
  var isCheckbox = e.target.classList.contains('detail-checkmark');
  var isRename = e.target.localName == 'input';
  if (eventType == FileTapHandler.TapEvent.TAP && (isCheckbox || isRename)) {
    return false;
  }
  if (sm.multiple && sm.getCheckSelectMode() && isTap && !e.shiftKey) {
    // toggle item selection. Equivalent to mouse click on checkbox.
    sm.beginChange();
    sm.setIndexSelected(index, !sm.getIndexSelected(index));
    // Toggle the current one and make it anchor index.
    sm.leadIndex = index;
    sm.anchorIndex = index;
    sm.endChange();
    return true;
  } else if (sm.multiple && (eventType == FileTapHandler.TapEvent.LONG_PRESS)) {
    sm.beginChange();
    if (!sm.getCheckSelectMode()) {
      // Make sure to unselect the leading item that was not the touch target.
      sm.unselectAll();
      sm.setCheckSelectMode(true);
    }
    sm.setIndexSelected(index, true);
    sm.leadIndex = index;
    sm.anchorIndex = index;
    sm.endChange();
    return true;
    // Do not toggle selection yet, so as to avoid unselecting before drag.
  } else if (
      eventType == FileTapHandler.TapEvent.TAP && !sm.getCheckSelectMode()) {
    // Single tap should open the item with default action.
    // Select the item, so that MainWindowComponent will execute action of it.
    sm.beginChange();
    sm.unselectAll();
    sm.setIndexSelected(index, true);
    sm.leadIndex = index;
    sm.anchorIndex = index;
    sm.endChange();
  }
  return false;
};

/**
 * Handles mouseup/mousedown events on file list to change the selection state.
 *
 * Basically the content of this function is identical to
 * cr.ui.ListSelectionController's handlePointerDownUp(), but following
 * handlings are inserted to control the check-select mode.
 *
 * 1) When checkmark area is clicked, toggle item selection and enable the
 *    check-select mode.
 * 2) When non-checkmark area is clicked in check-select mode, disable the
 *    check-select mode.
 *
 * @param {!Event} e The browser mouse event.
 * @param {number} index The index that was under the mouse pointer, -1 if
 *     none.
 * @this {cr.ui.ListSelectionController}
 */
filelist.handlePointerDownUp = function(e, index) {
  var sm = /** @type {!FileListSelectionModel|!FileListSingleSelectionModel} */
           (this.selectionModel);
  var anchorIndex = sm.anchorIndex;
  var isDown = (e.type == 'mousedown');

  var isTargetCheckmark = e.target.classList.contains('detail-checkmark') ||
                          e.target.classList.contains('checkmark');
  // If multiple selection is allowed and the checkmark is clicked without
  // modifiers(Ctrl/Shift), the click should toggle the item's selection.
  // (i.e. same behavior as Ctrl+Click)
  var isClickOnCheckmark = isTargetCheckmark && sm.multiple && index != -1 &&
                           !e.shiftKey && !e.ctrlKey && e.button == 0;

  sm.beginChange();

  if (index == -1) {
    sm.leadIndex = sm.anchorIndex = -1;
    sm.unselectAll();
  } else {
    if (sm.multiple && (e.ctrlKey || isClickOnCheckmark) && !e.shiftKey) {
      // Selection is handled at mouseUp.
      if (!isDown) {
        // 1) When checkmark area is clicked, toggle item selection and enable
        //    the check-select mode.
        if (isClickOnCheckmark) {
          // If Files app enters check-select mode by clicking an item's icon,
          // existing selection should be cleared.
          if (!sm.getCheckSelectMode())
            sm.unselectAll();
        }
        // Always enables check-select mode when the selection is updated by
        // Ctrl+Click or Click on an item's icon.
        sm.setCheckSelectMode(true);

        // Toggle the current one and make it anchor index.
        sm.setIndexSelected(index, !sm.getIndexSelected(index));
        sm.leadIndex = index;
        sm.anchorIndex = index;
      }
    } else if (e.shiftKey && anchorIndex != -1 && anchorIndex != index) {
      // Shift is done in mousedown.
      if (isDown) {
        sm.unselectAll();
        sm.leadIndex = index;
        if (sm.multiple)
          sm.selectRange(anchorIndex, index);
        else
          sm.setIndexSelected(index, true);
      }
    } else {
      // Right click for a context menu needs to not clear the selection.
      var isRightClick = e.button == 2;

      // If the index is selected this is handled in mouseup.
      var indexSelected = sm.getIndexSelected(index);
      if ((indexSelected && !isDown || !indexSelected && isDown) &&
          !(indexSelected && isRightClick)) {
        // 2) When non-checkmark area is clicked in check-select mode, disable
        //    the check-select mode.
        if (sm.getCheckSelectMode()) {
          // Unselect all items once to ensure that the check-select mode is
          // terminated.
          sm.endChange();
          sm.unselectAll();
          sm.beginChange();
        }
        sm.selectedIndex = index;
      }
    }
  }
  sm.endChange();
};

/**
 * Handles key events on file list to change the selection state.
 *
 * Basically the content of this function is identical to
 * cr.ui.ListSelectionController's handleKeyDown(), but following handlings is
 * inserted to control the check-select mode.
 *
 * 1) When pressing direction key results in a single selection, the
 *    check-select mode should be terminated.
 *
 * @param {Event} e The keydown event.
 * @this {cr.ui.ListSelectionController}
 */
filelist.handleKeyDown = function(e) {
  var SPACE_KEY_CODE = 32;
  var tagName = e.target.tagName;

  // If focus is in an input field of some kind, only handle navigation keys
  // that aren't likely to conflict with input interaction (e.g., text
  // editing, or changing the value of a checkbox or select).
  if (tagName == 'INPUT') {
    var inputType = e.target.type;
    // Just protect space (for toggling) for checkbox and radio.
    if (inputType == 'checkbox' || inputType == 'radio') {
      if (e.keyCode == SPACE_KEY_CODE)
        return;
    // Protect all but the most basic navigation commands in anything else.
    } else if (e.key != 'ArrowUp' && e.key != 'ArrowDown') {
      return;
    }
  }
  // Similarly, don't interfere with select element handling.
  if (tagName == 'SELECT')
    return;

  var sm = /** @type {!FileListSelectionModel|!FileListSingleSelectionModel} */
           (this.selectionModel);
  var newIndex = -1;
  var leadIndex = sm.leadIndex;
  var prevent = true;

  // Ctrl/Meta+A
  if (sm.multiple && e.keyCode == 65 &&
      (cr.isMac && e.metaKey || !cr.isMac && e.ctrlKey)) {
    sm.selectAll();
    e.preventDefault();
    return;
  }

  // Esc
  if (e.keyCode === 27 && !e.ctrlKey && !e.shiftKey) {
    sm.unselectAll();
    e.preventDefault();
    return;
  }

  // Space
  if (e.keyCode == SPACE_KEY_CODE) {
    if (leadIndex != -1) {
      var selected = sm.getIndexSelected(leadIndex);
      if (e.ctrlKey || !selected) {
        sm.setIndexSelected(leadIndex, !selected || !sm.multiple);
        return;
      }
    }
  }

  switch (e.key) {
    case 'Home':
      newIndex = this.getFirstIndex();
      break;
    case 'End':
      newIndex = this.getLastIndex();
      break;
    case 'ArrowUp':
      newIndex = leadIndex == -1 ?
          this.getLastIndex() : this.getIndexAbove(leadIndex);
      break;
    case 'ArrowDown':
      newIndex = leadIndex == -1 ?
          this.getFirstIndex() : this.getIndexBelow(leadIndex);
      break;
    case 'ArrowLeft':
    case 'MediaTrackPrevious':
      newIndex = leadIndex == -1 ?
          this.getLastIndex() : this.getIndexBefore(leadIndex);
      break;
    case 'ArrowRight':
    case 'MediaTrackNext':
      newIndex = leadIndex == -1 ?
          this.getFirstIndex() : this.getIndexAfter(leadIndex);
      break;
    default:
      prevent = false;
  }

  if (newIndex >= 0 && newIndex < sm.length) {
    sm.beginChange();

    sm.leadIndex = newIndex;
    if (e.shiftKey) {
      var anchorIndex = sm.anchorIndex;
      if (sm.multiple)
        sm.unselectAll();
      if (anchorIndex == -1) {
        sm.setIndexSelected(newIndex, true);
        sm.anchorIndex = newIndex;
      } else {
        sm.selectRange(anchorIndex, newIndex);
      }
    } else {
      // 1) When pressing direction key results in a single selection, the
      //    check-select mode should be terminated.
      sm.setCheckSelectMode(false);

      if (sm.multiple)
        sm.unselectAll();
      sm.setIndexSelected(newIndex, true);
      sm.anchorIndex = newIndex;
    }

    sm.endChange();

    if (prevent)
      e.preventDefault();
  }
};

/**
 * Focus on the file list that contains the event target.
 * @param {!Event} event the touch event.
 */
filelist.focusParentList = function(event) {
  var element = event.target;
  while (element && !(element instanceof cr.ui.List)) {
    element = element.parentElement;
  }
  if (element) {
    element.focus();
  }
};
