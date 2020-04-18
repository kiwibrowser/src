// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
goog.provide('ink.CanvasManager');

goog.require('goog.array');
goog.require('goog.asserts');
goog.require('goog.html.SafeUrl');
goog.require('goog.structs.Set');
goog.require('goog.ui.Component');
goog.require('ink.BrushModel');
goog.require('ink.Color');
goog.require('ink.ElementListener');
goog.require('ink.SketchologyEngineWrapper');
goog.require('ink.embed.events');
goog.require('ink.util');
goog.require('sketchology.proto.BackgroundImageInfo');
goog.require('sketchology.proto.Border');
goog.require('sketchology.proto.ImageExport');
goog.require('sketchology.proto.Rect');
goog.require('sketchology.proto.SetCallbackFlags');



/**
 * The controller of the canvas used for drawing.
 *
 * @param {?string} engineUrl
 * @param {ink.util.SEngineType} sengineType
 * @struct
 * @constructor
 * @extends {goog.ui.Component}
 * @implements {ink.ElementListener}
 */
ink.CanvasManager = function(engineUrl, sengineType) {
  ink.CanvasManager.base(this, 'constructor');

  /** @private {ink.BrushModel} */
  this.brushModel_ = null;

  /** @private {!ink.SketchologyEngineWrapper} */
  this.engine_ = new ink.SketchologyEngineWrapper(
      engineUrl, this, goog.bind(this.onPngExportComplete_, this), sengineType);
  this.addChild(this.engine_);
  this.getHandler().listenOnce(
      this.engine_, ink.SketchologyEngineWrapper.EventType.CANVAS_INITIALIZED,
      goog.bind(function() {
        this.brushUpdate_();
        this.setBorderImage_();
        this.dispatchEvent(ink.embed.events.EventType.CANVAS_INITIALIZED);
      }, this));

  // Redispatch CANVAS_FATAL_ERROR events as the top level FATAL_ERROR.
  this.getHandler().listen(
      this.engine_, ink.SketchologyEngineWrapper.EventType.CANVAS_FATAL_ERROR,
      this.dispatchFatalError_);

  this.getHandler().listen(
      this.engine_, ink.SketchologyEngineWrapper.EventType.PEN_MODE_ENABLED,
      (ev) => {
        this.dispatchEvent(new ink.embed.events.PenModeEnabled(ev.enabled));
      });

  /**
   * Known element UUIDs from bottom to top
   * @private {Array.<string>}
   */
  this.UUIDs_ = [];

  /**
   * Set of UUIDs created by the engine but not yet acknowledged by Brix.
   * @private {goog.structs.Set}}
   */
  this.pendingUUIDs_ = new goog.structs.Set();

  /**
   * Next local ID to use for Brix element bundles missing IDs.
   * @private {number}
   */
  this.nextLocalId_ = 0;

  /**
   * @const
   * @type {string}
   */
  this.FAKE_UUID = 'fake';

  /**
   * Background counter
   * @type {number}
   * @private
   */
  this.bgCount_ = 0;

  /** @private {?function(!goog.html.SafeUrl)} */
  this.onPngExportCompleteCallback_ = null;

  /** @private {boolean} */
  this.exportAsBlob_ = false;
};
goog.inherits(ink.CanvasManager, goog.ui.Component);


