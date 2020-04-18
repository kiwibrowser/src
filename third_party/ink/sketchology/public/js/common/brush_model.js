// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
goog.provide('ink.BrushModel');

goog.require('goog.events');
goog.require('goog.events.EventTarget');
goog.require('ink.Model');
goog.require('sketchology.proto.BrushType');
goog.require('sketchology.proto.ToolParams.ToolType');



/**
 * Holds the state of the ink brush. Toolbar widgets update BrushModel, which in
 * turn dispatches CHANGE events to update the toolbar components' states.
 * @constructor
 * @extends {ink.Model}
 * @param {!goog.events.EventTarget} root Unused
 */
ink.BrushModel = function(root) {
  ink.BrushModel.base(this, 'constructor');
  /**
   * Last brush color (not including erase color).
   * @private {string}
   */
  this.color_ = ink.BrushModel.DEFAULT_DRAW_COLOR;

  /**
   * The active stroke width of the brush (includes erase size).
   * @private {number}
   */
  this.strokeWidth_ = ink.BrushModel.DEFAULT_DRAW_SIZE;

  /**
   * @private {boolean}
   */
  this.isErasing_ = ink.BrushModel.DEFAULT_ISERASING;

  /**
   * @private {sketchology.proto.ToolParams.ToolType}
   */
  this.toolType_ = sketchology.proto.ToolParams.ToolType.LINE;

  /**
   * @private {sketchology.proto.BrushType}
   */
  this.brushType_ = sketchology.proto.BrushType.CALLIGRAPHY;

  /**
   * @private {string}
   */
  this.shape_ = 'CALLIGRAPHY';
};
goog.inherits(ink.BrushModel, ink.Model);
ink.Model.addGetter(ink.BrushModel);


/**
 * @const {string}
 */
ink.BrushModel.DEFAULT_DRAW_COLOR = '#000000';


/**
 * @const {number}
 */
ink.BrushModel.DEFAULT_DRAW_SIZE = 0.6;


/**
 * @const {string}
 */
ink.BrushModel.DEFAULT_ERASE_COLOR = '#FFFFFF';

/**
 * @const {boolean}
 */
ink.BrushModel.DEFAULT_ISERASING = false;


/**
 * The events fired by the BrushModel.
 * @enum {string} The event types for the BrushModel.
 */
ink.BrushModel.EventType = {
  /**
   * Fired when the BrushModel is changed.
   */
  CHANGE: goog.events.getUniqueId('change')
};


/**
 * @const {Object}
 */
ink.BrushModel.SHAPE_TO_TOOLTYPE = {
  'AIRBRUSH': sketchology.proto.ToolParams.ToolType.LINE,
  'CALLIGRAPHY': sketchology.proto.ToolParams.ToolType.LINE,
  'EDIT': sketchology.proto.ToolParams.ToolType.EDIT,
  'ERASER': sketchology.proto.ToolParams.ToolType.LINE,
  'HIGHLIGHTER': sketchology.proto.ToolParams.ToolType.LINE,
  'INKPEN': sketchology.proto.ToolParams.ToolType.LINE,
  'MAGIC_ERASE': sketchology.proto.ToolParams.ToolType.MAGIC_ERASE,
  'MARKER': sketchology.proto.ToolParams.ToolType.LINE,
  'PENCIL': sketchology.proto.ToolParams.ToolType.LINE,
  'BALLPOINT': sketchology.proto.ToolParams.ToolType.LINE,
  'BALLPOINT_IN_PEN_MODE_ELSE_MARKER':
      sketchology.proto.ToolParams.ToolType.LINE,
  'QUERY': sketchology.proto.ToolParams.ToolType.QUERY,
};


/**
 * @const {Object}
 */
ink.BrushModel.SHAPE_TO_BRUSHTYPE = {
  'AIRBRUSH': sketchology.proto.BrushType.AIRBRUSH,
  'CALLIGRAPHY': sketchology.proto.BrushType.CALLIGRAPHY,
  'ERASER': sketchology.proto.BrushType.ERASER,
  'HIGHLIGHTER': sketchology.proto.BrushType.HIGHLIGHTER,
  'INKPEN': sketchology.proto.BrushType.INKPEN,
  'MARKER': sketchology.proto.BrushType.MARKER,
  'BALLPOINT': sketchology.proto.BrushType.BALLPOINT,
  'BALLPOINT_IN_PEN_MODE_ELSE_MARKER':
      sketchology.proto.BrushType.BALLPOINT_IN_PEN_MODE_ELSE_MARKER,
  'PENCIL': sketchology.proto.BrushType.PENCIL,
};


/**
 * @param {string} color The color in hex.
 */
ink.BrushModel.prototype.setColor = function(color) {
  this.color_ = color;
  this.dispatchEvent(ink.BrushModel.EventType.CHANGE);
};


/**
 * @param {number} strokeWidth The brush's stroke width.
 */
ink.BrushModel.prototype.setStrokeWidth = function(strokeWidth) {
  this.strokeWidth_ = strokeWidth;
  this.dispatchEvent(ink.BrushModel.EventType.CHANGE);
};


/**
 * @param {boolean} isErasing Whether user is erasing or not.
 */
ink.BrushModel.prototype.setIsErasing = function(isErasing) {
  this.isErasing_ = isErasing;
  this.dispatchEvent(ink.BrushModel.EventType.CHANGE);
};


/**
 * @param {string} shape The brush shape, which is either a brush type or a tool
 * type.  If it's a brush type, implies tool type LINE.
 */
ink.BrushModel.prototype.setShape = function(shape) {
  this.toolType_ = ink.BrushModel.SHAPE_TO_TOOLTYPE[shape];
  this.brushType_ = ink.BrushModel.SHAPE_TO_BRUSHTYPE[shape] !== undefined ?
      ink.BrushModel.SHAPE_TO_BRUSHTYPE[shape] :
      this.brushType_;
  this.shape_ = shape;
  this.dispatchEvent(ink.BrushModel.EventType.CHANGE);
};


/**
 * @return {string} The last used shape.
 */
ink.BrushModel.prototype.getShape = function() {
  return this.shape_;
};


/**
 * @return {string} The last draw color in hex (excluding erase color).
 */
ink.BrushModel.prototype.getColor = function() {
  return this.color_;
};


/**
 * Gets the current color being drawn on the screen (including erase color).
 * @return {string} The brush color in hex.
 */
ink.BrushModel.prototype.getActiveColor = function() {
  if (!this.isErasing_) {
    return this.color_;
  } else {
    return ink.BrushModel.DEFAULT_ERASE_COLOR;
  }
};


/**
 * Wraps getActiveColor() by returning the numeric rgb of the color.
 * @return {number} The brush color in numeric rbg.
 */
ink.BrushModel.prototype.getActiveColorNumericRbg = function() {
  return parseInt(this.getActiveColor().substring(1), 16);
};


/**
 * @return {number} Percentage size for stroke width, [0, 1].
 *
 * See sengine.proto SizeType
 */
ink.BrushModel.prototype.getStrokeWidth = function() {
  return this.strokeWidth_;
};


/**
 * @return {boolean} Whether user is erasing.
 */
ink.BrushModel.prototype.getIsErasing = function() {
  return this.isErasing_;
};


/**
 * @return {sketchology.proto.BrushType} The brush type for line
 * tool.
 */
ink.BrushModel.prototype.getBrushType = function() {
  return this.brushType_;
};


/**
 * @return {sketchology.proto.ToolParams.ToolType} The tool type.
 */
ink.BrushModel.prototype.getToolType = function() {
  return this.toolType_;
};

