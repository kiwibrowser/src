// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

Polymer({
  is: 'bookmarks-edit-dialog',

  properties: {
    /** @private */
    isFolder_: Boolean,

    /** @private */
    isEdit_: Boolean,

    /**
     * Item that is being edited, or null when adding.
     * @private {?BookmarkNode}
     */
    editItem_: Object,

    /**
     * Parent node for the item being added, or null when editing.
     * @private {?string}
     */
    parentId_: String,

    /** @private */
    titleValue_: String,

    /** @private */
    urlValue_: String,
  },

  /**
   * Show the dialog to add a new folder (if |isFolder|) or item, which will be
   * inserted into the tree as a child of |parentId|.
   * @param {boolean} isFolder
   * @param {string} parentId
   */
  showAddDialog: function(isFolder, parentId) {
    this.reset_();
    this.isEdit_ = false;
    this.isFolder_ = isFolder;
    this.parentId_ = parentId;

    bookmarks.DialogFocusManager.getInstance().showDialog(this.$.dialog);
  },

  /**
   * Show the edit dialog for |editItem|.
   * @param {BookmarkNode} editItem
   */
  showEditDialog: function(editItem) {
    this.reset_();
    this.isEdit_ = true;
    this.isFolder_ = !editItem.url;
    this.editItem_ = editItem;

    this.titleValue_ = editItem.title;
    if (!this.isFolder_)
      this.urlValue_ = assert(editItem.url);

    bookmarks.DialogFocusManager.getInstance().showDialog(this.$.dialog);
  },

  /**
   * Clear out existing values from the dialog, allowing it to be reused.
   * @private
   */
  reset_: function() {
    this.editItem_ = null;
    this.parentId_ = null;
    this.$.url.invalid = false;
    this.titleValue_ = '';
    this.urlValue_ = '';
  },

  /**
   * @param {boolean} isFolder
   * @param {boolean} isEdit
   * @return {string}
   * @private
   */
  getDialogTitle_: function(isFolder, isEdit) {
    let title;
    if (isEdit)
      title = isFolder ? 'renameFolderTitle' : 'editBookmarkTitle';
    else
      title = isFolder ? 'addFolderTitle' : 'addBookmarkTitle';

    return loadTimeData.getString(title);
  },

  /**
   * Validates the value of the URL field, returning true if it is a valid URL.
   * May modify the value by prepending 'http://' in order to make it valid.
   * @return {boolean}
   * @private
   */
  validateUrl_: function() {
    const urlInput = /** @type {PaperInputElement} */ (this.$.url);
    const originalValue = this.urlValue_;

    if (urlInput.validate())
      return true;

    this.urlValue_ = 'http://' + originalValue;

    if (urlInput.validate())
      return true;

    this.urlValue_ = originalValue;
    return false;
  },

  /** @private */
  onSaveButtonTap_: function() {
    const edit = {'title': this.titleValue_};
    if (!this.isFolder_) {
      if (!this.validateUrl_())
        return;

      edit['url'] = this.urlValue_;
    }

    if (this.isEdit_) {
      chrome.bookmarks.update(this.editItem_.id, edit);
    } else {
      edit['parentId'] = this.parentId_;
      bookmarks.ApiListener.trackUpdatedItems();
      chrome.bookmarks.create(
          edit, bookmarks.ApiListener.highlightUpdatedItems);
    }
    this.$.dialog.close();
  },

  /** @private */
  onCancelButtonTap_: function() {
    this.$.dialog.cancel();
  },
});
