// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * Crop mode.
 *
 * @extends {ImageEditorMode}
 * @constructor
 * @struct
 */
ImageEditorMode.Crop = function() {
  ImageEditorMode.call(this, 'crop', 'GALLERY_CROP');

  this.paddingTop = ImageEditorMode.Crop.MOUSE_GRAB_RADIUS;
  this.paddingBottom = ImageEditorMode.Crop.MOUSE_GRAB_RADIUS;

  /**
   * @type {HTMLElement}
   * @private
   */
  this.domOverlay_ = null;

  /**
   * @type {HTMLElement}
   * @private
   */
  this.shadowTop_ = null;

  /**
   * @type {HTMLElement}
   * @private
   */
  this.middleBox_ = null;

  /**
   * @type {HTMLElement}
   * @private
   */
  this.shadowLeft_ = null;

  /**
   * @type {HTMLElement}
   * @private
   */
  this.cropFrame_ = null;

  /**
   * @type {HTMLElement}
   * @private
   */
  this.shadowRight_ = null;

  /**
   * @type {HTMLElement}
   * @private
   */
  this.shadowBottom_ = null;

  /**
   * @type {?function()}
   * @private
   */
  this.onViewportResizedBound_ = null;

  /**
   * @type {DraggableRect}
   * @private
   */
  this.cropRect_ = null;
};

ImageEditorMode.Crop.prototype = {
  __proto__: ImageEditorMode.prototype
};

/**
 * Sets the mode up.
 * @override
 */
ImageEditorMode.Crop.prototype.setUp = function() {
  ImageEditorMode.prototype.setUp.apply(this, arguments);

  var container = this.getImageView().container_;
  var doc = container.ownerDocument;

  this.domOverlay_ = /** @type {!HTMLElement} */ (doc.createElement('div'));
  this.domOverlay_.className = 'crop-overlay';
  container.appendChild(this.domOverlay_);

  this.shadowTop_ = /** @type {!HTMLElement} */ (doc.createElement('div'));
  this.shadowTop_.className = 'shadow';
  this.domOverlay_.appendChild(this.shadowTop_);

  this.middleBox_ = /** @type {!HTMLElement} */ (doc.createElement('div'));
  this.middleBox_.className = 'middle-box';
  this.domOverlay_.appendChild(this.middleBox_);

  this.shadowLeft_ = /** @type {!HTMLElement} */ (doc.createElement('div'));
  this.shadowLeft_.className = 'shadow';
  this.middleBox_.appendChild(this.shadowLeft_);

  this.cropFrame_ = /** @type {!HTMLElement} */ (doc.createElement('div'));
  this.cropFrame_.className = 'crop-frame';
  this.middleBox_.appendChild(this.cropFrame_);

  this.shadowRight_ = /** @type {!HTMLElement} */ (doc.createElement('div'));
  this.shadowRight_.className = 'shadow';
  this.middleBox_.appendChild(this.shadowRight_);

  this.shadowBottom_ = /** @type {!HTMLElement} */ (doc.createElement('div'));
  this.shadowBottom_.className = 'shadow';
  this.domOverlay_.appendChild(this.shadowBottom_);

  var cropFrame = this.cropFrame_;
  function addCropFrame(className) {
    var div = doc.createElement('div');
    div.className = className;
    cropFrame.appendChild(div);
  }

  addCropFrame('left top corner');
  addCropFrame('top horizontal');
  addCropFrame('right top corner');
  addCropFrame('left vertical');
  addCropFrame('right vertical');
  addCropFrame('left bottom corner');
  addCropFrame('bottom horizontal');
  addCropFrame('right bottom corner');

  this.onViewportResizedBound_ = this.onViewportResized_.bind(this);
  this.getViewport().addEventListener('resize', this.onViewportResizedBound_);

  this.createDefaultCrop();
};

/**
 * @override
 */
