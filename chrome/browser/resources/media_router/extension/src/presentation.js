// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * @fileoverview Presentation API.
 * @externs
 * @see http://w3c.github.io/presentation-api
 * @see https://code.google.com/p/chromium/codesearch#chromium/src/third_party/WebKit/Source/modules/presentation/
 */



/**
 * @interface
 * @see http://w3c.github.io/presentation-api/#idl-def-presentationconnection
 */
function PresentationConnection() {}


/**
 * @type {string}
 * @see http://w3c.github.io/presentation-api/#dom-presentationconnection-id
 */
PresentationConnection.prototype.id;


/**
 * @type {string}
 * @see http://w3c.github.io/presentation-api/#dom-presentationconnection-url
 */
PresentationConnection.prototype.url;


/**
 * @type {string}
 * @see http://w3c.github.io/presentation-api/#dom-presentationconnection-state
 */
PresentationConnection.prototype.state;


/**
 * @type {?function(!MessageEvent)}
 * @see http://w3c.github.io/presentation-api/#dom-presentationconnection-onmessage
 */
PresentationConnection.prototype.onmessage;


/**
 * @type {?function(!PresentationConnectionCloseEvent)}
 * @see http://w3c.github.io/presentation-api/#dom-presentationconnection-onclose
 */
PresentationConnection.prototype.onclose;


/**
 * @type {?function()}
 * @see https://www.w3.org/TR/presentation-api/#dom-presentationconnection-onterminate
 */
PresentationConnection.prototype.onterminate;


/**
 * @param {string} message
 * @see http://w3c.github.io/presentation-api/#dom-presentationconnection-send
 */
PresentationConnection.prototype.send = function(message) {};

/**
 * @see https://w3c.github.io/presentation-api/#dom-presentationconnection-terminate
 */
PresentationConnection.prototype.terminate = function() {};

/**
 * @see http://w3c.github.io/presentation-api/#dom-presentationconnection-close
 */
PresentationConnection.prototype.close = function() {};



/**
 * @constructor
 * @extends {Event}
 */
function PresentationConnectionCloseEvent() {}


/** @type {string} */
PresentationConnectionCloseEvent.prototype.reason;


/** @type {string} */
PresentationConnectionCloseEvent.prototype.message;



/**
 * @constructor
 * @extends {Event}
 * @see http://w3c.github.io/presentation-api/#presentationavailability
 */
function PresentationAvailability() {}


/** @type {boolean} */
PresentationAvailability.prototype.value;


/** @type {?function()} */
PresentationAvailability.prototype.onchange;



/**
 * @constructor
 * @extends {Event}
 * @see http://w3c.github.io/presentation-api/#availablechangeevent
 */
function AvailableChangeEvent() {}


/**
 * @type {boolean}
 */
AvailableChangeEvent.prototype.available;



/**
 * @constructor
 * @extends {Event}
 */
function PresentationConnectionAvailableEvent() {}


/**
 * @type {!PresentationConnection}
 */
PresentationConnectionAvailableEvent.prototype.connection;



/**
 * @param {string|!Array<string>} url
 * @constructor
 */
function PresentationRequest(url) {}


/**
 * @return {!Promise<!PresentationConnection>}
 */
PresentationRequest.prototype.start = function() {};


/**
 * @param {string} presentationId
 * @return {!Promise<!PresentationConnection>}
 */
PresentationRequest.prototype.reconnect = function(presentationId) {};


/**
 * @return {!Promise<!PresentationAvailability>}
 */
PresentationRequest.prototype.getAvailability = function() {};


/**
 * @type {?function(!PresentationConnectionAvailableEvent)}
 */
PresentationRequest.prototype.onconnectionavailable;



/**
 * @interface
 */
function Presentation() {}


/**
 * @type {PresentationRequest}
 */
Presentation.prototype.defaultRequest;


/**
 * @type {!Presentation}
 */
Navigator.prototype.presentation;
