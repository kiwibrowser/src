// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
/**
 * @fileoverview Wrapper to call the Sketchology engine.
 */

goog.provide('ink.SketchologyEngineWrapper');

goog.require('goog.asserts');
goog.require('goog.events');
goog.require('goog.labs.userAgent.platform');
goog.require('goog.math.Box');
goog.require('goog.math.Coordinate');
goog.require('goog.math.Rect');
goog.require('goog.math.Size');
goog.require('goog.soy');
goog.require('goog.ui.Component');
goog.require('ink.Color');
goog.require('ink.ElementListener');
goog.require('ink.UndoStateChangeEvent');
goog.require('ink.soy.nacl.canvasHTML');
goog.require('ink.util');
goog.require('net.proto2.contrib.WireSerializer');
goog.require('sketchology.proto.BackgroundColor');
goog.require('sketchology.proto.BackgroundImageInfo');
goog.require('sketchology.proto.Command');
goog.require('sketchology.proto.Flag');
goog.require('sketchology.proto.FlagAssignment');
goog.require('sketchology.proto.ImageInfo');
goog.require('sketchology.proto.MutationPacket');
goog.require('sketchology.proto.NoArgCommand');
goog.require('sketchology.proto.OutOfBoundsColor');
goog.require('sketchology.proto.PageProperties');
goog.require('sketchology.proto.Rect');
goog.require('sketchology.proto.SequencePoint');
goog.require('sketchology.proto.SetCallbackFlags');
goog.require('sketchology.proto.Snapshot');
goog.require('sketchology.proto.ToolParams');



/**
 * @param {?string} engineUrl URL to the native client manifest file
 * @param {ink.ElementListener} elementListener
 * @param {function(number, number, Uint8ClampedArray)} onImageExportComplete
 * @param {!ink.util.SEngineType} sengineType
 * @struct
 * @constructor
 * @extends {goog.ui.Component}
 */
ink.SketchologyEngineWrapper = function(
    engineUrl, elementListener, onImageExportComplete, sengineType) {
  ink.SketchologyEngineWrapper.base(this, 'constructor');

  goog.asserts.assert(engineUrl);
  /** @private {string} */
  this.engineUrl_ = engineUrl;

  /** @private {ink.ElementListener} */
  this.elementListener_ = elementListener;

  /** @private {function(number, number, Uint8ClampedArray)} */
  this.onImageExportComplete_ = onImageExportComplete;

  // default document bounds
  this.pageLeft_ = 0;
  this.pageTop_ = 600;
  this.pageRight_ = 800;
  this.pageBottom_ = 0;

  /**
   * Protocol Buffer wire format serializer.
   * @type {!net.proto2.contrib.WireSerializer}
   * @private
   */
  this.wireSerializer_ = new net.proto2.contrib.WireSerializer();

  /** @private */
  this.lastBrushUpdate_ = goog.nullFunction;

  /** @private */
  this.penModeEnabled_ = false;

  /** @private {boolean} */
  this.listenersAdded_ = false;

  /** @private {string} */
  this.sengineType_ = /** @type {string} */ (sengineType);

  /** @private {Array.<function(!sketchology.proto.Snapshot)>} */
  this.snapshotCallbacks_ = [];

  /** @private {Array.<function(!sketchology.proto.Snapshot)>} */
  this.brixConversionCallbacks_ = [];

  /** @private {Array.<!ink.util.RealtimeDocument>} */
  this.brixDocuments_ = [];

  /** @private {Array.<function(boolean)>} */
  this.snapshotHasPendingMutationsCallbacks_ = [];

  /** @private {Array.<function(sketchology.proto.MutationPacket)>} */
  this.extractMutationPacketCallbacks_ = [];

  /** @private {Array.<function(sketchology.proto.Snapshot)>} */
  this.clearPendingMutationsCallbacks_ = [];

  /** @private {Element} */
  this.engineElement_;

  /** @private {Object<number, !Function>} */
  this.sequencePointCallbacks_ = {};

  /** @private {number} */
  this.nextSequencePointId_ = 0;
};
goog.inherits(ink.SketchologyEngineWrapper, goog.ui.Component);