ImageEditorMode.Crop.prototype.createTools = function(toolbar) {
  var aspects = {
    GALLERY_ASPECT_RATIO_1_1: 1 / 1,
    GALLERY_ASPECT_RATIO_6_4: 6 / 4,
    GALLERY_ASPECT_RATIO_7_5: 7 / 5,
    GALLERY_ASPECT_RATIO_16_9: 16 / 9
  };

  // TODO(fukino): The loop order is not guaranteed. Fix it!
  for (var name in aspects) {
    var button = toolbar.addButton(
        name, ImageEditorToolbar.ButtonType.LABEL,
        this.onCropAspectRatioClicked_.bind(this, toolbar, aspects[name]),
        'crop-aspect-ratio');

    // Prevent from cropping by Enter key if the button is focused.
    button.addEventListener('keydown', function(event) {
      var key = util.getKeyModifiers(event) + event.key;
      if (key === 'Enter')
        event.stopPropagation();
    });
  }
};

/**
 * Handles click events of crop aspect ratio buttons.
 * @param {!ImageEditorToolbar} toolbar Toolbar.
 * @param {number} aspect Aspect ratio.
 * @param {Event} event An event.
 * @private
 */
ImageEditorMode.Crop.prototype.onCropAspectRatioClicked_ = function(
    toolbar, aspect, event) {
  var button = event.target;

  if (button.classList.contains('selected')) {
    button.classList.remove('selected');
    this.cropRect_.fixedAspectRatio = null;
  } else {
    var selectedButtons =
        toolbar.getElement().querySelectorAll('button.selected');
    for (var i = 0; i < selectedButtons.length; i++) {
      selectedButtons[i].classList.remove('selected');
    }
    button.classList.add('selected');
    var clipRect = this.viewport_.screenToImageRect(
        this.viewport_.getImageBoundsOnScreenClipped());
    this.cropRect_.fixedAspectRatio = aspect;
    this.cropRect_.forceAspectRatio(aspect, clipRect);
    this.markUpdated();
    this.positionDOM();
  }
};

/**
 * Handles resizing of the viewport and updates the crop rectangle.
 * @private
 */
ImageEditorMode.Crop.prototype.onViewportResized_ = function() {
  this.positionDOM();
};

/**
 * Resets the mode.
 */
ImageEditorMode.Crop.prototype.reset = function() {
  ImageEditorMode.prototype.reset.call(this);
  this.createDefaultCrop();
};

/**
 * Updates the position of DOM elements.
 */
ImageEditorMode.Crop.prototype.positionDOM = function() {
  var screenCrop = this.viewport_.imageToScreenRect(this.cropRect_.getRect());

  this.shadowLeft_.style.width = screenCrop.left + 'px';
  this.shadowTop_.style.height = screenCrop.top + 'px';
  this.shadowRight_.style.width = window.innerWidth - screenCrop.right + 'px';
  this.shadowBottom_.style.height =
      window.innerHeight - screenCrop.bottom + 'px';
};

/**
 * Removes the overlay elements from the document.
 */
ImageEditorMode.Crop.prototype.cleanUpUI = function() {
  ImageEditorMode.prototype.cleanUpUI.apply(this, arguments);
  this.domOverlay_.parentNode.removeChild(this.domOverlay_);
  this.domOverlay_ = null;
  this.getViewport().removeEventListener(
      'resize', this.onViewportResizedBound_);
  this.onViewportResizedBound_ = null;
};

/**
 * @const
 * @type {number}
 */
ImageEditorMode.Crop.MOUSE_GRAB_RADIUS = 6;

/**
 * @const
 * @type {number}
 */
ImageEditorMode.Crop.TOUCH_GRAB_RADIUS = 20;

/**
 * Gets command to do the crop depending on the current state.
 *
 * @return {!Command.Crop} Crop command.
 */
ImageEditorMode.Crop.prototype.getCommand = function() {
  var cropImageRect = this.cropRect_.getRect();
  return new Command.Crop(cropImageRect);
};

/**
 * Creates default (initial) crop.
 */
ImageEditorMode.Crop.prototype.createDefaultCrop = function() {
  var viewport = this.getViewport();
  assert(viewport);

  var rect = viewport.screenToImageRect(
      viewport.getImageBoundsOnScreenClipped());
  rect = rect.inflate(
      -Math.round(rect.width / 6), -Math.round(rect.height / 6));

  this.cropRect_ = new DraggableRect(rect, viewport);

  this.positionDOM();
};

