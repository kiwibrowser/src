// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
goog.provide('ink.CursorUpdater');

goog.require('goog.ui.Component');
goog.require('ink.BrushModel');
goog.require('ink.Color');
goog.require('ink.embed.events');
goog.require('ink.util');



/**
 * @constructor
 * @extends {goog.ui.Component}
 * @struct
 */
ink.CursorUpdater = function() {
  ink.CursorUpdater.base(this, 'constructor');

  /** @private {ink.BrushModel} */
  this.brushModel_ = null;
};
goog.inherits(ink.CursorUpdater, goog.ui.Component);


/** @override */
ink.CursorUpdater.prototype.enterDocument = function() {
  ink.CursorUpdater.base(this, 'enterDocument');

  this.brushModel_ = ink.BrushModel.getInstance(this);

  var handler = this.getHandler();

  handler.listen(this.brushModel_,
      ink.BrushModel.EventType.CHANGE,
      this.updateCursor_);

  handler.listen(ink.util.getRootParentComponent(this),
      ink.embed.events.EventType.DONE_LOADING,
      this.handleDoneLoadingEvent_);
};


/**
 * @param {!ink.embed.events.DoneLoadingEvent} evt
 * @private
 */
ink.CursorUpdater.prototype.handleDoneLoadingEvent_ = function(evt) {
  if (evt.isReadOnly) {
    // If we aren't editable, use a default cursor.
    this.getElement().style.cursor = '';
  } else {
    this.updateCursor_();
  }
};


/**
 * Updates the cursor icon for the drawable area based on the current selection.
 * @private
 */
ink.CursorUpdater.prototype.updateCursor_ = function() {
  var rgb = this.brushModel_.getActiveColorNumericRbg();
  var r = 8;

  var url = 'url(' + this.getCursorDataUrlImage(r, rgb) + ')';
  var target = r + ' ' + r; // target is center of cursor
  var fallback = ', auto';
  var cursorStyle = url + target + fallback;

  this.getElement().style.cursor = cursorStyle;
};

/**
 * @param {number} radius
 * @param {number} rgb
 * @return {string} A data url for a cursor with the provided radius and color.
 */
ink.CursorUpdater.prototype.getCursorDataUrlImage = function(radius, rgb) {

  // TODO(esrauch): We avoid initializing with a fixed brush width for normal
  // rendering ahead of time since the android client has the same brush at a
  // very large number of different radiuses. Since this is only used for
  // cursors, we should really just precompute these as images offline and just
  // splice in the colors.
  var scratchCanvas =
      /** @type {!HTMLCanvasElement} */ (document.createElement('canvas'));

  var context =
      /** @type {!CanvasRenderingContext2D} */ (scratchCanvas.getContext('2d'));

  // Make the cursors opaque.
  var color = new ink.Color(rgb | 0xFF000000);

  // Cap the minimum radius at 2.
  radius = Math.max(radius, 2);
  var diameter = Math.ceil(2 * radius);
  scratchCanvas.width = diameter;
  scratchCanvas.height = diameter;

  // If we have a dark color, use a white outline. For a light color, use a
  // black outline.
  // Compute the lightness value as defined by HSL.
  var max = Math.max(color.r, color.g, color.b);
  var min = Math.min(color.r, color.g, color.b);
  var lightness = 0.5 * (max + min);
  var outlineColor = lightness > 127 ? ink.Color.BLACK : ink.Color.WHITE;

  context.fillStyle = outlineColor.getRgbString();
  context.beginPath();
  context.arc(radius, radius, radius, 0, 2 * Math.PI);
  context.closePath();
  context.fill();

  context.fillStyle = color.getRgbString();
  context.beginPath();
  context.arc(radius, radius, radius - 1, 0, 2 * Math.PI);
  context.closePath();
  context.fill();

  return scratchCanvas.toDataURL();
};