/** @override */
ink.SketchologyEngineWrapper.prototype.createDom = function() {
  const useMSAA = !goog.labs.userAgent.platform.isMacintosh() ||
      // MSAA is disabled for MacOS 10.12.4 and prior: b/38280481
      goog.labs.userAgent.platform.isVersionOrHigher('10.12.5');
  const useSingleBuffer = goog.labs.userAgent.platform.isChromeOS();
  const elem = goog.soy.renderAsElement(ink.soy.nacl.canvasHTML, {
    manifestUrl: this.engineUrl_,
    useMSAA: !!useMSAA + '',
    useSingleBuffer: !!useSingleBuffer + '',
    sengineType: this.sengineType_
  });
  this.setElementInternal(elem);
  this.engineElement_ = elem.querySelector('#ink-engine');
};

/** @override */
ink.SketchologyEngineWrapper.prototype.enterDocument = function() {
  this.engineElement_.addEventListener(goog.events.EventType.LOAD, () => {
    this.initGl();
    this.lastBrushUpdate_();
    this.assignFlag(
        sketchology.proto.Flag.ENABLE_PEN_MODE, this.penModeEnabled_);
  });
};


/** @enum {string} */
ink.SketchologyEngineWrapper.EventType = {
  CANVAS_INITIALIZED: goog.events.getUniqueId('gl_canvas_initialized'),
  CANVAS_FATAL_ERROR: goog.events.getUniqueId('fatal_error'),
  PEN_MODE_ENABLED: goog.events.getUniqueId('pen_mode_enabled')
};


/**
 * An event fired when pen mode is enabled or disabled.
 *
 * @param {boolean} enabled
 *
 * @extends {goog.events.Event}
 * @constructor
 * @struct
 */
ink.SketchologyEngineWrapper.PenModeEnabled = function(enabled) {
  ink.SketchologyEngineWrapper.PenModeEnabled.base(this, 'constructor',
      ink.SketchologyEngineWrapper.EventType.PEN_MODE_ENABLED);

  /** @type {boolean} */
  this.enabled = enabled;
};
goog.inherits(ink.SketchologyEngineWrapper.PenModeEnabled, goog.events.Event);


/** Poke the engine to wake up and start drawing */
ink.SketchologyEngineWrapper.prototype.poke = function() {
  this.engineElement_.postMessage(['poke', '']);
};

/**
 * Global exit function for emscripten to call.
 * @export
 */
ink.SketchologyEngineWrapper.exit = function() {
  console.log('Engine requested exit.');
};

/**
 * @param {sketchology.proto.Command} command
 */
ink.SketchologyEngineWrapper.prototype.handleCommand = function(command) {
  var commandBytes = this.wireSerializer_.serialize(command);
  var buf = new Uint8Array(commandBytes);
  this.engineElement_.postMessage(['handleCommand', buf.buffer]);
};

/**
 * Tells the engine to handle a message received remotely.
 *
 * @param {!Object<string, string>} bundle
 */
ink.SketchologyEngineWrapper.prototype.addElement = function(bundle) {
  goog.asserts.assert(bundle);
  this.engineElement_.postMessage(['addElementToEngine', {'bundle': bundle}]);
};


/**
 * Add an encoded element bundle to the engine.
 *
 * @param {!Object<string, string>} bundle
 * @param {string} belowUUID
 */
ink.SketchologyEngineWrapper.prototype.addElementBelow = function(
    bundle, belowUUID) {
  goog.asserts.assert(bundle);
  this.engineElement_.postMessage(
      ['addElementToEngineBelow', {'bundle': bundle, 'below_uuid': belowUUID}]);
};


/**
 * @param {string} uuid
 */
ink.SketchologyEngineWrapper.prototype.removeElement = function(uuid) {
  this.engineElement_.postMessage(['removeElement', uuid]);
};


/**
 * NaCl does its own GL initialization, so we just hook up listeners.
 */
