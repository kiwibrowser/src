// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * @fileoverview Service name uniquely identifying each service.
 */


goog.provide('mr.mirror.ServiceName');


/**
 * @enum {string}
 */
mr.mirror.ServiceName = {
  CAST_STREAMING: 'cast_streaming',
  HANGOUTS: 'hangouts',
  MEETINGS: 'meetings',
  WEBRTC: 'webrtc'
};
