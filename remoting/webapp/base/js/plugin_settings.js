// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Contains all the settings that may need massaging by the build script.
// Keeping all that centralized here allows us to use symlinks for the other
// files making for a faster compile/run cycle when only modifying HTML/JS.

'use strict';

/** @suppress {duplicate} */
var remoting = remoting || {};

/** @type {remoting.Settings} */
remoting.settings = null;
/** @constructor */
remoting.Settings = function() {};

// The settings on this file are automatically substituted by build-webapp.py.
// Do not override them manually, except for running local tests.

/** @type {string} API client ID.*/
remoting.Settings.prototype.OAUTH2_CLIENT_ID = 'API_CLIENT_ID';
/** @type {string} API client secret.*/
remoting.Settings.prototype.OAUTH2_CLIENT_SECRET = 'API_CLIENT_SECRET';
/** @type {string} Google API Key.*/
remoting.Settings.prototype.GOOGLE_API_KEY = 'API_KEY';

/** @type {string} Base URL for OAuth2 authentication. */
remoting.Settings.prototype.OAUTH2_BASE_URL = 'OAUTH2_BASE_URL';
/** @type {string} Base URL for the OAuth2 API. */
remoting.Settings.prototype.OAUTH2_API_BASE_URL = 'OAUTH2_API_BASE_URL';
/** @type {string} Base URL for the Remoting Directory REST API. */
remoting.Settings.prototype.DIRECTORY_API_BASE_URL = 'DIRECTORY_API_BASE_URL';
/** @type {string} URL for the talk gadget web service. */
remoting.Settings.prototype.TALK_GADGET_URL = 'TALK_GADGET_URL';
/** @type {string} Base URL for the telemetry REST API. */
remoting.Settings.prototype.TELEMETRY_API_BASE_URL = 'TELEMETRY_API_BASE_URL';

/**
 * @return {string} OAuth2 redirect URI. Note that this needs to be a function
 *     because it gets expanded at compile-time to an expression that involves
 *     a chrome API. Since this file is loaded into the WCS sandbox, which has
 *     no access to these APIs, we can't call it at global scope.
 */
remoting.Settings.prototype.OAUTH2_REDIRECT_URL = function() {
  return 'OAUTH2_REDIRECT_URL';
};

/** @type {string} Base URL for the App Remoting API. */
remoting.Settings.prototype.APP_REMOTING_API_BASE_URL =
    'APP_REMOTING_API_BASE_URL';

/** @type {string} XMPP JID for the remoting directory server bot. */
remoting.Settings.prototype.DIRECTORY_BOT_JID = 'DIRECTORY_BOT_JID';

// XMPP server connection settings.
/** @type {string} XMPP server name and port. */
remoting.Settings.prototype.XMPP_SERVER = 'XMPP_SERVER';
/** @type {boolean} Whether to use TLS on connections to the XMPP server. */
remoting.Settings.prototype.XMPP_SERVER_USE_TLS =
    !!'XMPP_SERVER_USE_TLS';

// Third party authentication settings.
/** @type {string} The third party auth redirect URI. */
remoting.Settings.prototype.THIRD_PARTY_AUTH_REDIRECT_URI =
    'THIRD_PARTY_AUTH_REDIRECT_URL';

/** @const {boolean} If true, use GCD instead of Chromoting registry. */
remoting.Settings.prototype.USE_GCD = !!'USE_GCD';
