// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
/**
 * @fileoverview The embeddable ink component. Generally should be constructed
 * by using {@code ink.embed.Config.execute()}.
 */
goog.provide('ink.embed.EmbedComponent');

goog.require('goog.dom');
goog.require('goog.events');
goog.require('goog.events.Event');
goog.require('goog.math.Size');
goog.require('goog.soy');
goog.require('goog.ui.Component');
goog.require('ink.CanvasManager');
goog.require('ink.Color');
goog.require('ink.CursorUpdater');
goog.require('ink.embed.Config');
goog.require('ink.embed.events');
goog.require('ink.soy.embedContent');
goog.require('ink.util');
goog.require('protos.research.ink.InkEvent');
goog.require('sketchology.proto.SetCallbackFlags');



/**
 * @param {!ink.embed.Config} config
 * @param {!Function} callback
 * @constructor
 * @extends {goog.ui.Component}
 * @struct
 */
ink.embed.EmbedComponent = function(config, callback) {
  ink.embed.EmbedComponent.base(this, 'constructor');

  /** @private {!ink.embed.Config} */
  this.config_ = config;

  /** @public {boolean} */
  this.allowDialogs = config.allowDialogs;

  /** @private {!ink.CursorUpdater} */
  this.cursorUpdater_ = new ink.CursorUpdater();
  this.addChild(this.cursorUpdater_);

  /** @private {!ink.CanvasManager} */
  this.canvasManager_ =
      new ink.CanvasManager(config.nativeClientManifestUrl, config.sengineType);
  this.addChild(this.canvasManager_);

  /** @private {!Function} */
  this.callback_ = callback;
};
goog.inherits(ink.embed.EmbedComponent, goog.ui.Component);


////////////////////////////////////////////////////////////////
// Public API for embedders.
////////////////////////////////////////////////////////////////


/**
 * @param {!ink.embed.Config} config
 * @param {function(ink.embed.EmbedComponent)} callback Callback function that
 * returns the component that is configured based on the settings in this
 * Config. This component will raise the relevant ink.embed.Events and provides
 * an interface to change the brush color, size, etc. Null is returned if the
 * config is invalid.
 */
ink.embed.EmbedComponent.execute = function(config, callback) {
  var embed = new ink.embed.EmbedComponent(config, callback);
  embed.setParent(config.parentComponent);
  embed.render(config.parentEl);
};


/** Removes all elements from the drawing space. */
ink.embed.EmbedComponent.prototype.clear = function() {
  var e = new goog.events.Event(ink.embed.events.EventType.CLEAR_REQUESTED);
  this.dispatchEvent(e);
  if (!e.defaultPrevented) {
    this.canvasManager_.clear();
  }
};


/** Undoes the last modification to the document taken by the user. */
ink.embed.EmbedComponent.prototype.undo = function() {
  var e = new goog.events.Event(ink.embed.events.EventType.UNDO_REQUESTED);
  this.dispatchEvent(e);
  if (!e.defaultPrevented) {
    this.canvasManager_.undo();
  }
  var eventProto = ink.util.createDocumentEvent(
      this.getLogsHost(),
      protos.research.ink.InkEvent.DocumentEvent.DocumentEventType.UNDO);
  var logEvent = new ink.embed.events.LogEvent(eventProto);
  this.dispatchEvent(logEvent);
};


/** Redoes the last undone action. */
ink.embed.EmbedComponent.prototype.redo = function() {
  var e = new goog.events.Event(ink.embed.events.EventType.REDO_REQUESTED);
  this.dispatchEvent(e);
  if (!e.defaultPrevented) {
    this.canvasManager_.redo();
  }
  var eventProto = ink.util.createDocumentEvent(
      this.getLogsHost(),
      protos.research.ink.InkEvent.DocumentEvent.DocumentEventType.REDO);
  var logEvent = new ink.embed.events.LogEvent(eventProto);
  this.dispatchEvent(logEvent);
};


/**
 * Adds a background image.  Sets page bounds to match the given image.
 * @param {string} imgSrc
 * @param {Function=} opt_cb callback on image load complete
 */
ink.embed.EmbedComponent.prototype.setBackgroundImage =
    function(imgSrc, opt_cb) {
  ink.util.getImageBytes(imgSrc,
    (data, size) => {
      this.canvasManager_.setBackgroundImage(data, size);
      if (opt_cb) opt_cb();
    });
};


