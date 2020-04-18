// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * Dialog to confirm the share between profiles.
 *
 * @param {HTMLElement} parentNode Node to be parent for this dialog.
 * @constructor
 * @extends {FileManagerDialogBase}
 */
function MultiProfileShareDialog(parentNode) {
  FileManagerDialogBase.call(this, parentNode);

  this.mailLabel_ = parentNode.ownerDocument.createElement('label');
  this.mailLabel_.className = 'mail-label';

  var canEdit = parentNode.ownerDocument.createElement('option');
  canEdit.textContent = str('DRIVE_SHARE_TYPE_CAN_EDIT');
  canEdit.value = MultiProfileShareDialog.Result.CAN_EDIT;

  var canComment = parentNode.ownerDocument.createElement('option');
  canComment.textContent = str('DRIVE_SHARE_TYPE_CAN_COMMENT');
  canComment.value = MultiProfileShareDialog.Result.CAN_COMMET;

  var canView = parentNode.ownerDocument.createElement('option');
  canView.textContent = str('DRIVE_SHARE_TYPE_CAN_VIEW');
  canView.value = MultiProfileShareDialog.Result.CAN_VIEW;

  this.shareTypeSelect_ = parentNode.ownerDocument.createElement('select');
  this.shareTypeSelect_.setAttribute('size', 1);
  this.shareTypeSelect_.appendChild(canEdit);
  this.shareTypeSelect_.appendChild(canComment);
  this.shareTypeSelect_.appendChild(canView);

  var shareLine = parentNode.ownerDocument.createElement('div');
  shareLine.className = 'share-line';
  shareLine.appendChild(this.mailLabel_);
  shareLine.appendChild(this.shareTypeSelect_);

  this.frame_.insertBefore(shareLine, this.buttons);
  this.frame_.id = 'multi-profile-share-dialog';

  this.currentProfileId_ = new Promise(function(callback) {
    chrome.fileManagerPrivate.getProfiles(
        function(profiles, currentId, displayedId) {
          callback(currentId);
        });
  });
}

/**
 * Result of the dialog box.
 * @enum {string}
 * @const
 */
MultiProfileShareDialog.Result = {
  CAN_EDIT: 'can_edit',
  CAN_COMMET: 'can_comment',
  CAN_VIEW: 'can_view',
  CANCEL: 'cancel'
};
Object.freeze(MultiProfileShareDialog.Result);

MultiProfileShareDialog.prototype = {
  __proto__: FileManagerDialogBase.prototype
};

/**
 * Shows the dialog.
 * @param {boolean} plural Whether to use message of plural or not.
 * @return {!Promise} Promise fulfilled with the result of dialog. If the dialog
 *     is already opened, it returns null.
 */
MultiProfileShareDialog.prototype.showMultiProfileShareDialog =
    function(plural) {
  return this.currentProfileId_.then(function(currentProfileId) {
    return new Promise(function(fulfill, reject) {
      this.shareTypeSelect_.selectedIndex = 0;
      this.mailLabel_.textContent = currentProfileId;
      var result = FileManagerDialogBase.prototype.showOkCancelDialog.call(
          this,
          str(plural ?
              'MULTI_PROFILE_SHARE_DIALOG_TITLE_PLURAL' :
              'MULTI_PROFILE_SHARE_DIALOG_TITLE'),
          str(plural ?
              'MULTI_PROFILE_SHARE_DIALOG_MESSAGE_PLURAL' :
              'MULTI_PROFILE_SHARE_DIALOG_MESSAGE'),
          function() {
            fulfill(this.shareTypeSelect_.value);
          }.bind(this),
          function() {
            fulfill(MultiProfileShareDialog.Result.CANCEL);
          });
      if (!result)
        reject(new Error('Another dialog has already shown.'));
    }.bind(this));
  }.bind(this));
};