ink.SketchologyEngineWrapper.prototype.initGl = function() {
  var elem = this.engineElement_;
  this.setPageBounds(
      this.pageLeft_, this.pageTop_, this.pageRight_, this.pageBottom_);
  if (!this.listenersAdded_) {
    this.listenersAdded_ = true;
    elem.addEventListener('message', goog.bind(function(msg) {
      if (!('event_type' in msg['data'])) {
        return;  // Unknown event type!
      }
      var data = msg['data'];
      switch (data['event_type']) {
        case 'exit':
          ink.SketchologyEngineWrapper.exit();
          break;
        case 'debug':
          if (goog.DEBUG) {
            console.log(data['message']);
          }
          break;
        case 'image_export':
          this.onImageExportComplete_(
              data['width'], data['height'],
              new Uint8ClampedArray(data['bytes']));
          break;
        case 'element_added':
          if (this.elementListener_) {
            this.elementListener_.onElementCreated(data['uuid'],
                data['encoded_element'], data['encoded_transform']);
          }
          break;
        case 'elements_mutated':
          if (this.elementListener_) {
            this.elementListener_.onElementsMutated(data['uuids'],
                data['encoded_transforms']);
          }
          break;
        case 'elements_removed':
          if (this.elementListener_) {
            this.elementListener_.onElementsRemoved(data['uuids']);
          }
          break;
        case 'flag_changed':
          if (data['which'] == sketchology.proto.Flag.ENABLE_PEN_MODE) {
            this.penModeEnabled_ = data['enabled'];
            this.dispatchEvent(
                new ink.SketchologyEngineWrapper.PenModeEnabled(
                    data['enabled']));
          }
          break;
        case 'undo_redo_state_changed':
          this.dispatchEvent(new ink.UndoStateChangeEvent(
              !!data['can_undo'], !!data['can_redo']));
          break;
        case 'snapshot_gotten':
          var proto = new sketchology.proto.Snapshot();
          this.wireSerializer_.deserializeTo(proto, data['snapshot']);
          this.snapshotCallbacks_.shift().call(null, proto);
          break;
        case 'brix_elements_converted':
          var proto = new sketchology.proto.Snapshot();
          this.wireSerializer_.deserializeTo(proto, data['snapshot']);
          var pageProperties = new sketchology.proto.PageProperties();
          var rect = new sketchology.proto.Rect();
          var brixDoc = this.brixDocuments_.shift();
          var model = brixDoc.getModel();
          var root = model.getRoot();
          var brixBounds = root.get('bounds');
          rect.setXhigh(brixBounds.xhigh || 0);
          rect.setXlow(brixBounds.xlow || 0);
          rect.setYhigh(brixBounds.yhigh || 0);
          rect.setYlow(brixBounds.ylow || 0);
          pageProperties.setBounds(rect);
          proto.setPageProperties(pageProperties);
          this.brixConversionCallbacks_.shift().call(null, proto);
          break;
        case 'snapshot_has_pending_mutations':
          this.snapshotHasPendingMutationsCallbacks_.shift().call(
              null, data['has_mutations']);
          break;
        case 'extracted_mutation_packet':
          var proto = new sketchology.proto.MutationPacket();
          this.wireSerializer_.deserializeTo(proto, data['extraction_packet']);
          this.extractMutationPacketCallbacks_.shift().call(null, proto);
          break;
        case 'cleared_pending_mutations':
          var proto = new sketchology.proto.Snapshot();
          this.wireSerializer_.deserializeTo(proto, data['snapshot']);
          this.clearPendingMutationsCallbacks_.shift().call(null, proto);
          break;
        case 'hwoverlay':
          this.setHardwareOverlay(!!data['enable']);
          break;
        case 'single_buffer':
          // If the Native Client module was able to obtain a single buffered
          // graphics context, flip the embed element to allow promotion to
          // hardware overlay on Eve in landscape mode.
          // TODO(b/64569245): Add support for all device orientations
          this.engineElement_.style.transform = 'scaleY(-1)';
          break;
        case 'sequence_point_reached':
          var id = data['id'];
          var cb = this.sequencePointCallbacks_[id];
          delete this.sequencePointCallbacks_[id];
          cb();
          break;
      }
    }, this));
  }
  this.dispatchEvent(ink.SketchologyEngineWrapper.EventType.CANVAS_INITIALIZED);
};


/**
 * Sets the border image.
 * @param {Uint8ClampedArray} data
 * @param {goog.math.Size} size
 * @param {string} uri
 * @param {!sketchology.proto.Border} borderImageProto
 * @param {number} outOfBoundsColor The out of bounds color in rgba 8888.
 */
ink.SketchologyEngineWrapper.prototype.setBorderImage = function(
    data, size, uri, borderImageProto, outOfBoundsColor) {
  var outOfBoundsColorProto = new sketchology.proto.OutOfBoundsColor();
  outOfBoundsColorProto.setRgba(outOfBoundsColor);
  var commandProto = new sketchology.proto.Command();
  commandProto.setSetOutOfBoundsColor(outOfBoundsColorProto);
  this.handleCommand(commandProto);

  var msg = {
    'imageData': data.buffer,
    'uri': uri,
    'width': size.width,
    'height': size.height,
    'assetType': sketchology.proto.ImageInfo.AssetType.BORDER
  };
  this.engineElement_.postMessage(['addImageData', msg]);

  commandProto = new sketchology.proto.Command();
  commandProto.setSetPageBorder(borderImageProto);
  this.handleCommand(commandProto);
};