/**
 * Sets a background image to match the existing page bounds.  Will not
 * display if no page bounds are set.
 * @param {string} imgSrc
 * @param {Function=} opt_cb callback on image load complete
 */
ink.embed.EmbedComponent.prototype.setImageToUseForPageBackground =
    function(imgSrc, opt_cb) {
  ink.util.getImageBytes(imgSrc,
    (data, size) => {
      this.canvasManager_.setImageToUseForPageBackground(data, size);
      if (opt_cb) opt_cb();
    });
};


/**
 * Set background color.
 * @param {ink.Color} color
 */
ink.embed.EmbedComponent.prototype.setBackgroundColor = function(color) {
  this.canvasManager_.setBackgroundColor(color);
};


/**
 * Export the scene as a PNG from the engine.
 * @param {number} maxWidth
 * @param {boolean} drawBackground
 * @param {function(!goog.html.SafeUrl)} callback
 * @param {boolean=} opt_asBlob If true, returns a blob uri.
 */
ink.embed.EmbedComponent.prototype.exportPng = function(
    maxWidth, drawBackground, callback, opt_asBlob) {
  this.canvasManager_.exportPng(maxWidth, drawBackground, callback, opt_asBlob);
};


/**
 * Set callback flags, for whether to receive callbacks and what data to attach.
 * @param {!sketchology.proto.SetCallbackFlags} setCallbackFlags
 */
ink.embed.EmbedComponent.prototype.setCallbackFlags =
    function(setCallbackFlags) {
  this.canvasManager_.setCallbackFlags(setCallbackFlags);
};


/**
 * Sets the size of the page.
 * @param {number} left
 * @param {number} top
 * @param {number} right
 * @param {number} bottom
 */
ink.embed.EmbedComponent.prototype.setPageBounds =
    function(left, top, right, bottom) {
  this.canvasManager_.setPageBounds(left, top, right, bottom);
};


/**
 * Deselects anything selected with the edit tool.
 */
ink.embed.EmbedComponent.prototype.deselectAll = function() {
  this.canvasManager_.deselectAll();
};


////////////////////////////////////////////////////////////////
// Internal code.
////////////////////////////////////////////////////////////////


/** @override */
ink.embed.EmbedComponent.prototype.createDom = function() {
  this.setElementInternal(goog.soy.renderAsElement(ink.soy.embedContent));
};


/** @override */
ink.embed.EmbedComponent.prototype.enterDocument = function() {
  ink.embed.EmbedComponent.base(this, 'enterDocument');

  var container = goog.dom.getElement('layer-container');

  this.canvasManager_.render(container);
  this.cursorUpdater_.decorate(container);

  this.getHandler().listen(
      this.canvasManager_, ink.embed.events.EventType.CANVAS_INITIALIZED,
      goog.bind(this.callback_, this, this));
};


/**
 * Manually adds an element.
 * @param {!sketchology.proto.Element} elem
 * @param {number} idx index to add the element at
 * @param {boolean} isLocal
 */
ink.embed.EmbedComponent.prototype.addElement = function(elem, idx, isLocal) {
  this.canvasManager_.addElement(elem, idx, isLocal);
};


/**
 * Manually removes a number of elements.
 * @param {number} idx index to start removing.
 * @param {number} count number of items to remove.
 */
ink.embed.EmbedComponent.prototype.removeElements = function(idx, count) {
  this.canvasManager_.removeElements(idx, count);
};


/**
 * Resets the canvas associated with the embed component.
 *
 * Note: Does not affect any attached Brix documents.
 */
ink.embed.EmbedComponent.prototype.resetCanvas = function() {
  this.canvasManager_.resetCanvas();
};


/**
 * Sets or unsets readOnly on the canvas.
 * @param {boolean} readOnly
 */
ink.embed.EmbedComponent.prototype.setReadOnly = function(readOnly) {
  this.canvasManager_.setReadOnly(readOnly);
};


/**
 * Sets element transforms.
 * @param {Array.<string>} uuids
 * @param {Array.<string>} encodedTransforms
 */
ink.embed.EmbedComponent.prototype.setElementTransforms = function(
    uuids, encodedTransforms) {
  this.canvasManager_.setElementTransforms(uuids, encodedTransforms);
};


/**
 * Returns true if the document is empty, false if it has content, and
 *   undefined if not a brix document.
 * @param {Function} callback
 */
