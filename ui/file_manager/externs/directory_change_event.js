// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * @constructor
 * @extends {Event}
 * @struct
 */
var DirectoryChangeEvent = function() {};

/** @type {DirectoryEntry} */
DirectoryChangeEvent.prototype.previousDirEntry;

/** @type {DirectoryEntry|FakeEntry} */
DirectoryChangeEvent.prototype.newDirEntry;

/** @type {boolean} */
DirectoryChangeEvent.prototype.volumeChanged;