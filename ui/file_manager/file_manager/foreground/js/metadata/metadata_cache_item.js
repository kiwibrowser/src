// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * Cache of metadata for a FileEntry.
 * @constructor
 * @struct
 */
function MetadataCacheItem() {
  /**
   * Map of property name and MetadataCacheItemProperty.
   * @private {!Object<!MetadataCacheItemProperty>}
   * @const
   */
  this.properties_ = {};
}

/**
 * Creates requested names that need to be loaded.
 * @param {!Array<string>} names
 * @return {!Array<string>} Property names that need to be loaded.
 */
MetadataCacheItem.prototype.createRequests = function(names) {
  var loadRequested = [];
  for (var i = 0; i < names.length; i++) {
    var name = names[i];
    assert(!/Error$/.test(name));
    // Check if the property needs to be updated.
    if (this.properties_[name] &&
        this.properties_[name].state !==
        MetadataCacheItemPropertyState.INVALIDATED) {
      continue;
    }
    loadRequested.push(name);
  }
  return loadRequested;
};

/**
 * Marks the given properies as loading.
 * @param {number} requestId
 * @param {!Array<string>} names
 */
MetadataCacheItem.prototype.startRequests = function(requestId, names) {
  for (var i = 0; i < names.length; i++) {
    var name = names[i];
    assert(!/Error$/.test(name));
    if (!this.properties_[name])
      this.properties_[name] = new MetadataCacheItemProperty();
    this.properties_[name].requestId = requestId;
    this.properties_[name].state = MetadataCacheItemPropertyState.LOADING;
  }
};

/**
 * Feeds the result of startRequests.
 * @param {number} requestId Request ID passed when calling startRequests.
 * @param {!MetadataItem} typedObject Map of property name and value.
 * @return {boolean} Whether at least one property is updated or not.
 */
MetadataCacheItem.prototype.storeProperties = function(requestId, typedObject) {
  var changed = false;
  var object = /** @type {!Object} */(typedObject);
  for (var name in object) {
    if (/.Error$/.test(name) && object[name])
      object[name.substr(0, name.length - 5)] = undefined;
  }
  for (var name in object) {
    if (/.Error$/.test(name))
      continue;
    if (!this.properties_[name])
      this.properties_[name] = new MetadataCacheItemProperty();
    if (requestId < this.properties_[name].requestId ||
        this.properties_[name].state ===
        MetadataCacheItemPropertyState.FULFILLED) {
      continue;
    }
    changed = true;
    this.properties_[name].requestId = requestId;
    this.properties_[name].value = object[name];
    this.properties_[name].error = object[name + 'Error'];
    this.properties_[name].state = MetadataCacheItemPropertyState.FULFILLED;
  }
  return changed;
};

/**
 * Marks the caches of all properties in the item as invalidates and forces to
 * reload at the next time of startRequests.
 * @param {number} requestId Request ID of the invalidation request. This must
 *     be larger than other requets ID passed to the item before.
 */
MetadataCacheItem.prototype.invalidate = function(requestId) {
  for (var name in this.properties_) {
    assert(this.properties_[name].requestId < requestId);
    this.properties_[name].requestId = requestId;
    this.properties_[name].state = MetadataCacheItemPropertyState.INVALIDATED;
  }
};

/**
 * Obtains property for entries and names.
 * Note that it returns invalidated properties also.
 * @param {!Array<string>} names
 * @return {!MetadataItem}
 */
MetadataCacheItem.prototype.get = function(names) {
  var result = /** @type {!Object} */(new MetadataItem());
  for (var i = 0; i < names.length; i++) {
    var name = names[i];
    assert(!/Error$/.test(name));
    if (this.properties_[name]) {
      result[name] = this.properties_[name].value;
      result[name + 'Error'] = this.properties_[name].error;
    }
  }
  return /** @type {!MetadataItem} */(result);
};

/**
 * Creates deep copy of the item.
 * @return {!MetadataCacheItem}
 */
MetadataCacheItem.prototype.clone = function() {
  var clonedItem = new MetadataCacheItem();
  for (var name in this.properties_) {
    var property = this.properties_[name];
    clonedItem.properties_[name] = new MetadataCacheItemProperty();
    clonedItem.properties_[name].value = property.value;
    clonedItem.properties_[name].error = property.error;
    clonedItem.properties_[name].requestId = property.requestId;
    clonedItem.properties_[name].state = property.state;
  }
  return clonedItem;
};

/**
 * Returns whether all the given properties are fulfilled.
 * @param {!Array<string>} names Property names.
 * @return {boolean}
 */
MetadataCacheItem.prototype.hasFreshCache = function(names) {
  for (var i = 0; i < names.length; i++) {
    if (!(this.properties_[names[i]] &&
          this.properties_[names[i]].state ===
          MetadataCacheItemPropertyState.FULFILLED)) {
      return false;
    }
  }
  return true;
};

/**
 * @enum {string}
 */
var MetadataCacheItemPropertyState = {
  INVALIDATED: 'invalidated',
  LOADING: 'loading',
  FULFILLED: 'fulfilled'
};

/**
 * Cache of metadata for a property.
 * @constructor
 * @struct
 */
function MetadataCacheItemProperty() {
  /**
   * Cached value of property.
   * @public {*}
   */
  this.value = null;

  /**
   * @public {Error}
   */
  this.error = null;

  /**
   * Last request ID.
   * @public {number}
   */
  this.requestId = -1;

  /**
   * Cache state of the property.
   * @public {MetadataCacheItemPropertyState}
   */
  this.state = MetadataCacheItemPropertyState.INVALIDATED;
}
