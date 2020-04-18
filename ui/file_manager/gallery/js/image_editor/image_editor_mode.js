// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * ImageEditorMode represents a modal state dedicated to a specific operation.
 * Inherits from ImageBuffer. Overlay to simplify the drawing of mode-specific
 * tools.
 *
 * @param {string} name The mode name.
 * @param {string} title The mode title.
 * @constructor
 * @struct
 * @extends {ImageBuffer.Overlay}
 */
function ImageEditorMode(name, title) {
  this.name = name;
  this.title = title;
  this.message_ = 'GALLERY_ENTER_WHEN_DONE';

  /**
   * @type {boolean}
   */
  this.implicitCommit = false;

  /**
   * @type {boolean}
   */
  this.instant = false;

  /**
   * @type {number}
   */
  this.paddingTop = 0;

  /**
   * @type {number}
   */
  this.paddingBottom = 0;

  /**
   * @type {Viewport}
   */
  this.viewport_ = null;

  /**
   * @type {HTMLElement}
   */
  this.button_ = null;

  /**
   * @type {ImageBuffer}
   * @private
   */
  this.buffer_ = null;

  /**
   * @type {boolean}
   */
  this.updated_ = false;

  /**
   * @type {ImageView}
   * @private
   */
  this.imageView_ = null;
}

ImageEditorMode.prototype = {
  __proto__: ImageBuffer.Overlay.prototype
};

/**
 * @return {Viewport} Viewport instance.
 */
ImageEditorMode.prototype.getViewport = function() {
  return this.viewport_;
};

/**
 * @return {ImageView} ImageView instance.
 */
ImageEditorMode.prototype.getImageView = function() {
  return this.imageView_;
};

/**
 * @return {string} The mode-specific message to be displayed when entering.
 */
ImageEditorMode.prototype.getMessage = function() {
  return this.message_;
};

/**
 * @return {boolean} True if the mode is applicable in the current context.
 */
ImageEditorMode.prototype.isApplicable = function() {
  return true;
};

/**
 * Called once after creating the mode button.
 *
 * @param {!HTMLElement} button The mode button.
 * @param {!ImageBuffer} buffer
 * @param {!Viewport} viewport
 * @param {!ImageView} imageView
 */

ImageEditorMode.prototype.bind = function(button, buffer, viewport, imageView) {
  this.button_ = button;
  this.buffer_ = buffer;
  this.viewport_ = viewport;
  this.imageView_ = imageView;
};

/**
 * Called before entering the mode.
 */
ImageEditorMode.prototype.setUp = function() {
  this.buffer_.addOverlay(this);
  this.updated_ = false;
};

/**
 * Create mode-specific controls here.
 * @param {!ImageEditorToolbar} toolbar The toolbar to populate.
 */
ImageEditorMode.prototype.createTools = function(toolbar) {};

/**
 * Called before exiting the mode.
 */
ImageEditorMode.prototype.cleanUpUI = function() {
  this.buffer_.removeOverlay(this);
};

/**
 * Called after exiting the mode.
 */
ImageEditorMode.prototype.cleanUpCaches = function() {};

/**
 * Called when any of the controls changed its value.
 * @param {Object} options A map of options.
 */
ImageEditorMode.prototype.update = function(options) {
  this.markUpdated();
};

/**
 * Mark the editor mode as updated.
 */
ImageEditorMode.prototype.markUpdated = function() {
  this.updated_ = true;
};

/**
 * @return {boolean} True if the mode controls changed.
 */
ImageEditorMode.prototype.isUpdated = function() {
  return this.updated_;
};

/**
 * @return {boolean} True if a key event should be consumed by the mode.
 */
ImageEditorMode.prototype.isConsumingKeyEvents = function() {
  return false;
};

/**
 * Resets the mode to a clean state.
 */
ImageEditorMode.prototype.reset = function() {
  this.updated_ = false;
};

/**
 * @return {Command} Command.
 */
ImageEditorMode.prototype.getCommand = function() {
  return null;
};

/**
 * One-click editor tool, requires no interaction, just executes the command.
 *
 * @param {string} name The mode name.
 * @param {string} title The mode title.
 * @param {!Command} command The command to execute on click.
 * @constructor
 * @extends {ImageEditorMode}
 * @struct
 */
ImageEditorMode.OneClick = function(name, title, command) {
  ImageEditorMode.call(this, name, title);
  this.instant = true;
  this.command_ = command;
};

ImageEditorMode.OneClick.prototype = {
  __proto__: ImageEditorMode.prototype
};

/**
 * @override
 */
ImageEditorMode.OneClick.prototype.getCommand = function() {
  return this.command_;
};