/**
 * Sets the background image from a data URI.
 *
 * @param {Uint8ClampedArray} data
 * @param {goog.math.Size} size
 * @param {string} uri
 * @param {!sketchology.proto.BackgroundImageInfo} bgImageProto
 */
ink.SketchologyEngineWrapper.prototype.setBackgroundImage = function(
    data, size, uri, bgImageProto) {
  var msg = {
    'imageData': data.buffer,
    'uri': uri,
    'width': size.width,
    'height': size.height,
    'assetType': sketchology.proto.ImageInfo.AssetType.DEFAULT
  };
  this.engineElement_.postMessage(['addImageData', msg]);

  var commandProto = new sketchology.proto.Command();
  commandProto.setBackgroundImage(bgImageProto);
  this.handleCommand(commandProto);
};


/**
 * Set the background color
 * @param {ink.Color} color
 */
ink.SketchologyEngineWrapper.prototype.setBackgroundColor = function(color) {
  var bgColorProto = new sketchology.proto.BackgroundColor();
  bgColorProto.setRgba(color.getRgbaUint32()[0]);
  var commandProto = new sketchology.proto.Command();
  commandProto.setBackgroundColor(bgColorProto);
  this.handleCommand(commandProto);
};


/**
 * Sets the camera position.
 *
 * @param {!goog.math.Rect} cameraRect The camera rect.
 */
ink.SketchologyEngineWrapper.prototype.setCamera = function(cameraRect) {
  var camera = cameraRect.toBox();
  var rectProto = new sketchology.proto.Rect();
  // Top and bottom are reversed in Sketchology for "reasons."
  rectProto.setYhigh(camera.bottom);
  rectProto.setXhigh(camera.right);
  rectProto.setYlow(camera.top);
  rectProto.setXlow(camera.left);
  var commandProto = new sketchology.proto.Command();
  commandProto.setCameraPosition(rectProto);
  this.handleCommand(commandProto);
};


/**
 * @private
 * @param {sketchology.proto.Rect} rectProto rect js proto with top/bottom
 * reversed
 * @return {goog.math.Rect} proper CSS rect with top/bottom correct
 */
ink.SketchologyEngineWrapper.prototype.convertRect_ = function(rectProto) {
  // Top and bottom are reversed in Sketchology for "reasons."
  var box = new goog.math.Box(
      rectProto.getYlowOrDefault(), rectProto.getXhighOrDefault(),
      rectProto.getYhighOrDefault(), rectProto.getXlowOrDefault());
  return box ?
      goog.math.Rect.createFromBox(box) :
      null;
};


/**
 * Create a scaled rectangle with a given size/center scaled by factor.
 *
 * @param {!goog.math.Coordinate} center
 * @param {!goog.math.Size} size
 * @param {number} factor The scale factor
 *
 * @return {goog.math.Rect} The scaled rectangle.
 * @private
 */
ink.SketchologyEngineWrapper.prototype.getScaledRect_ = function(
    center, size, factor) {
  size.width /= factor;
  size.height /= factor;

  var x = center.x - size.width / 2;
  var y = center.y - size.height / 2;

  var cameraRect = new goog.math.Rect(x, y, size.width, size.height);

  goog.asserts.assert(
      Math.round(cameraRect.getCenter().x) === Math.round(center.x) &&
      Math.round(cameraRect.getCenter().y) === Math.round(center.y));

  return cameraRect;
};


/**
 * Sets the brush parameters.
 *
 * @param {Uint32Array} color rgba 32-bit unsigned color
 * @param {number} strokeWidth brush size percent [0,1]
 * @param {sketchology.proto.ToolParams.ToolType} toolType
 * @param {sketchology.proto.BrushType} brushType
 */