/**
 * Obtains the cursor style depending on the mouse state.
 *
 * @param {number} x X coordinate for cursor.
 * @param {number} y Y coordinate for cursor.
 * @param {boolean} mouseDown If mouse button is down.
 * @return {string} A value for style.cursor CSS property.
 */
ImageEditorMode.Crop.prototype.getCursorStyle = function(x, y, mouseDown) {
  return this.cropRect_.getCursorStyle(x, y, mouseDown);
};

/**
 * Obtains handler function depending on the mouse state.
 *
 * @param {number} x Event X coordinate.
 * @param {number} y Event Y coordinate.
 * @param {boolean} touch True if it's a touch event, false if mouse.
 * @return {?function(number,number,boolean)} A function to be called on mouse
 *     drag. It takes x coordinate value, y coordinate value, and shift key
 *     flag.
 */
ImageEditorMode.Crop.prototype.getDragHandler = function(x, y, touch) {
  var cropDragHandler = this.cropRect_.getDragHandler(x, y, touch);
  if (!cropDragHandler)
    return null;

  return function(x, y, shiftKey) {
    cropDragHandler(x, y, shiftKey);
    this.markUpdated();
    this.positionDOM();
  }.bind(this);
};

/**
 * Obtains the double tap action depending on the coordinate.
 *
 * @param {number} x X coordinate of the event.
 * @param {number} y Y coordinate of the event.
 * @return {!ImageBuffer.DoubleTapAction} Action to perform as result.
 */
ImageEditorMode.Crop.prototype.getDoubleTapAction = function(x, y) {
  return this.cropRect_.getDoubleTapAction(x, y);
};

/**
 * A draggable rectangle over the image.
 *
 * @param {!ImageRect} rect Initial size of the image.
 * @param {!Viewport} viewport Viewport.
 * @constructor
 * @struct
 */
function DraggableRect(rect, viewport) {
  /**
   * The bounds are not held in a regular rectangle (with width/height).
   * left/top/right/bottom held instead for convenience.
   *
   * @type {{left: number, right: number, top: number, bottom: number}}
   * @private
   */
  this.bounds_ = {
    left: rect.left,
    right: rect.left + rect.width,
    top: rect.top,
    bottom: rect.top + rect.height
  };

  /**
   * Viewport.
   *
   * @type {!Viewport}
   * @private
   * @const
   */
  this.viewport_ = viewport;

  /**
   * Drag mode.
   *
   * @type {Object}
   * @private
   */
  this.dragMode_ = null;

  /**
   * Fixed aspect ratio.
   * The aspect ratio is not fixed when null.
   * @type {?number}
   */
  this.fixedAspectRatio = null;
}

// Static members to simplify reflective access to the bounds.
/**
 * @const
 * @type {string}
 */
DraggableRect.LEFT = 'left';

/**
 * @const
 * @type {string}
 */
DraggableRect.RIGHT = 'right';

/**
 * @const
 * @type {string}
 */
DraggableRect.TOP = 'top';

/**
 * @const
 * @type {string}
 */
DraggableRect.BOTTOM = 'bottom';

/**
 * @const
 * @type {string}
 */
DraggableRect.NONE = 'none';

/**
 * Obtains the left position.
 * @return {number} Position.
 */
DraggableRect.prototype.getLeft = function() {
  return this.bounds_[DraggableRect.LEFT];
};

/**
 * Obtains the right position.
 * @return {number} Position.
 */
DraggableRect.prototype.getRight = function() {
  return this.bounds_[DraggableRect.RIGHT];
};

/**
 * Obtains the top position.
 * @return {number} Position.
 */
DraggableRect.prototype.getTop = function() {
  return this.bounds_[DraggableRect.TOP];
};

/**
 * Obtains the bottom position.
 * @return {number} Position.
 */
DraggableRect.prototype.getBottom = function() {
  return this.bounds_[DraggableRect.BOTTOM];
};

/**
 * Obtains the geometry of the rectangle.
 * @return {!ImageRect} Geometry of the rectangle.
 */
