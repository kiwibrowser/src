// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * @constructor
 * @extends {Event}
 */
var FileOperationProgressEvent = function() {};

/** @type {fileOperationUtil.EventRouter.EventType} */
FileOperationProgressEvent.prototype.reason;

/** @type {(fileOperationUtil.Error|undefined)} */
FileOperationProgressEvent.prototype.error;