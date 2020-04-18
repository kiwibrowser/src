// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Namespace
var importer = importer || {};

/**
 * A duplicate finder for testing.  Allows the return value to be set.
 * @constructor
 * @implements {importer.DuplicateFinder}
 * @struct
 */
importer.TestDuplicateFinder = function() {
  /** @type {boolean} */
  this.returnValue = false;
};

/** @override */
importer.TestDuplicateFinder.prototype.checkDuplicate = function(entry) {
  return Promise.resolve(this.returnValue);
};

/** @override */
importer.TestDuplicateFinder.prototype.getStatistics = function() {
  return {
    computeHashTime: 0,
    searchHashTime: 0
  };
};

/**
 * @constructor
 * @implements {importer.DuplicateFinder.Factory}
 */
importer.TestDuplicateFinder.Factory = function() {
  /** @private {!Array<!importer.DuplicateFinder>} */
  this.instances = [];
};

/** @override */
importer.TestDuplicateFinder.Factory.prototype.create = function() {
  var newInstance = new importer.TestDuplicateFinder();
  this.instances.push(newInstance);
  return newInstance;
};