DraggableRect.prototype.getRect = function() {
  return ImageRect.createFromBounds(this.bounds_);
};

/**
 * Obtains the drag mode depending on the coordinate.
 *
 * @param {number} x X coordinate for cursor.
 * @param {number} y Y coordinate for cursor.
 * @param {boolean=} opt_touch  Whether the operation is done by touch or not.
 * @return {{xSide: string, ySide:string, whole:boolean, newCrop:boolean}}
 *     Drag mode.
 */
DraggableRect.prototype.getDragMode = function(x, y, opt_touch) {
  var touch = opt_touch || false;

  var result = {
    xSide: DraggableRect.NONE,
    ySide: DraggableRect.NONE,
    whole: false,
    newCrop: false
  };

  var bounds = this.bounds_;
  var R = this.viewport_.screenToImageSize(
      touch ? ImageEditorMode.Crop.TOUCH_GRAB_RADIUS :
              ImageEditorMode.Crop.MOUSE_GRAB_RADIUS);

  var circle = new Circle(x, y, R);

  var xBetween = ImageUtil.between(bounds.left, x, bounds.right);
  var yBetween = ImageUtil.between(bounds.top, y, bounds.bottom);

  if (circle.inside(bounds.left, bounds.top)) {
    result.xSide = DraggableRect.LEFT;
    result.ySide = DraggableRect.TOP;
  } else if (circle.inside(bounds.left, bounds.bottom)) {
    result.xSide = DraggableRect.LEFT;
    result.ySide = DraggableRect.BOTTOM;
  } else if (circle.inside(bounds.right, bounds.top)) {
    result.xSide = DraggableRect.RIGHT;
    result.ySide = DraggableRect.TOP;
  } else if (circle.inside(bounds.right, bounds.bottom)) {
    result.xSide = DraggableRect.RIGHT;
    result.ySide = DraggableRect.BOTTOM;
  } else if (yBetween && Math.abs(x - bounds.left) <= R) {
    result.xSide = DraggableRect.LEFT;
  } else if (yBetween && Math.abs(x - bounds.right) <= R) {
    result.xSide = DraggableRect.RIGHT;
  } else if (xBetween && Math.abs(y - bounds.top) <= R) {
    result.ySide = DraggableRect.TOP;
  } else if (xBetween && Math.abs(y - bounds.bottom) <= R) {
    result.ySide = DraggableRect.BOTTOM;
  } else if (xBetween && yBetween) {
    result.whole = true;
  } else {
    result.newcrop = true;
    result.xSide = DraggableRect.RIGHT;
    result.ySide = DraggableRect.BOTTOM;
  }

  return result;
};

/**
 * Obtains the cursor style depending on the coordinate.
 *
 * @param {number} x X coordinate for cursor.
 * @param {number} y Y coordinate for cursor.
 * @param {boolean} mouseDown  If mouse button is down.
 * @return {string} Cursor style.
 */
DraggableRect.prototype.getCursorStyle = function(x, y, mouseDown) {
  var mode;
  if (mouseDown) {
    mode = this.dragMode_;
  } else {
    mode = this.getDragMode(
        this.viewport_.screenToImageX(x), this.viewport_.screenToImageY(y));
  }
  if (mode.whole)
    return 'move';
  if (mode.newcrop)
    return 'crop';

  var xSymbol = '';
  switch (mode.xSide) {
    case 'left': xSymbol = 'w'; break;
    case 'right': xSymbol = 'e'; break;
  }
  var ySymbol = '';
  switch (mode.ySide) {
    case 'top': ySymbol = 'n'; break;
    case 'bottom': ySymbol = 's'; break;
  }
  return ySymbol + xSymbol + '-resize';
};

/**
 * Obtains the drag handler depending on the coordinate.
 *
 * @param {number} initialScreenX X coordinate for cursor in the screen.
 * @param {number} initialScreenY Y coordinate for cursor in the screen.
 * @param {boolean} touch Whether the operation is done by touch or not.
 * @return {?function(number,number,boolean)} Drag handler that takes x
 *     coordinate value, y coordinate value, and shift key flag.
 */
