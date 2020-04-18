// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * Resize Mode
 *
 * @extends {ImageEditorMode}
 * @constructor
 * @struct
 */
ImageEditorMode.Resize = function() {
  ImageEditorMode.call(this, 'resize', 'GALLERY_RESIZE');

  /** @private {number} */
  this.imageWidth_ = 0;

  /** @private {number} */
  this.imageHeight_ = 0;

  /** @private {number} */
  this.maxValidWidth_ = 0;

  /** @private {number} */
  this.maxValidHeight_ = 0;

  /** @private {number} */
  this.widthInputValue_ = 0;

  /** @private {number} */
  this.heightInputValue_ = 0;

  /** @private {HTMLElement} */
  this.widthInputElement_ = null;

  /** @private {HTMLElement} */
  this.heightInputElement_ = null;

  /** @private {HTMLElement} */
  this.lockIcon_ = null;

  /** @private {number} */
  this.ratio_ = 0;

  /** @private {boolean} */
  this.fixedRatio_ = true;

  /** @private {boolean} */
  this.showingAlertDialog_ = false;

  /** @private {FilesAlertDialog} */
  this.alertDialog_ = null;
};

/** @const {number} */
ImageEditorMode.Resize.MINIMUM_VALID_VALUE = 1;

/** @const {number} */
ImageEditorMode.Resize.DEFAULT_MAX_VALID_VALUE = 10000;

ImageEditorMode.Resize.prototype = {
  __proto__: ImageEditorMode.prototype
};

/** @override */
ImageEditorMode.Resize.prototype.setUp = function() {
  ImageEditorMode.prototype.setUp.apply(this, arguments);

  this.setDefault_();
};

/** @override */
ImageEditorMode.Resize.prototype.createTools = function(toolbar) {
  this.widthInputElement_ = toolbar.addInput('width', 'GALLERY_WIDTH',
      this.onInputChanged_.bind(this, 'width'),
      this.widthInputValue_, 'px');

  this.lockIcon_ = toolbar.addButton(
      'GALLERY_FIXRATIO', ImageEditorToolbar.ButtonType.ICON_TOGGLEABLE,
      this.onLockIconClicked_.bind(this), 'lockicon');
  if(this.fixedRatio_)
    this.lockIcon_.setAttribute('locked', '');

  this.heightInputElement_ = toolbar.addInput('height', 'GALLERY_HEIGHT',
      this.onInputChanged_.bind(this, 'height'),
      this.heightInputValue_, 'px');
};

/**
 * Handlers change events of width/height input element.
 * @param {string} name
 * @param {Event} event
 * @private
 */
ImageEditorMode.Resize.prototype.onInputChanged_ = function(name, event) {

  if(name !== 'height' && name !== 'width')
    return;

  this.updateInputValues_();

  function adjustToRatio() {
    switch (name) {
      case 'width':
        var newHeight = Math.ceil(this.widthInputValue_ / this.ratio_);
        if (this.isInputValidByName_('height', newHeight)) {
          this.heightInputValue_ = newHeight;
          this.setHeightInputValue_();
        }
        break;
      case 'height':
        var newWidth = Math.ceil(this.heightInputValue_ * this.ratio_);
        if(this.isInputValidByName_('width', newWidth)) {
          this.widthInputValue_ = newWidth;
          this.setWidthInputValue_();
        }
        break;
    }
  }

  if(this.fixedRatio_ && this.isInputValidByName_(name))
    adjustToRatio.call(this);
};

/**
 * Handlers change events of lock icon.
 * @param {Event} event An event.
 * @private
 */
ImageEditorMode.Resize.prototype.onLockIconClicked_ = function(event) {
  var toggled = !this.fixedRatio_;
  if(toggled) {
    this.initializeInputValues_();
    this.lockIcon_.setAttribute('locked', '');
  } else {
    this.lockIcon_.removeAttribute('locked');
  }

  this.fixedRatio_ = toggled;
};

/**
 * Set default values.
 * @private
 */
