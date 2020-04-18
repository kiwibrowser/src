// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * Interface for the Polymer element that shows media controls for a route that
 * is currently cast to a device.
 * @record
 */
function RouteControlsInterface() {}

/**
 * @type {!media_router.RouteStatus}
 */
RouteControlsInterface.prototype.routeStatus;

/**
 * Resets the route controls. Called when the route details view is closed.
 */
RouteControlsInterface.prototype.reset = function() {};
