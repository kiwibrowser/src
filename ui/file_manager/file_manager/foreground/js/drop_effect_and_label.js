// Copyright (c) 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * Drop effect names supported as a value of DataTransfer.dropEffect.
 * @enum {string}
 */
const DropEffectType = {
  NONE: 'none',
  COPY: 'copy',
  MOVE: 'move',
  LINK: 'link'
};

/**
 * Represents a drop effect and a label to describe it.
 *
 * @param {!DropEffectType} dropEffect
 * @param {?string} label
 * @constructor
 * @struct
 */
function DropEffectAndLabel(dropEffect, label) {
  /**
   * @private {string}
   */
  this.dropEffect_ = dropEffect;

  /**
   * Optional description why the drop opeartion is (not) permitted.
   *
   * @private {?string}
   */
  this.label_ = label;
}

/**
 * @return {string} Returns the type of the drop effect.
 */
DropEffectAndLabel.prototype.getDropEffect = function() {
  return this.dropEffect_;
}

/**
 * @return {?string} Returns the label. |none| if a label should not appear.
 */
DropEffectAndLabel.prototype.getLabel = function() {
  return this.label_;
}