DraggableRect.prototype.getDragHandler = function(
    initialScreenX, initialScreenY, touch) {
  // Check if the initial coordinate is in the image rect.
  var boundsOnScreen = this.viewport_.getImageBoundsOnScreenClipped();
  var handlerRadius = touch ? ImageEditorMode.Crop.TOUCH_GRAB_RADIUS :
                              ImageEditorMode.Crop.MOUSE_GRAB_RADIUS;
  var draggableAreas = [
      boundsOnScreen,
      new Circle(boundsOnScreen.left, boundsOnScreen.top, handlerRadius),
      new Circle(boundsOnScreen.right, boundsOnScreen.top, handlerRadius),
      new Circle(boundsOnScreen.left, boundsOnScreen.bottom, handlerRadius),
      new Circle(boundsOnScreen.right, boundsOnScreen.bottom, handlerRadius)
  ];

  if (!draggableAreas.some(
      (area) => area.inside(initialScreenX, initialScreenY))) {
    return null;
  }

  // Convert coordinates.
  var initialX = this.viewport_.screenToImageX(initialScreenX);
  var initialY = this.viewport_.screenToImageY(initialScreenY);
  var initialWidth = this.bounds_.right - this.bounds_.left;
  var initialHeight = this.bounds_.bottom - this.bounds_.top;
  var clipRect = this.viewport_.screenToImageRect(boundsOnScreen);

  // Obtain the drag mode.
  this.dragMode_ = this.getDragMode(initialX, initialY, touch);

  if (this.dragMode_.whole) {
    // Calc constant values during the operation.
    var mouseBiasX = this.bounds_.left - initialX;
    var mouseBiasY = this.bounds_.top - initialY;
    var maxX = clipRect.left + clipRect.width - initialWidth;
    var maxY = clipRect.top + clipRect.height - initialHeight;

    // Returns a handler.
    return function(newScreenX, newScreenY) {
      var newX = this.viewport_.screenToImageX(newScreenX);
      var newY = this.viewport_.screenToImageY(newScreenY);
      var clamppedX = ImageUtil.clamp(clipRect.left, newX + mouseBiasX, maxX);
      var clamppedY = ImageUtil.clamp(clipRect.top, newY + mouseBiasY, maxY);
      this.bounds_.left = clamppedX;
      this.bounds_.right = clamppedX + initialWidth;
      this.bounds_.top = clamppedY;
      this.bounds_.bottom = clamppedY + initialHeight;
    }.bind(this);
  } else {
    // Calc constant values during the operation.
    var mouseBiasX = this.bounds_[this.dragMode_.xSide] - initialX;
    var mouseBiasY = this.bounds_[this.dragMode_.ySide] - initialY;
    var maxX = clipRect.left + clipRect.width;
    var maxY = clipRect.top + clipRect.height;

    // Returns a handler.
    return function(newScreenX, newScreenY, shiftKey) {
      var newX = this.viewport_.screenToImageX(newScreenX);
      var newY = this.viewport_.screenToImageY(newScreenY);

      // Check new crop.
      if (this.dragMode_.newcrop) {
        this.dragMode_.newcrop = false;
        this.bounds_.left = this.bounds_.right = initialX;
        this.bounds_.top = this.bounds_.bottom = initialY;
        mouseBiasX = 0;
        mouseBiasY = 0;
      }

      // Update X coordinate.
      if (this.dragMode_.xSide !== DraggableRect.NONE) {
        this.bounds_[this.dragMode_.xSide] =
            ImageUtil.clamp(clipRect.left, newX + mouseBiasX, maxX);
        if (this.bounds_.left > this.bounds_.right) {
          var left = this.bounds_.left;
          var right = this.bounds_.right;
          this.bounds_.left = right - 1;
          this.bounds_.right = left + 1;
          this.dragMode_.xSide =
              this.dragMode_.xSide == 'left' ? 'right' : 'left';
        }
      }

      // Update Y coordinate.
      if (this.dragMode_.ySide !== DraggableRect.NONE) {
        this.bounds_[this.dragMode_.ySide] =
            ImageUtil.clamp(clipRect.top, newY + mouseBiasY, maxY);
        if (this.bounds_.top > this.bounds_.bottom) {
          var top = this.bounds_.top;
          var bottom = this.bounds_.bottom;
          this.bounds_.top = bottom - 1;
          this.bounds_.bottom = top + 1;
          this.dragMode_.ySide =
              this.dragMode_.ySide === 'top' ? 'bottom' : 'top';
        }
      }

      // Update aspect ratio.
      if (this.fixedAspectRatio)
        this.forceAspectRatio(this.fixedAspectRatio, clipRect);
      else if (shiftKey)
        this.forceAspectRatio(initialWidth / initialHeight, clipRect);
    }.bind(this);
  }
};