/** @const */
ink.CanvasManager.BORDER_IMAGE = 'data:image/png;base64,iVBORw0KGgoAAAANSUhEU' +
    'gAAAFgAAABYCAYAAABxlTA0AAAABmJLR0QA/wD/AP+gvaeTAAAACXBIWXMAAAsTAAALEwEAm' +
    'pwYAAAAB3RJTUUH4AgPEBYrHoEFUgAAAw1JREFUeNrt3b9uE0EQBvBvZvd8LURUkVwEkEDAm' +
    '9AgQYOQKHgZWloqUMoUaVJSpAivQBEhJGhSWiAlKXz7JwXey9nE9vrOgHT3fZKVFM5F/mUyN' +
    '5tIY8F2EtG/yNYucnZ21keg/57d3V1RMvzdEJjABGYITGACM+1iNz5RxGGPzCIbnT+i3RA1A' +
    'jgFcADgR4zRDwTVABjHGF8AeJwOaDnYNgd29jGGED6JyMvxeDwZYvVOJpN3FxcXH0TkWfMov' +
    'Qpac6p39jgdMi4A7Ozs/HLOvQkhfEkurW9yDVh47+GcOxgybsre3t5P59y+937OqHUFhxAQQ' +
    'kBVVd85E/yOc+5bcuk0pqWLeO/hvQ+krV289x45yLqqPSRk5xym0yllZ5lOp6iqCiGEtW3C5' +
    'lZwzq/DUOK9h4jUwK3GtGYFz1oEZa97cA2crJaNapozoiVk5rqCm+2h898ihn487uKiWPMPy' +
    '2arYG7G7TQHM91CYAITmCEwgQnMEJjABGYITGCGwAQmMENgAhOYITCBCUwCAhOYITCBCcwQm' +
    'MAEZghMYIbABCYwQ2ACE5ghMIEJzBCYwAyBCUxghsAEJjBDYAIzBP63wFkLcUX4lhAp3nu79' +
    'Qr23t8lbZ37WwNuLFx7fnJycnvossfHx7dijK+WGLWrYBFBjPHh5eXl/tHR0Z0h41ZV9T7G+' +
    'EREum/AThcREaiqqOpTAJ8PDw8/VlX11TnnhgArIrYsy3vn5+evy7J8NLNADrJd1xpUFcaY9' +
    'BBVfSAib2foAPq7GTAZGGNSkcFaC2stjDH161+BLGsrOAEXRYGyLOsNgKqK5hbovgIng/T6R' +
    '6MRiqKogVtVcPqi5k+tKIo5XOfcYICNMbDWYjQaoSzLP4BXtYqVFayqiDHCWlsjJnDnXPYG0' +
    'j5UcCqy9LDWtq/gZcipHxVFMbfitec3uMX70FwP7nyTW2zyaSt236v3pipO7UJVs9pDVgU3p' +
    '4n0jZqwQwBeHFlzYLPn4MXPb4Lt+5i2CJ1zgsuu4GUX2vBNk3qJnvX8jOdwv3h7O1wBYIqaD' +
    '5lCtYoAAAAASUVORK5CYII=';


/**
 * This color needs to match BORDER_IMAGE.
 * @const
 */
ink.CanvasManager.OUT_OF_BOUNDS_COLOR = 0xe6e6e6ff;


/** @override */
ink.CanvasManager.prototype.enterDocument = function() {
  ink.CanvasManager.base(this, 'enterDocument');

  this.engine_.render(this.getElement());

  var handler = this.getHandler();
  goog.asserts.assert(handler);

  this.brushModel_ = ink.BrushModel.getInstance(this);

  handler.listen(
      this.brushModel_, ink.BrushModel.EventType.CHANGE, this.brushUpdate_);
};

/**
 * Sets or unsets readOnly on the engine.
 * @param {boolean} readOnly
 */
ink.CanvasManager.prototype.setReadOnly = function(readOnly) {
  this.engine_.setReadOnly(readOnly);
};


/**
 * Export the scene as a PNG from the engine.
 * @param {number} maxWidth
 * @param {boolean} drawBackground
 * @param {function(!goog.html.SafeUrl)} callback
 * @param {boolean=} opt_asBlob
 */
ink.CanvasManager.prototype.exportPng = function(
    maxWidth, drawBackground, callback, opt_asBlob) {
  this.onPngExportCompleteCallback_ = callback;
  this.exportAsBlob_ = !!opt_asBlob;
  var exportProto = new sketchology.proto.ImageExport();
  exportProto.setMaxDimensionPx(maxWidth);
  exportProto.setShouldDrawBackground(drawBackground);
  this.engine_.exportPng(exportProto);
};


/**
 * @private
 * @param {number} width
 * @param {number} height
 * @param {Uint8ClampedArray} bytesArr
 */