ImageEditorMode.Resize.prototype.setDefault_ = function() {
  var viewport = this.getViewport();
  assert(viewport);

  var rect = viewport.getImageBounds();
  this.imageWidth_ = rect.width;
  this.imageHeight_ = rect.height;

  this.initializeInputValues_();

  this.ratio_ = this.imageWidth_ / this.imageHeight_;

  this.maxValidWidth_ = Math.max(
      this.imageWidth_, ImageEditorMode.Resize.DEFAULT_MAX_VALID_VALUE);
  this.maxValidHeight_ = Math.max(
      this.imageHeight_, ImageEditorMode.Resize.DEFAULT_MAX_VALID_VALUE);
};

/**
 * Initialize width/height input values.
 * @private
 */
ImageEditorMode.Resize.prototype.initializeInputValues_ = function() {
  this.widthInputValue_ = this.imageWidth_;
  this.setWidthInputValue_();

  this.heightInputValue_ = this.imageHeight_;
  this.setHeightInputValue_();
};

/**
 * Update input values to local variales.
 * @private
 */
ImageEditorMode.Resize.prototype.updateInputValues_ = function() {
  if(this.widthInputElement_)
    this.widthInputValue_ = parseInt(this.widthInputElement_.getValue(), 10);
  if(this.heightInputElement_)
    this.heightInputValue_ = parseInt(this.heightInputElement_.getValue(), 10);
};

/**
 * Apply local variables' change to width input element.
 * @private
 */
ImageEditorMode.Resize.prototype.setWidthInputValue_ = function() {
  if(this.widthInputElement_)
    this.widthInputElement_.setValue(this.widthInputValue_);
};

/**
 * Apply local variables' change to height input element.
 * @private
 */
ImageEditorMode.Resize.prototype.setHeightInputValue_ = function() {
  if(this.heightInputElement_)
    this.heightInputElement_.setValue(this.heightInputValue_);
};

/**
 * Check if the given name of input has a valid value.
 *
 * @param {string} name Name of input to check.
 * @param {number=} opt_value Value to be checked. Local input's variable will
       be checked if undefined.
 * @return {boolean} True if the input
 * @private
 */
ImageEditorMode.Resize.prototype.isInputValidByName_ = function(
    name, opt_value) {
  console.assert(name === 'height' || name === 'width');

  var limit = name === 'width' ? this.maxValidWidth_ : this.maxValidHeight_;
  var value = opt_value ||
      (name === 'width' ? this.widthInputValue_ : this.heightInputValue_);

  return ImageEditorMode.Resize.MINIMUM_VALID_VALUE <= value && value <= limit;
};

/**
 * Check if width/height input values are valid.
 * @return {boolean} true if both values are valid.
 */
ImageEditorMode.Resize.prototype.isInputValid = function() {
  return this.isInputValidByName_('width') &&
      this.isInputValidByName_('height');
};

/**
 * Show alert dialog only if input value is invalid.
 */
ImageEditorMode.Resize.prototype.showAlertDialog = function() {
  if(this.isInputValid() || this.showingAlertDialog_)
    return;

  this.alertDialog_ = this.alertDialog_ ||
      new FilesAlertDialog(/** @type {!HTMLElement} */ (document.body));
  this.showingAlertDialog_ = true;
  this.alertDialog_.showWithTitle('', strf('GALLERY_INVALIDVALUE'),
      function() {
        this.initializeInputValues_();
        this.showingAlertDialog_ = false;
      }.bind(this), null, null);
};

/**
 * @return {boolean} True when showing an alert dialog.
 * @override
 */
ImageEditorMode.Resize.prototype.isConsumingKeyEvents = function() {
  return this.showingAlertDialog_;
};

/** @override */
ImageEditorMode.Resize.prototype.cleanUpUI = function() {
  ImageEditorMode.prototype.cleanUpUI.apply(this, arguments);

  if(this.showingAlertDialog_) {
    this.alertDialog_.hide();
    this.showingAlertDialog_ = false;
  }
};

/** @override */
ImageEditorMode.Resize.prototype.reset = function() {
  ImageEditorMode.prototype.reset.call(this);

  this.setDefault_();
};

/** @override */
ImageEditorMode.Resize.prototype.getCommand = function() {
  return new Command.Resize(this.widthInputValue_, this.heightInputValue_);
};