/**
 * Obtains double tap action depending on the coordinate.
 *
 * @param {number} x X coordinate for cursor.
 * @param {number} y Y coordinate for cursor.
 * @return {!ImageBuffer.DoubleTapAction} Double tap action.
 */
DraggableRect.prototype.getDoubleTapAction = function(x, y) {
  var clipRect = this.viewport_.getImageBoundsOnScreenClipped();
  if (clipRect.inside(x, y))
    return ImageBuffer.DoubleTapAction.COMMIT;
  else
    return ImageBuffer.DoubleTapAction.NOTHING;
};

/**
 * Forces the aspect ratio.
 *
 * @param {number} aspectRatio Aspect ratio.
 * @param {!Object} clipRect Clip rect.
 */
DraggableRect.prototype.forceAspectRatio = function(aspectRatio, clipRect) {
  // Get current rectangle scale.
  var width = this.bounds_.right - this.bounds_.left;
  var height = this.bounds_.bottom - this.bounds_.top;
  var currentScale;
  if (!this.dragMode_)
    currentScale = ((width / aspectRatio) + height) / 2;
  else if (this.dragMode_.xSide === 'none')
    currentScale = height;
  else if (this.dragMode_.ySide === 'none')
    currentScale = width / aspectRatio;
  else
    currentScale = Math.max(width / aspectRatio, height);

  // Get maximum width/height scale.
  var maxWidth;
  var maxHeight;
  var center = (this.bounds_.left + this.bounds_.right) / 2;
  var middle = (this.bounds_.top + this.bounds_.bottom) / 2;
  var xSide = this.dragMode_ ? this.dragMode_.xSide : 'none';
  var ySide = this.dragMode_ ? this.dragMode_.ySide : 'none';
  switch (xSide) {
    case 'left':
      maxWidth = this.bounds_.right - clipRect.left;
      break;
    case 'right':
      maxWidth = clipRect.left + clipRect.width - this.bounds_.left;
      break;
    case 'none':
      maxWidth = Math.min(
          clipRect.left + clipRect.width - center,
          center - clipRect.left) * 2;
      break;
  }
  switch (ySide) {
    case 'top':
      maxHeight = this.bounds_.bottom - clipRect.top;
      break;
    case 'bottom':
      maxHeight = clipRect.top + clipRect.height - this.bounds_.top;
      break;
    case 'none':
      maxHeight = Math.min(
          clipRect.top + clipRect.height - middle,
          middle - clipRect.top) * 2;
      break;
  }

  // Obtains target scale.
  var targetScale = Math.min(
      currentScale,
      maxWidth / aspectRatio,
      maxHeight);

  // Update bounds.
  var newWidth = targetScale * aspectRatio;
  var newHeight = targetScale;
  switch (xSide) {
    case 'left':
      this.bounds_.left = this.bounds_.right - newWidth;
      break;
    case 'right':
      this.bounds_.right = this.bounds_.left + newWidth;
      break;
    case 'none':
      this.bounds_.left = center - newWidth / 2;
      this.bounds_.right = center + newWidth / 2;
      break;
  }
  switch (ySide) {
    case 'top':
      this.bounds_.top = this.bounds_.bottom - newHeight;
      break;
    case 'bottom':
      this.bounds_.bottom = this.bounds_.top + newHeight;
      break;
    case 'none':
      this.bounds_.top = middle - newHeight / 2;
      this.bounds_.bottom = middle + newHeight / 2;
      break;
  }
};
