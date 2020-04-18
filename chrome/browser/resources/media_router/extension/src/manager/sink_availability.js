// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * @fileoverview Enum for per-media-route-provider sink availability.
 */

goog.provide('mr.SinkAvailability');


/**
 * Per-provider sink availability.
 * Keep in sync with MediaRouter.SinkAvailability in media_router.mojom.
 * @enum {number}
 */
mr.SinkAvailability = {
  UNAVAILABLE: 0,
  PER_SOURCE: 1,
  AVAILABLE: 2
};