ink.SketchologyEngineWrapper.prototype.brushUpdate =
    function(color, strokeWidth, toolType, brushType) {
  var self = this;
  this.lastBrushUpdate_ = function() {
    // LINE tools need special handling
    if (toolType != sketchology.proto.ToolParams.ToolType.LINE) {
      var toolParamsProto = new sketchology.proto.ToolParams();
      toolParamsProto.setTool(toolType);
      var commandProto = new sketchology.proto.Command();
      commandProto.setToolParams(toolParamsProto);
      this.handleCommand(commandProto);
    } else {
      var updateBrushData = {
        'brush': brushType,
        'rgba': color[0],
        'stroke_width': strokeWidth
      };
      self.engineElement_.postMessage(['updateBrush', updateBrushData]);
    }
  };
  this.lastBrushUpdate_();
};


/** Clears the canvas. */
ink.SketchologyEngineWrapper.prototype.clear = function() {
  this.engineElement_.postMessage(['clear', '']);
};


/** Removes all elements from the document. */
ink.SketchologyEngineWrapper.prototype.removeAll = function() {
  this.engineElement_.postMessage(['removeAll', '']);
};


/**
 * Sets or unsets readOnly on the canvas.
 * @param {boolean} readOnly
 */
ink.SketchologyEngineWrapper.prototype.setReadOnly = function(readOnly) {
  this.assignFlag(sketchology.proto.Flag.READ_ONLY_MODE, !!readOnly);
};


/**
 * Assign a flag on the canvas
 * @param {sketchology.proto.Flag} flag
 * @param {boolean} enable
 */
ink.SketchologyEngineWrapper.prototype.assignFlag = function(flag, enable) {
  var flagProto = new sketchology.proto.FlagAssignment();
  flagProto.setFlag(flag);
  flagProto.setBoolValue(!!enable);
  var commandProto = new sketchology.proto.Command();
  commandProto.setFlagAssignment(flagProto);
  this.handleCommand(commandProto);
};




/**
 * Sets element transforms.
 * @param {Array.<string>} uuids
 * @param {Array.<string>} encodedTransforms
 */
ink.SketchologyEngineWrapper.prototype.setElementTransforms = function(
    uuids, encodedTransforms) {
  if (uuids.length !== encodedTransforms.length) {
    throw new Error('mismatch in transform array lengths');
  }
  this.engineElement_.postMessage([
    'setElementTransforms',
    {'uuids': uuids, 'encoded_transforms': encodedTransforms}
  ]);
};

/**
 * Set callback flags for what data is attached to element callbacks.
 * @param {!sketchology.proto.SetCallbackFlags} setCallbackFlags
 */
ink.SketchologyEngineWrapper.prototype.setCallbackFlags = function(
    setCallbackFlags) {
  var commandProto = new sketchology.proto.Command();
  commandProto.setSetCallbackFlags(setCallbackFlags);
  this.handleCommand(commandProto);
};


/**
 * Sets the size of the page.
 * @param {number} left
 * @param {number} top
 * @param {number} right
 * @param {number} bottom
 */
ink.SketchologyEngineWrapper.prototype.setPageBounds =
    function(left, top, right, bottom) {
  this.pageLeft_ = left;
  this.pageTop_ = top;
  this.pageRight_ = right;
  this.pageBottom_ = bottom;
  var pageBounds = new sketchology.proto.Rect();
  pageBounds.setXlow(this.pageLeft_);
  pageBounds.setYlow(this.pageBottom_);
  pageBounds.setXhigh(this.pageRight_);
  pageBounds.setYhigh(this.pageTop_);
  var commandProto = new sketchology.proto.Command();
  commandProto.setPageBounds(pageBounds);
  this.handleCommand(commandProto);
};


/**
 * Deselects anything selected with the edit tool.
 */
ink.SketchologyEngineWrapper.prototype.deselectAll = function() {
  throw new Error('deselectAll not yet implemented for NaCl.');
};


/**
 * Start the PNG export process.
 *
 * @param {!sketchology.proto.ImageExport} exportProto
 */
ink.SketchologyEngineWrapper.prototype.exportPng = function(exportProto) {
  var commandProto = new sketchology.proto.Command();
  commandProto.setImageExport(exportProto);
  this.handleCommand(commandProto);
};


/**
 * Simple undo.
 */
ink.SketchologyEngineWrapper.prototype.undo = function() {
  var commandProto = new sketchology.proto.Command();
  commandProto.setUndo(new sketchology.proto.NoArgCommand());
  this.handleCommand(commandProto);
};


/**
 * Simple redo.
 */
ink.SketchologyEngineWrapper.prototype.redo = function() {
  var commandProto = new sketchology.proto.Command();
  commandProto.setRedo(new sketchology.proto.NoArgCommand());
  this.handleCommand(commandProto);
};


