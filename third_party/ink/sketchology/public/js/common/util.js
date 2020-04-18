// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
goog.provide('ink.util');

goog.require('goog.dom');
goog.require('goog.events');
goog.require('goog.math.Size');
goog.require('protos.research.ink.InkEvent');


/** @enum {string} */
ink.util.SEngineType = {
  IN_MEMORY: 'makeSEngineInMemory',
  LONGFORM: 'makeLongformSEngine',
  PASSTHROUGH_DOCUMENT: 'makeSEnginePassthroughDocument'
};


/**
 * @typedef {{
 *   getModel: (function():ink.util.RealtimeModel)
 * }}
 */
ink.util.RealtimeDocument;


/**
 * @typedef {{
 *  getRoot: (function():ink.util.RealtimeRoot)
 * }}
 */
ink.util.RealtimeModel;


/**
 * @typedef {{
 *   get: (function(string):ink.util.RealtimePages)
 * }}
 */
ink.util.RealtimeRoot;


/**
 * @typedef {{
 *   get: (function(number):ink.util.RealtimePage),
 *   xhigh: number,
 *   xlow: number,
 *   yhigh: number,
 *   ylow: number
 * }}
 */
ink.util.RealtimePages;


/**
 * @typedef {{
 *   get: (function(string):ink.util.RealtimeElements)
 * }}
 */
ink.util.RealtimePage;


/**
 * @typedef {{
 *   asArray: (function():Array.<ink.util.RealtimeElement>)
 * }}
 */
ink.util.RealtimeElements;


/**
 * @typedef {{
 *   get: (function(string):string)
 * }}
 */
ink.util.RealtimeElement;


/**
 * Wrapper used for event handlers to pass the event target instead of the
 * event.
 * @param {!Function} callback The event handler function.
 * @param {Object=} opt_handler Element in whole scope to call the callback.
 * @return {!Function} Wrapped function.
 */
ink.util.eventTargetWrapper = function(callback, opt_handler) {
  return function(evt) {
    var self = /** @type {Object} */ (this);
    callback.call(opt_handler || self, evt.target);
  };
};


/**
 * @param {!goog.events.EventTarget} child The child to get the root parent
 *     EventTarget for.
 * @return {!goog.events.EventTarget} The root parent.
 * TODO(esrauch): Rename this function and consider moving it to a new
 * whiteboard/util.js file.
 */
ink.util.getRootParentComponent = function(child) {
  var current = child;
  var parent = current.getParentEventTarget();
  while (parent) {
    current = parent;
    parent = current.getParentEventTarget();
  }
  return current;
};


/**
 * Downloads an image, renders it to a canvas, returns the raw bytes.
 * @param {string} imgSrc
 * @param {Function} callback
 */
ink.util.getImageBytes = function(imgSrc, callback) {
  var canvasElement = goog.dom.createElement(goog.dom.TagName.CANVAS);
  var imgElement = goog.dom.createElement(goog.dom.TagName.IMG);
  imgElement.setAttribute(
      'style',
      'position:absolute;visibility:hidden;top:-1000px;left:-1000px;');
  imgElement.crossOrigin = 'Anonymous';

  goog.events.listenOnce(imgElement, 'load', function() {
    var width = imgElement.width;
    var height = imgElement.height;
    canvasElement.width = width;
    canvasElement.height = height;
    var ctx = canvasElement.getContext('2d');
    ctx.drawImage(imgElement, 0, 0);
    var data = ctx.getImageData(0, 0, width, height);

    document.body.removeChild(imgElement);

    callback(data.data, new goog.math.Size(width, height));
  });

  imgElement.setAttribute('src', imgSrc);
  document.body.appendChild(imgElement);
};


/**
 * Creates document events.
 * @param {!protos.research.ink.InkEvent.Host} host
 * @param {!protos.research.ink.InkEvent.DocumentEvent.DocumentEventType} type
 * @return {!protos.research.ink.InkEvent}
 */
ink.util.createDocumentEvent = function(host, type) {
  var eventProto = new protos.research.ink.InkEvent();
  eventProto.setHost(host);
  eventProto.setEventType(
      protos.research.ink.InkEvent.EventType.DOCUMENT_EVENT);
  var documentEvent = new protos.research.ink.InkEvent.DocumentEvent();
  documentEvent.setEventType(type);
  eventProto.setDocumentEvent(documentEvent);
  return eventProto;
};


/**
 * Helper for constructing a document created event
 *
 * @param {!protos.research.ink.InkEvent.Host} host
 * @param {!protos.research.ink.InkEvent.DocumentEvent.DocumentState} state
 * @param {number} firstLoadTime (which is when the first byte is loaded)
 * @param {number} startLoadTime (which is when the document is first editable).
 * @return {!protos.research.ink.InkEvent}
 */