ink.CanvasManager.prototype.onPngExportComplete_ = function(
    width, height, bytesArr) {
  if (this.onPngExportCompleteCallback_) {
    try {
      var imageData = new ImageData(bytesArr, width, height);
      var scratchCanvas =
          /** @type {!HTMLCanvasElement} */ (document.createElement('canvas'));
      var scratchContext =
          /** @type {!CanvasRenderingContext2D} */ (
              scratchCanvas.getContext('2d'));
      scratchCanvas.width = width;
      scratchCanvas.height = height;
      scratchContext.putImageData(imageData, 0, 0);
    } catch (ex) {
      this.dispatchFatalError_(ex);
      return;
    }
    if (this.exportAsBlob_) {
      var cb = (blob) => {
        this.onPngExportCompleteCallback_(goog.html.SafeUrl.fromBlob(blob));
      };

      if (scratchCanvas['msToBlob']) {
        scratchCanvas['msToBlob'](cb, 'image/png');
      } else {
        scratchCanvas.toBlob(cb, 'image/png');
      }
    } else {
      this.onPngExportCompleteCallback_(
          goog.html.SafeUrl.fromDataUrl(scratchCanvas.toDataURL()));
    }
  }
};


/**
 * Sets a background image, and sets the page bounds to match the image size.
 * @param {Uint8ClampedArray} data The image data in RGBA 8888.
 * @param {goog.math.Size} size The image dimensions.
 */
ink.CanvasManager.prototype.setBackgroundImage = function(data, size) {
  this.setBackgroundImage_(data, size);
};


/**
 * Sets a background image and scales the image to match the existing page
 * bounds.  Will not display the background if no page bounds are set.
 * @param {Uint8ClampedArray} data The image data in RGBA 8888.
 * @param {goog.math.Size} size The image dimensions.
 */
ink.CanvasManager.prototype.setImageToUseForPageBackground = function(
    data, size) {
  this.setBackgroundImage_(data, size, {'bounds': 'none'});
};


/**
 * @private
 * @param {Uint8ClampedArray} data The image data in RGBA 8888.
 * @param {goog.math.Size} size The image dimensions.
 * @param {Object<string, *>=} opt_options
 */
ink.CanvasManager.prototype.setBackgroundImage_ = function(
    data, size, opt_options) {
  opt_options = opt_options || {};

  var nextUri = 'sketchology://background_' + this.bgCount_;
  this.bgCount_++;

  var bgImageProto = new sketchology.proto.BackgroundImageInfo();
  bgImageProto.setUri(nextUri);
  if (opt_options['bounds'] != 'none') {
    var optBounds = opt_options['bounds'] ||
        {'xlow': 0, 'ylow': 0, 'xhigh': size.width, 'yhigh': size.height};
    var bounds = new sketchology.proto.Rect();
    bounds.setXlow(optBounds['xlow']);
    bounds.setYlow(optBounds['ylow']);
    bounds.setXhigh(optBounds['xhigh']);
    bounds.setYhigh(optBounds['yhigh']);
    bgImageProto.setBounds(bounds);
  }
  this.engine_.setBackgroundImage(data, size, nextUri, bgImageProto);
};


/**
 * Set background color.
 * @param {ink.Color} color
 */
ink.CanvasManager.prototype.setBackgroundColor = function(color) {
  this.engine_.setBackgroundColor(color);
};


/** @private */
ink.CanvasManager.prototype.brushUpdate_ = function() {
  goog.asserts.assert(this.brushModel_);

  var tool_type = this.brushModel_.getToolType();
  var brush_type = this.brushModel_.getBrushType();
  var strokeWidth = this.brushModel_.getStrokeWidth();

  var colorString = this.brushModel_.getColor().substring(1);
  var color = new ink.Color(parseInt(colorString, 16));
  // Using this alpha channel would make calligraphy or marker brushes
  // translucent; highlighter, watercolor, and airbrush have hard-coded alpha
  // values in the engine.
  color.a = 0xFF;  // Set alpha to opaque

  this.engine_.brushUpdate(
      color.getRgbaUint32(), strokeWidth, tool_type, brush_type);
};


/**
 * Handles a new element created in the engine.
 * @param {string} uuid
 * @param {string} encodedElement
 * @param {string} encodedTransform
 * @override
 */
ink.CanvasManager.prototype.onElementCreated = function(
    uuid, encodedElement, encodedTransform) {
  this.pendingUUIDs_.add(uuid);
  this.dispatchEvent(new ink.embed.events.ElementCreatedEvent(
      uuid, encodedElement, encodedTransform));
};


