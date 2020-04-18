// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * This variable is checked in SelectFileDialogExtensionBrowserTest.
 * @type {number}
 */
window.JSErrorCount = 0;

/**
 * Counts uncaught exceptions.
 */
window.onerror = function() { window.JSErrorCount++; };
