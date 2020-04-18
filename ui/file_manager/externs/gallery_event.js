// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.


/**
 * @constructor
 * @extends {Event}
 */
var GalleryEvent = function() {};

/**
 * @type {Entry}
 */
GalleryEvent.prototype.oldEntry;

/**
 * @type {GalleryItem}
 */
GalleryEvent.prototype.item;