/**
 * Returns the current snapshot.
 * @param {function(!sketchology.proto.Snapshot)} callback
 */
ink.SketchologyEngineWrapper.prototype.getSnapshot = function(callback) {
  this.snapshotCallbacks_.push(callback);
  this.engineElement_.postMessage(['getSnapshot']);
};


/**
 * Loads a document from a snapshot.
 *
 * @param {!sketchology.proto.Snapshot} snapshotProto
 */
ink.SketchologyEngineWrapper.prototype.loadFromSnapshot =
    function(snapshotProto) {
  var bytes = this.wireSerializer_.serialize(snapshotProto);
  var buf = new Uint8Array(bytes);
  this.engineElement_.postMessage(['loadFromSnapshot', buf.buffer]);
};


/**
 * Gets the raw engine object. Do not use this.
 * @return {Object}
 */
ink.SketchologyEngineWrapper.prototype.getRawEngineObject = function() {
  throw new Error('getRawEngineObject not supported for NaCl.');
};


/**
 * Generates a snapshot based on a brix document.
 * @param {!ink.util.RealtimeDocument} brixDoc
 * @param {function(!sketchology.proto.Snapshot)} callback
 */
ink.SketchologyEngineWrapper.prototype.convertBrixDocumentToSnapshot =
    function(brixDoc, callback) {
  var model = brixDoc.getModel();
  var root = model.getRoot();
  var pages = root.get('pages');
  var page = pages.get(0);
  if (!page) {
    throw Error('unable to get page from brix document.');
  }
  var elements = page.get('elements').asArray();

  var jsonElements = [];
  for (var i = 0; i < elements.length; i++) {
    var element = elements[i];
    jsonElements.push({'id': element.get('id'),
                       'proto': element.get('proto'),
                       'transform': element.get('transform')});
  }
  this.brixDocuments_.push(brixDoc);
  this.brixConversionCallbacks_.push(callback);
  this.engineElement_.postMessage(['convertBrixElements', jsonElements]);
};


/**
 * @param {!sketchology.proto.Snapshot} snapshot
 * @param {function(boolean)} callback
 */
ink.SketchologyEngineWrapper.prototype.snapshotHasPendingMutations =
    function(snapshot, callback) {
  var bytes = this.wireSerializer_.serialize(snapshot);
  var buf = new Uint8Array(bytes);
  this.snapshotHasPendingMutationsCallbacks_.push(callback);
  this.engineElement_.postMessage(['snapshotHasPendingMutations', buf.buffer]);
};


/**
 * @param {!sketchology.proto.Snapshot} snapshot
 * @param {function(sketchology.proto.MutationPacket)} callback
 */
ink.SketchologyEngineWrapper.prototype.extractMutationPacket =
    function(snapshot, callback) {
  var bytes = this.wireSerializer_.serialize(snapshot);
  var buf = new Uint8Array(bytes);
  this.extractMutationPacketCallbacks_.push(callback);
  this.engineElement_.postMessage(['extractMutationPacket', buf.buffer]);
};


/**
 * @param {!sketchology.proto.Snapshot} snapshot
 * @param {function(sketchology.proto.Snapshot)} callback
 */
ink.SketchologyEngineWrapper.prototype.clearPendingMutations = function(
    snapshot, callback) {
  var bytes = this.wireSerializer_.serialize(snapshot);
  var buf = new Uint8Array(bytes);
  this.clearPendingMutationsCallbacks_.push(callback);
  this.engineElement_.postMessage(['clearPendingMutations', buf.buffer]);
};


/**
 * Enable or disable hardware overlay by hiding or showing a 1-pixel div over
 * the canvas.
 *
 * @param {boolean} enable
 */
ink.SketchologyEngineWrapper.prototype.setHardwareOverlay = function(enable) {
  document.querySelector('#ink-engine-hwoverlay').style.display =
      enable ? 'none' : 'block';
};


/**
 * Calls the given callback once all previous asynchronous engine operations
 * have been applied.
 * @param {!Function} callback
 */
ink.SketchologyEngineWrapper.prototype.flush = function(callback) {
  var commandProto = new sketchology.proto.Command();
  var sp = new sketchology.proto.SequencePoint();
  sp.setId(this.nextSequencePointId_);
  this.sequencePointCallbacks_[this.nextSequencePointId_++] = callback;
  commandProto.setSequencePoint(sp);
  this.handleCommand(commandProto);
};
