// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * @fileoverview Information about the current build configuration.
 */
goog.module('mr.Config');
goog.module.declareLegacyNamespace();


/**
 * Compiler flag used to enable debug/testing only components.  The default
 * value defined here is only used in open-source builds.
 * @define {boolean} True if this extension was released through debug channel.
 */
exports.isDebugChannel = true;


/**
 * Compiler flag used to set logging level and other privacy sensitive config
 * for public release.  The default value defined here is only used in
 * open-source builds.
 * @define {boolean} True if this extension was released through public channel.
 */
exports.isPublicChannel = false;


/**
 * Used to determine whether code is being executed as part of a unit test.
 * This is used only to control logging configuration.  Don't use it for any
 * other purpose!
 * @const
 */
exports.isUnitTest = 'jasmine' in window;
