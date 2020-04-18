// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

(function() {
'use strict';

/**
 * Radius of the margin control in pixels. Padding of control + 1 for border.
 * @type {number}
 */
const RADIUS_PX = 9;

Polymer({
  is: 'print-preview-margin-control',

  behaviors: [print_preview_new.InputBehavior],

  properties: {
    side: {
      type: String,
      reflectToAttribute: true,
    },

    invalid: {
      type: Boolean,
      reflectToAttribute: true,
    },

    invisible: {
      type: Boolean,
      reflectToAttribute: true,
      observer: 'onClipSizeChange_',
    },

    /** @private {number} */
    positionInPts_: {
      type: Number,
      notify: true,
      value: 0,
    },

    /** @type {number} */
    scaleTransform: {
      type: Number,
      notify: true,
    },

    /** @type {!print_preview.Coordinate2d} */
    translateTransform: {
      type: Object,
      notify: true,
    },

    /** @type {!print_preview.Size} */
    pageSize: {
      type: Object,
      notify: true,
    },

    /** @type {?print_preview.Size} */
    clipSize: {
      type: Object,
      notify: true,
      observer: 'onClipSizeChange_',
    },
  },

  observers:
      ['updatePosition_(positionInPts_, scaleTransform, translateTransform, ' +
       'pageSize, side)'],

  listeners: {
    'input-change': 'onInputChange_',
  },

  /** @return {!HTMLInputElement} The input element for InputBehavior. */
  getInput: function() {
    return this.$.textbox;
  },

  /** @param {string} value New value of the margin control's textbox. */
  setTextboxValue: function(value) {
    const textbox = this.$.textbox;
    if (textbox.value != value)
      textbox.value = value;
  },

  /** @return {number} The current position of the margin control. */
  getPositionInPts: function() {
    return this.positionInPts_;
  },

  /** @param {number} position The new position for the margin control. */
  setPositionInPts: function(position) {
    this.positionInPts_ = position;
  },

  /**
   * @return {string} 'true' or 'false', indicating whether the input should be
   *     aria-hidden.
   * @private
   */
  getAriaHidden_: function() {
    return this.invisible.toString();
  },

  /**
   * Converts a value in pixels to points.
   * @param {number} pixels Pixel value to convert.
   * @return {number} Given value expressed in points.
   */
  convertPixelsToPts: function(pixels) {
    let pts;
    const orientationEnum = print_preview.ticket_items.CustomMarginsOrientation;
    if (this.side == orientationEnum.TOP) {
      pts = pixels - this.translateTransform.y + RADIUS_PX;
      pts /= this.scaleTransform;
    } else if (this.side == orientationEnum.RIGHT) {
      pts = pixels - this.translateTransform.x + RADIUS_PX;
      pts /= this.scaleTransform;
      pts = this.pageSize.width - pts;
    } else if (this.side == orientationEnum.BOTTOM) {
      pts = pixels - this.translateTransform.y + RADIUS_PX;
      pts /= this.scaleTransform;
      pts = this.pageSize.height - pts;
    } else {
      assert(this.side == orientationEnum.LEFT);
      pts = pixels - this.translateTransform.x + RADIUS_PX;
      pts /= this.scaleTransform;
    }
    return pts;
  },

  /**
   * @param {!PointerEvent} event A pointerdown event triggered by this element.
   * @return {boolean} Whether the margin should start being dragged.
   */
  shouldDrag: function(event) {
    return !this.$.textbox.disabled && event.button == 0 &&
        (event.path[0] == this || event.path[0] == this.$.line);
  },

  /**
   * @param {!CustomEvent} e Contains the new value of the input.
   * @private
   */
  onInputChange_: function(e) {
    this.fire('text-change', e.detail);
  },

  /** @private */
  onBlur_: function() {
    this.resetAndUpdate();
    if (this.invalid)
      this.fire('text-blur');
  },

  /** @private */
  updatePosition_: function() {
    const orientationEnum = print_preview.ticket_items.CustomMarginsOrientation;
    let x = this.translateTransform.x;
    let y = this.translateTransform.y;
    let width = null;
    let height = null;
    if (this.side == orientationEnum.TOP) {
      y = this.scaleTransform * this.positionInPts_ +
          this.translateTransform.y - RADIUS_PX;
      width = this.scaleTransform * this.pageSize.width;
    } else if (this.side == orientationEnum.RIGHT) {
      x = this.scaleTransform * (this.pageSize.width - this.positionInPts_) +
          this.translateTransform.x - RADIUS_PX;
      height = this.scaleTransform * this.pageSize.height;
    } else if (this.side == orientationEnum.BOTTOM) {
      y = this.scaleTransform * (this.pageSize.height - this.positionInPts_) +
          this.translateTransform.y - RADIUS_PX;
      width = this.scaleTransform * this.pageSize.width;
    } else {
      x = this.scaleTransform * this.positionInPts_ +
          this.translateTransform.x - RADIUS_PX;
      height = this.scaleTransform * this.pageSize.height;
    }
    window.requestAnimationFrame(() => {
      this.style.left = Math.round(x) + 'px';
      this.style.top = Math.round(y) + 'px';
      if (width != null) {
        this.style.width = Math.round(width) + 'px';
      }
      if (height != null) {
        this.style.height = Math.round(height) + 'px';
      }
    });
    this.onClipSizeChange_();
  },

  /** @private */
  onClipSizeChange_: function() {
    if (!this.clipSize)
      return;
    window.requestAnimationFrame(() => {
      const offsetLeft = this.offsetLeft;
      const offsetTop = this.offsetTop;
      this.style.clip = 'rect(' + (-offsetTop) + 'px, ' +
          (this.clipSize.width - offsetLeft) + 'px, ' +
          (this.clipSize.height - offsetTop) + 'px, ' + (-offsetLeft) + 'px)';
    });
  },
});
})();