ink.embed.EmbedComponent.prototype.isEmpty = function(callback) {
  var e = new ink.embed.events.EmptyStatusRequestedEvent(callback);
  this.dispatchEvent(e);
  if (!e.defaultPrevented) {
    callback(undefined);
  }
};


/**
 * Returns the current dimensions of the canvas element.
 * @return {goog.math.Size} The width and height of the canvas.
 */
ink.embed.EmbedComponent.prototype.getCanvasDimensions = function() {
  var element =
      this.canvasManager_.getElementStrict().querySelector('canvas,embed');
  return new goog.math.Size(element.clientWidth, element.clientHeight);
};


/**
 * Returns the logs host id.
 * @return {protos.research.ink.InkEvent.Host}
 */
ink.embed.EmbedComponent.prototype.getLogsHost = function() {
  return this.config_.logsHost;
};


/**
 * Enable or disable an engine flag.
 * @param {sketchology.proto.Flag} which
 * @param {boolean} enable
 */
ink.embed.EmbedComponent.prototype.assignFlag = function(which, enable) {
  this.canvasManager_.assignFlag(which, enable);
};


/**
 * Returns the current snapshot. Only works if sengineType is set to
 *   ink.util.SEngineType.IN_MEMORY.
 * @param {function(!sketchology.proto.Snapshot)} callback
 */
ink.embed.EmbedComponent.prototype.getSnapshot = function(callback) {
  if (this.config_.sengineType !== ink.util.SEngineType.IN_MEMORY) {
    throw new Error(`Can't getSnapshot without sengineType IN_MEMORY.`);
  }
  this.canvasManager_.getSnapshot(callback);
};


/**
 * Loads a document from a snapshot. Only works if sengineType is set to
 *   ink.util.SEngineType.IN_MEMORY.
 *
 * @param {!sketchology.proto.Snapshot} snapshotProto
 */
ink.embed.EmbedComponent.prototype.loadFromSnapshot = function(snapshotProto) {
  if (this.config_.sengineType !== ink.util.SEngineType.IN_MEMORY) {
    throw new Error(`Can't loadFromSnapshot without sengineType IN_MEMORY.`);
  }
  this.canvasManager_.loadFromSnapshot(snapshotProto);
};


/**
 * Allows the user to execute arbitrary commands on the engine.
 * @param {!sketchology.proto.Command} command
 */
ink.embed.EmbedComponent.prototype.handleCommand = function(command) {
  this.canvasManager_.handleCommand(command);
};


/**
 * Gets the raw engine object. Do not use this.
 * @return {Object}
 */
ink.embed.EmbedComponent.prototype.getRawEngineObject = function() {
  return this.canvasManager_.getRawEngineObject();
};


/**
 * Generates a snapshot based on a brix document.
 * @param {!ink.util.RealtimeDocument} brixDoc
 * @param {function(!sketchology.proto.Snapshot)} callback
 */
ink.embed.EmbedComponent.prototype.convertBrixDocumentToSnapshot =
    function(brixDoc, callback) {
  this.canvasManager_.convertBrixDocumentToSnapshot(brixDoc, callback);
};


/**
 * @param {!sketchology.proto.Snapshot} snapshot
 * @param {function(boolean)} callback
 */
ink.embed.EmbedComponent.prototype.snapshotHasPendingMutations =
    function(snapshot, callback) {
  this.canvasManager_.snapshotHasPendingMutations(snapshot, callback);
};


/**
 * @param {!sketchology.proto.Snapshot} snapshot
 * @param {function(sketchology.proto.MutationPacket)} callback
 */
ink.embed.EmbedComponent.prototype.extractMutationPacket =
    function(snapshot, callback) {
  this.canvasManager_.extractMutationPacket(snapshot, callback);
};


/**
 * @param {!sketchology.proto.Snapshot} snapshot
 * @param {function(sketchology.proto.Snapshot)} callback
 */
ink.embed.EmbedComponent.prototype.clearPendingMutations =
    function(snapshot, callback) {
  this.canvasManager_.clearPendingMutations(snapshot, callback);
};


/**
 * Calls the given callback once all previous asynchronous engine operations
 * have been applied.
 * @param {!Function} callback
 */
ink.embed.EmbedComponent.prototype.flush = function(callback) {
  this.canvasManager_.flush(callback);
};
