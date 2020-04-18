// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * @fileoverview Interface to a presentation session.
 */

goog.provide('mr.presentation.Session');



/**
 * Creates a new PresentationSession.
 * @record
 */
mr.presentation.Session = class {
  /**
   * Starts the presentation session.
   * @return {!Promise<!mr.Route>} Fulfilled when the
   *     session has been created and transports have started.
   */
  start() {}
  /**
   * Stops the presentation session. The underlying streams and transports are
   * stopped and destroyed.
   *
   * @return {!Promise} Promise that resolves when the session is stopped.
   */
  stop() {}
};
