// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// This file contains type definitions for various remoting classes.
// It is used only with JSCompiler to verify the type-correctness of our code.

/** @suppress {duplicate} */
var remoting = remoting || {};


/** @constructor
 */
remoting.WcsIqClient = function() {};

/** @param {function(Array<string>): void} onMsg The function called when a
 *      message is received.
 *  @return {void} Nothing. */
remoting.WcsIqClient.prototype.setOnMessage = function(onMsg) {};

/** @return {void} Nothing. */
remoting.WcsIqClient.prototype.connectChannel = function() {};

/** @param {string} stanza An IQ stanza.
 *  @return {void} Nothing. */
remoting.WcsIqClient.prototype.sendIq = function(stanza) {};

/** @param {string} token An OAuth2 access token.
 *  @return {void} Nothing. */
remoting.WcsIqClient.prototype.updateAccessToken = function(token) {};
