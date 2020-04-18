// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * @param {!HTMLElement} element
 * @constructor
 * @struct
 */
function GearMenu(element) {
  /**
   * @type {!HTMLMenuItemElement}
   * @const
   */
  this.syncButton = /** @type {!HTMLMenuItemElement} */
      (queryRequiredElement('#gear-menu-drive-sync-settings', element));

  /**
   * @type {!HTMLMenuItemElement}
   * @const
   */
  this.hostedButton = /** @type {!HTMLMenuItemElement} */
      (queryRequiredElement('#gear-menu-drive-hosted-settings', element));

  /**
   * @type {!HTMLElement}
   * @const
   */
  this.volumeSpaceInfo = queryRequiredElement('#volume-space-info', element);

  /**
   * @type {!HTMLElement}
   * @const
   * @private
   */
  this.volumeSpaceInfoSeparator_ =
      queryRequiredElement('#volume-space-info-separator', element);

  /**
   * @type {!HTMLElement}
   * @const
   * @private
   */
  this.volumeSpaceInfoLabel_ =
      queryRequiredElement('#volume-space-info-label', element);

  /**
   * @type {!HTMLElement}
   * @const
   * @private
   */
  this.volumeSpaceInnerBar_ =
      queryRequiredElement('#volume-space-info-bar', element);

  /**
   * @type {!HTMLElement}
   * @const
   * @private
   */
  this.volumeSpaceOuterBar_ = assertInstanceof(
      this.volumeSpaceInnerBar_.parentElement,
      HTMLElement);

  /**
   * Volume space info.
   * @type {Promise<MountPointSizeStats>}
   * @private
   */
  this.spaceInfoPromise_ = null;

  // Initialize attributes.
  this.syncButton.checkable = true;
  this.hostedButton.checkable = true;
}

/**
 * @param {Promise<MountPointSizeStats>} spaceInfoPromise Promise to be
 *     fulfilled with space info.
 * @param {boolean} showLoadingCaption Whether show loading caption or not.
 */
GearMenu.prototype.setSpaceInfo = function(
    spaceInfoPromise, showLoadingCaption) {
  this.spaceInfoPromise_ = spaceInfoPromise;

  if (!spaceInfoPromise) {
    this.volumeSpaceInfo.hidden = true;
    this.volumeSpaceInfoSeparator_.hidden = true;
    return;
  }

  this.volumeSpaceInfo.hidden = false;
  this.volumeSpaceInfoSeparator_.hidden = false;
  this.volumeSpaceInnerBar_.setAttribute('pending', '');
  if (showLoadingCaption) {
    this.volumeSpaceInfoLabel_.innerText = str('WAITING_FOR_SPACE_INFO');
    this.volumeSpaceInnerBar_.style.width = '100%';
  }

  spaceInfoPromise.then(function(spaceInfo) {
    if (this.spaceInfoPromise_ != spaceInfoPromise)
      return;
    this.volumeSpaceInnerBar_.removeAttribute('pending');
    if (spaceInfo) {
      var sizeStr = util.bytesToString(spaceInfo.remainingSize);
      this.volumeSpaceInfoLabel_.textContent = strf('SPACE_AVAILABLE', sizeStr);

      var usedSpace = spaceInfo.totalSize - spaceInfo.remainingSize;
      this.volumeSpaceInnerBar_.style.width =
          (100 * usedSpace / spaceInfo.totalSize) + '%';

      this.volumeSpaceOuterBar_.hidden = false;
    } else {
      this.volumeSpaceOuterBar_.hidden = true;
      this.volumeSpaceInfoLabel_.textContent = str('FAILED_SPACE_INFO');
    }
  }.bind(this));
};