/**
 * Handles an element being transformed.
 * @param {Array.<string>} uuids
 * @param {Array.<string>} encodedTransforms
 * @override
 */
ink.CanvasManager.prototype.onElementsMutated = function(
    uuids, encodedTransforms) {
  this.dispatchEvent(
      new ink.embed.events.ElementsMutatedEvent(uuids, encodedTransforms));
};


/**
 * Handles elements being removed.
 * @param {Array.<string>} uuids
 * @override
 */
ink.CanvasManager.prototype.onElementsRemoved = function(uuids) {
  this.dispatchEvent(new ink.embed.events.ElementsRemovedEvent(uuids));
};


/**
 * Handle an addElement request from Brix.  If this is a remote add or an
 * add-by-undo or redo, adds the element to the engine and the local list of
 * elements (this.UUIDs_) at the specified index.  If this is a local add
 * originated by the engine, the element is already present in the engine and is
 * simply removed from this.pendingUUIDs_.
 *
 * @param {!Object<string, string>} bundle
 * @param {number} idx index to add the element at
 * @param {boolean} isLocal
 */
ink.CanvasManager.prototype.addElement = function(bundle, idx, isLocal) {
  if (!bundle['id']) {
    bundle['id'] = 'local-' + this.nextLocalId_++;
  }

  var uuid = bundle['id'];
  goog.array.insertAt(this.UUIDs_, uuid, idx);

  // If the element originated in the engine, it should be present in the
  // pending UUID set, so we just remove it from that set so that future adds by
  // undo/redo will work as expected.
  if (this.pendingUUIDs_.contains(uuid)) {
    this.pendingUUIDs_.remove(uuid);
  } else {
    // If the element is a remote add or an add by undo or redo, it may not be
    // the top element, in which case we add it below the element after it.
    if (idx < this.UUIDs_.length - 1) {
      this.engine_.addElementBelow(bundle, this.UUIDs_[idx + 1]);
    } else {
      this.engine_.addElement(bundle);
    }
  }

  // TODO(wfurr): Figure out how to have the engine wake itself up.
  // See b/18830720.
  this.engine_.poke();
};


/**
 * Removes a number of elements.
 * @param {number} idx index to start removing.
 * @param {number} count number of items to remove.
 */
ink.CanvasManager.prototype.removeElements = function(idx, count) {
  for (var i = 0; i < count; i++) {
    var uuid = this.UUIDs_[idx];
    this.engine_.removeElement(uuid);
    goog.array.removeAt(this.UUIDs_, idx);
  }

  // TODO(wfurr): Figure out how to have the engine wake itself up.
  // See b/18830720.
  this.engine_.poke();
};


/**
 * Resets the engine but does not dispatch any Brix related events. Used to
 * clear the canvas to reuse it to display another drawing.
 */
ink.CanvasManager.prototype.resetCanvas = function() {
  this.engine_.clear();
  this.engine_.setBackgroundColor(ink.Color.DEFAULT_BACKGROUND_COLOR);
  this.pendingUUIDs_.clear();
  this.UUIDs_ = [];
  this.engine_.poke();
};


/**
 * Clears the canvas.
 */
ink.CanvasManager.prototype.clear = function() {
  this.engine_.removeAll();
};


/**
 * Sets element transforms.
 * @param {Array.<string>} uuids
 * @param {Array.<string>} encodedTransforms
 */
ink.CanvasManager.prototype.setElementTransforms = function(
    uuids, encodedTransforms) {
  this.engine_.setElementTransforms(uuids, encodedTransforms);
};


/**
 * Set callback flags
 * @param {!sketchology.proto.SetCallbackFlags} setCallbackFlags
 */
ink.CanvasManager.prototype.setCallbackFlags = function(setCallbackFlags) {
  this.engine_.setCallbackFlags(setCallbackFlags);
};


/**
 * Sets the size of the page.
 * @param {number} left
 * @param {number} top
 * @param {number} right
 * @param {number} bottom
 */
ink.CanvasManager.prototype.setPageBounds = function(left, top, right, bottom) {
  this.engine_.setPageBounds(left, top, right, bottom);
};


/**
 * Deselects anything selected with the edit tool.
 */
ink.CanvasManager.prototype.deselectAll = function() {
  this.engine_.deselectAll();
};


/**
 * Sets the border image.
 * @private
 */