ink.util.createDocumentOpenedEvent = function(
    host, state, firstLoadTime, startLoadTime) {
  var type =
      protos.research.ink.InkEvent.DocumentEvent.DocumentEventType.OPENED;
  var ev = ink.util.createDocumentEvent(host, type);
  var openedEvent =
      new protos.research.ink.InkEvent.DocumentEvent.OpenedEvent();
  openedEvent.setMillisUntilFirstByteLoaded(firstLoadTime.toString());
  openedEvent.setMillisUntilEditable((goog.now() - startLoadTime).toString());
  var documentEvent = ev.getDocumentEventOrDefault();
  documentEvent.setOpenedEvent(openedEvent);
  documentEvent.setDocumentState(state);
  return ev;
};


/**
 * Helper for constructing a collaborator joined logging event.
 *
 * @param {!protos.research.ink.InkEvent.Host} host
 * @param {!protos.research.ink.InkEvent.DocumentEvent.DocumentState} state
 * @param {boolean} isMe
 * @return {!protos.research.ink.InkEvent}
 */
ink.util.createCollaboratorJoinedDocumentEvent = function(host, state, isMe) {
  var type = protos.research.ink.InkEvent.DocumentEvent.DocumentEventType
                 .COLLABORATOR_JOINED;
  var ev = ink.util.createDocumentEvent(host, type);
  var collaboratorJoinedEvent =
      new protos.research.ink.InkEvent.DocumentEvent.CollaboratorJoined();
  collaboratorJoinedEvent.setIsMe(isMe);
  var documentEvent = ev.getDocumentEventOrDefault();
  documentEvent.setCollaboratorJoinedEvent(collaboratorJoinedEvent);
  documentEvent.setDocumentState(state);
  return ev;
};


/**
 * @const
 */
ink.util.SHAPE_TO_LOG_TOOLTYPE = {
  'CALLIGRAPHY': protos.research.ink.InkEvent.ToolbarEvent.ToolType.CALLIGRAPHY,
  'EDIT': protos.research.ink.InkEvent.ToolbarEvent.ToolType.EDIT_TOOL,
  'HIGHLIGHTER': protos.research.ink.InkEvent.ToolbarEvent.ToolType.HIGHLIGHTER,
  'MAGIC_ERASE': protos.research.ink.InkEvent.ToolbarEvent.ToolType.MAGIC_ERASER,
  'MARKER': protos.research.ink.InkEvent.ToolbarEvent.ToolType.MARKER
};


/**
 * Creates toolbar events.
 * @param {protos.research.ink.InkEvent.Host} host
 * @param {protos.research.ink.InkEvent.ToolbarEvent.ToolEventType} type
 * @param {string} toolType
 * @param {string} color
 * @return {protos.research.ink.InkEvent}
 */
ink.util.createToolbarEvent = function(host, type, toolType, color) {
  var eventProto = new protos.research.ink.InkEvent();
  eventProto.setHost(host);
  eventProto.setEventType(protos.research.ink.InkEvent.EventType.TOOLBAR_EVENT);
  var toolbarEvent = new protos.research.ink.InkEvent.ToolbarEvent();
  // Alpha is always 0xFF. This does #RRGGBB -> #AARRGGBB -> 0xAARRGGBB.
  var colorAsNumber = parseInt('ff' + color.substring(1, 7), 16);
  toolbarEvent.setColor(colorAsNumber);
  toolbarEvent.setToolType(
      ink.util.SHAPE_TO_LOG_TOOLTYPE[toolType] ||
      protos.research.ink.InkEvent.ToolbarEvent.ToolType.UNKNOWN_TOOL_TYPE);
  toolbarEvent.setToolEventType(type);
  eventProto.setToolbarEvent(toolbarEvent);
  return eventProto;
};


// Note: These helpers are included here because we do not use the closure
// browser event wrappers to avoid additional GC pauses.  Logic forked from
// cs/piper///depot/google3/javascript/closure/events/browserevent.js?l=337

/**
 * "Action button" is  MouseEvent.button equal to 0 (main button) and no
 * control-key for Mac right-click action.  The button property is the one
 * that was responsible for triggering a mousedown or mouseup event.
 *
 * @param {MouseEvent} evt
 * @return {boolean}
 */
ink.util.isMouseActionButton = function(evt) {
  return evt.button == 0 &&
         !(goog.userAgent.WEBKIT && goog.userAgent.MAC && evt.ctrlKey);
};


/**
 * MouseEvent.buttons has 1 (main button) held down, and no control-key
 * for Mac right-click drag.  This is for which button is currently held down,
 * e.g. during a mousemove event.
 *
 * @param {MouseEvent} evt
 * @return {boolean}
 */
ink.util.hasMouseActionButton = function(evt) {
  return (evt.buttons & 1) == 1 &&
         !(goog.userAgent.WEBKIT && goog.userAgent.MAC && evt.ctrlKey);
};


/**
 * Checks MouseEvent.buttons has 2 (secondary button) held down, or 1 (main
 * button) with control key for Mac right-click drag.  This is for which
 * button is currently held down, e.g. during a mousemove event.
 *
 * @param {MouseEvent} evt
 * @return {boolean}
 */
ink.util.hasMouseSecondaryButton = function(evt) {
  return (evt.buttons & 2) == 2 ||
         ((evt.buttons & 1) == 1 && goog.userAgent.WEBKIT &&
          goog.userAgent.MAC && evt.ctrlKey);
};

