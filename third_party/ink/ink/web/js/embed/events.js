// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
/**
 * @fileoverview Events that embedders can listen to. Note: currently embedders
 * are able to listen to other internal events, but only these events should be
 * treated as a public API.
 */
goog.provide('ink.embed.events');
goog.provide('ink.embed.events.DoneLoadingEvent');
goog.provide('ink.embed.events.ElementCreatedEvent');
goog.provide('ink.embed.events.ElementsMutatedEvent');
goog.provide('ink.embed.events.ElementsRemovedEvent');
goog.provide('ink.embed.events.EmptyStatusRequestedEvent');
goog.provide('ink.embed.events.EventType');
goog.provide('ink.embed.events.FatalErrorEvent');
goog.provide('ink.embed.events.LogEvent');

goog.require('goog.events');
goog.require('protos.research.ink.InkEvent');


/** @enum {string} */
ink.embed.events.EventType = {
  // Dispatched when the space is ready to be drawn onto.
  DONE_LOADING: goog.events.getUniqueId('done_loading'),

  // Dispatched when pen mode is enabled.
  PEN_MODE_ENABLED: goog.events.getUniqueId('pen_mode_enabled'),

  // Dispatched when the GL canvas is initialized.
  CANVAS_INITIALIZED: goog.events.getUniqueId('canvas_initialized'),

  // Dispatched when a new element has been created on the canvas.
  ELEMENT_CREATED: goog.events.getUniqueId('element_created'),

  // Dispatched when an element is mutaed.
  ELEMENTS_MUTATED: goog.events.getUniqueId('elements_mutated'),

  // Dispatched when elements are removed.
  ELEMENTS_REMOVED: goog.events.getUniqueId('elements_removed'),

  // Dispatched when something wants to know if the document is empty.
  EMPTY_STATUS_REQUESTED: goog.events.getUniqueId('empty_status_requested'),

  // Dispatched when something needs to be logged.
  LOG: goog.events.getUniqueId('log'),

  // Dispatched to request an undo.
  UNDO_REQUESTED: goog.events.getUniqueId('undo_requested'),

  // Dispatched to request a redo.
  REDO_REQUESTED: goog.events.getUniqueId('redo_requested'),

  // Dispatched to request a clear.
  CLEAR_REQUESTED: goog.events.getUniqueId('clear_requested'),

  // Dispatched when a fatal error occurs and the canvas is no longer valid.
  // If this is not handled, an Error is thrown (or re-raised).  The canvas
  // should be discarded and a new one constructed.
  FATAL_ERROR: goog.events.getUniqueId('fatal_error')
};



/**
 * An event fired when the embed is ready to be drawn onto.
 *
 * @param {Object} brixDoc The brix realtime document associated with
 * the document that has been loaded.
 * @param {boolean} isReadOnly Whether the document is read only.
 *
 * @extends {goog.events.Event}
 * @constructor
 * @struct
 */
ink.embed.events.DoneLoadingEvent = function(brixDoc, isReadOnly) {
  ink.embed.events.DoneLoadingEvent.base(
      this, 'constructor', ink.embed.events.EventType.DONE_LOADING);

  /** @type {Object} */
  this.brixDoc = brixDoc;

  /** @type {boolean} */
  this.isReadOnly = isReadOnly;
};
goog.inherits(ink.embed.events.DoneLoadingEvent, goog.events.Event);


/**
 * An event fired when pen mode is enabled or disabled.
 *
 * @param {boolean} enabled
 *
 * @extends {goog.events.Event}
 * @constructor
 * @struct
 */
ink.embed.events.PenModeEnabled = function(enabled) {
  ink.embed.events.PenModeEnabled.base(this, 'constructor',
      ink.embed.events.EventType.PEN_MODE_ENABLED);

  /** @type {boolean} */
  this.enabled = enabled;
};
goog.inherits(ink.embed.events.PenModeEnabled, goog.events.Event);


/**
 * An event fired when a new element is created in the embed.
 *
 * @param {string} uuid
 * @param {string} encodedElement
 * @param {string} encodedTransform
 *
 * @extends {goog.events.Event}
 * @constructor
 * @struct
 */
ink.embed.events.ElementCreatedEvent = function(uuid, encodedElement,
    encodedTransform) {
  ink.embed.events.ElementCreatedEvent.base(
      this, 'constructor', ink.embed.events.EventType.ELEMENT_CREATED);

  this.uuid = uuid;
  this.encodedElement = encodedElement;
  this.encodedTransform = encodedTransform;
};
goog.inherits(ink.embed.events.ElementCreatedEvent, goog.events.Event);


/**
 * An event fired when an element is mutated.
 *
 * @param {Array.<string>} uuids
 * @param {Array.<string>} encodedTransforms
 *
 * @extends {goog.events.Event}
 * @constructor
 * @struct
 */
ink.embed.events.ElementsMutatedEvent = function(uuids, encodedTransforms) {
  ink.embed.events.ElementsMutatedEvent.base(
      this, 'constructor', ink.embed.events.EventType.ELEMENTS_MUTATED);

  this.uuids = uuids;
  this.encodedTransforms = encodedTransforms;
};
goog.inherits(ink.embed.events.ElementsMutatedEvent, goog.events.Event);


/**
 * An event fired when elements are removed.
 *
 * @param {Array.<string>} uuids
 *
 * @extends {goog.events.Event}
 * @constructor
 * @struct
 */
ink.embed.events.ElementsRemovedEvent = function(uuids) {
  ink.embed.events.ElementsRemovedEvent.base(
      this, 'constructor', ink.embed.events.EventType.ELEMENTS_REMOVED);

  this.uuids = uuids;
};
goog.inherits(ink.embed.events.ElementsRemovedEvent, goog.events.Event);


/**
 * An event fired when something wants to know if the document is empty.
 *
 * @param {Function} callback
 *
 * @extends {goog.events.Event}
 * @constructor
 * @struct
 */
ink.embed.events.EmptyStatusRequestedEvent = function(callback) {
  ink.embed.events.EmptyStatusRequestedEvent.base(
      this, 'constructor', ink.embed.events.EventType.EMPTY_STATUS_REQUESTED);

  this.callback = callback;
};
goog.inherits(ink.embed.events.EmptyStatusRequestedEvent, goog.events.Event);


/**
 * An event fired when an event should be logged by the embedder.
 *
 * @param {protos.research.ink.InkEvent} proto The ink event proto.
 *
 * @extends {goog.events.Event}
 * @constructor
 * @struct
 */
ink.embed.events.LogEvent = function(proto) {
  ink.embed.events.LogEvent.base(
      this, 'constructor', ink.embed.events.EventType.LOG);

  this.proto = proto;
};
goog.inherits(ink.embed.events.LogEvent, goog.events.Event);


/**
 * An event fired when a fatal error has occured.
 *
 * If this error is not handled by the embedder, the component will throw an
 * error.  The embedder should discard this component and construct a new one.
 *
 * @param {Error=} opt_cause Optional cause of the fatal error
 *
 * @extends {goog.events.Event}
 * @constructor
 * @struct
 */
ink.embed.events.FatalErrorEvent = function(opt_cause) {
  ink.embed.events.FatalErrorEvent.base(
      this, 'constructor', ink.embed.events.EventType.FATAL_ERROR);

  /** @type {Error} */
  this.cause = opt_cause || null;
};
goog.inherits(ink.embed.events.FatalErrorEvent, goog.events.Event);