ink.CanvasManager.prototype.setBorderImage_ = function() {
  var self = this;
  var uri = 'sketchology://border0';
  var borderImageProto = new sketchology.proto.Border();
  borderImageProto.setUri(uri);
  borderImageProto.setScale(1);

  ink.util.getImageBytes(ink.CanvasManager.BORDER_IMAGE, function(data, size) {
    self.engine_.setBorderImage(
        data, size, uri, borderImageProto,
        ink.CanvasManager.OUT_OF_BOUNDS_COLOR);
  });
};


/**
 * Dispatches a FATAL_ERROR event, and throws an Error if it isn't handled.
 * @param {Error=} opt_cause
 * @private
 */
ink.CanvasManager.prototype.dispatchFatalError_ = function(opt_cause) {
  if (this.dispatchEvent(new ink.embed.events.FatalErrorEvent(opt_cause))) {
    // Unless one of the listeners returns false or preventDefaults, throw an
    // error to trigger default exception handlers on the page.
    throw opt_cause || new Error('Unhandled fatal ink error');
  }
};


/**
 * Enable or disable an engine flag.
 * @param {sketchology.proto.Flag} which
 * @param {boolean} enable
 */
ink.CanvasManager.prototype.assignFlag = function(which, enable) {
  this.engine_.assignFlag(which, enable);
};


/**
 * Simple undo. This only works if the SEngine was constructed with a
 *   SingleUserDocument with InMemoryStorage.
 */
ink.CanvasManager.prototype.undo = function() {
  this.engine_.undo();
};


/**
 * Simple redo. This only works if the SEngine was constructed with a
 *   SingleUserDocument with InMemoryStorage.
 */
ink.CanvasManager.prototype.redo = function() {
  this.engine_.redo();
};


/**
 * Returns the current snapshot.
 * @param {function(!sketchology.proto.Snapshot)} callback
 */
ink.CanvasManager.prototype.getSnapshot = function(callback) {
  this.engine_.getSnapshot(callback);
};


/**
 * Loads a document from a snapshot.
 *
 * @param {!sketchology.proto.Snapshot} snapshotProto
 */
ink.CanvasManager.prototype.loadFromSnapshot = function(snapshotProto) {
  this.engine_.loadFromSnapshot(snapshotProto);
};


/**
 * Allows the user to execute arbitrary commands on the engine.
 * @param {!sketchology.proto.Command} command
 */
ink.CanvasManager.prototype.handleCommand = function(command) {
  this.engine_.handleCommand(command);
};


/**
 * Gets the raw engine object. Do not use this.
 * @return {Object}
 */
ink.CanvasManager.prototype.getRawEngineObject = function() {
  return this.engine_.getRawEngineObject();
};


/**
 * Generates a snapshot based on a brix document.
 * @param {!ink.util.RealtimeDocument} brixDoc
 * @param {function(!sketchology.proto.Snapshot)} callback
 */
ink.CanvasManager.prototype.convertBrixDocumentToSnapshot =
    function(brixDoc, callback) {
  this.engine_.convertBrixDocumentToSnapshot(brixDoc, callback);
};


/**
 * @param {!sketchology.proto.Snapshot} snapshot
 * @param {function(boolean)} callback
 */
ink.CanvasManager.prototype.snapshotHasPendingMutations =
    function(snapshot, callback) {
  this.engine_.snapshotHasPendingMutations(snapshot, callback);
};


/**
 * @param {!sketchology.proto.Snapshot} snapshot
 * @param {function(sketchology.proto.MutationPacket)} callback
 */
ink.CanvasManager.prototype.extractMutationPacket =
    function(snapshot, callback) {
  this.engine_.extractMutationPacket(snapshot, callback);
};


/**
 * @param {!sketchology.proto.Snapshot} snapshot
 * @param {function(sketchology.proto.Snapshot)} callback
 */
ink.CanvasManager.prototype.clearPendingMutations =
    function(snapshot, callback) {
  this.engine_.clearPendingMutations(snapshot, callback);
};


/**
 * Calls the given callback once all previous asynchronous engine operations
 * have been applied.
 * @param {!Function} callback
 */
ink.CanvasManager.prototype.flush = function(callback) {
  this.engine_.flush(callback);
};
