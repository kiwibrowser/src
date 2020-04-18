// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * @fileoverview Defines presentation connection related enums.
 */

goog.provide('mr.PresentationConnectionCloseReason');
goog.provide('mr.PresentationConnectionState');


/**
 * Presentation connection states.  Keep in sync with PresentationConnection.idl
 * in the Chromium code base.
 *
 * @enum {string}
 */
mr.PresentationConnectionState = {
  CONNECTED: 'connected',
  TERMINATED: 'terminated',
  CLOSED: 'closed'
};


/**
 * Keep in sync with PresentationConnectionCloseEvent.idl in Chromium code base.
 *
 * @enum {string}
 */
mr.PresentationConnectionCloseReason = {
  ERROR: 'error',
  CLOSED: 'closed',
  WENT_AWAY: 'went_away'
};
