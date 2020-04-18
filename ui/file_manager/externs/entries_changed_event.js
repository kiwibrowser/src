// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * @constructor
 * @extends {Event}
 */
var EntriesChangedEvent = function() {};

/** @type {util.EntryChangedKind} */
EntriesChangedEvent.prototype.kind;

/** @type {Array<!Entry>} */
EntriesChangedEvent.prototype.entries;