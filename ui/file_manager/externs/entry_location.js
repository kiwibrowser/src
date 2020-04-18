// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * Location information which shows where the path points in FileManager's
 * file system.
 * @interface
 */
function EntryLocation() {};

/**
 * Volume information.
 * @type {!VolumeInfo}
 */
EntryLocation.prototype.volumeInfo;

/**
 * Root type.
 * @type {VolumeManagerCommon.RootType}
 */
EntryLocation.prototype.rootType;

/**
 * Whether the entry is root entry or not.
 * @type {boolean}
 */
EntryLocation.prototype.isRootEntry;

/**
 * Whether the location obtained from the fake entry corresponds to special
 * searches.
 * @type {boolean}
 */
EntryLocation.prototype.isSpecialSearchRoot;

/**
 * Whether the location is under Google Drive or a special search root which
 * represents a special search from Google Drive.
 * @type {boolean}
 */
EntryLocation.prototype.isDriveBased;

/**
 * Whether the entry is read only or not.
 * @type {boolean}
 */
EntryLocation.prototype.isReadOnly;

/**
 * Whether the entry should be displayed with a fixed name instead of individual
 * entry's name. (e.g. "Downloads" is a fixed name)
 * @type {boolean}
 */
EntryLocation.prototype.hasFixedLabel;
