// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * Set of MetadataCacheItem.
 * @param {!MetadataCacheSetStorage} items Storage object containing
 *     MetadataCacheItem.
 * @extends {cr.EventTarget}
 * @constructor
 * @struct
 */
function MetadataCacheSet(items) {
  cr.EventTarget.call(this);

  /**
   * @private {!MetadataCacheSetStorage}
   * @const
   */
  this.items_ = items;
}

MetadataCacheSet.prototype.__proto__ = cr.EventTarget.prototype;

/**
 * Creates list of MetadataRequest based on the cache state.
 * @param {!Array<!Entry>} entries
 * @param {!Array<string>} names
 * @return {!Array<!MetadataRequest>}
 */
MetadataCacheSet.prototype.createRequests = function(entries, names) {
  var urls = util.entriesToURLs(entries);
  var requests = [];
  for (var i = 0; i < entries.length; i++) {
    var item = this.items_.peek(urls[i]);
    var requestedNames = item ? item.createRequests(names) : names;
    if (requestedNames.length)
      requests.push(new MetadataRequest(entries[i], requestedNames));
  }
  return requests;
};

/**
 * Updates cache states to start the given requests.
 * @param {number} requestId
 * @param {!Array<!MetadataRequest>} requests
 */
MetadataCacheSet.prototype.startRequests = function(requestId, requests) {
  for (var i = 0; i < requests.length; i++) {
    var request = requests[i];
    var url = requests[i].entry['cachedUrl'] || requests[i].entry.toURL();
    var item = this.items_.peek(url);
    if (!item) {
      item = new MetadataCacheItem();
      this.items_.put(url, item);
    }
    item.startRequests(requestId, request.names);
  }
};

/**
 * Stores results from MetadataProvider with the request Id.
 * @param {number} requestId Request ID. If a newer operation has already been
 *     done, the results must be ingored.
 * @param {!Array<!Entry>} entries
 * @param {!Array<!MetadataItem>} results
 * @return {boolean} Whether at least one result is stored or not.
 */
MetadataCacheSet.prototype.storeProperties = function(
    requestId, entries, results) {
  var changedEntries = [];
  var urls = util.entriesToURLs(entries);
  for (var i = 0; i < entries.length; i++) {
    var url = urls[i];
    var item = this.items_.peek(url);
    if (item && item.storeProperties(requestId, results[i]))
      changedEntries.push(entries[i]);
  }
  if (changedEntries.length) {
    var event = new Event('update');
    event.entries = changedEntries;
    this.dispatchEvent(event);
    return true;
  } else {
    return false;
  }
};

/**
 * Obtains cached properties for entries and names.
 * Note that it returns invalidated properties also.
 * @param {!Array<!Entry>} entries Entries.
 * @param {!Array<string>} names Property names.
 */
MetadataCacheSet.prototype.get = function(entries, names) {
  var results = [];
  var urls = util.entriesToURLs(entries);
  for (var i = 0; i < entries.length; i++) {
    var item = this.items_.get(urls[i]);
    results.push(item ? item.get(names) : {});
  }
  return results;
};

/**
 * Marks the caches of entries as invalidates and forces to reload at the next
 * time of startRequests.
 * @param {number} requestId Request ID of the invalidation request. This must
 *     be larger than other requets ID passed to the set before.
 * @param {!Array<!Entry>} entries
 */
MetadataCacheSet.prototype.invalidate = function(requestId, entries) {
  var urls = util.entriesToURLs(entries);
  for (var i = 0; i < entries.length; i++) {
    var item = this.items_.peek(urls[i]);
    if (item)
      item.invalidate(requestId);
  }
};

/**
 * Clears the caches of entries.
 * @param {!Array<string>} urls
 */
MetadataCacheSet.prototype.clear = function(urls) {
  for (var i = 0; i < urls.length; i++) {
    this.items_.remove(urls[i]);
  }
};

/**
 * Clears all cache.
 */
MetadataCacheSet.prototype.clearAll = function() {
  this.items_.removeAll();
};

/**
 * Creates snapshot of the cache for entries.
 * @param {!Array<!Entry>} entries
 */
MetadataCacheSet.prototype.createSnapshot = function(entries) {
  var items = {};
  var urls = util.entriesToURLs(entries);
  for (var i = 0; i < entries.length; i++) {
    var url = urls[i];
    var item = this.items_.peek(url);
    if (item)
      items[url] = item.clone();
  }
  return new MetadataCacheSet(new MetadataCacheSetStorageForObject(items));
};

/**
 * Returns whether all the given properties are fulfilled.
 * @param {!Array<!Entry>} entries Entries.
 * @param {!Array<string>} names Property names.
 * @return {boolean}
 */
MetadataCacheSet.prototype.hasFreshCache = function(entries, names) {
  if (!names.length)
    return true;
  var urls = util.entriesToURLs(entries);
  for (var i = 0; i < entries.length; i++) {
    var item = this.items_.peek(urls[i]);
    if (!(item && item.hasFreshCache(names)))
      return false;
  }
  return true;
};

/**
 * Interface of raw strage for MetadataCacheItem.
 * @interface
 */
function MetadataCacheSetStorage() {
}

/**
 * Returns an item corresponding to the given URL.
 * @param {string} url Entry URL.
 * @return {MetadataCacheItem}
 */
MetadataCacheSetStorage.prototype.get = function(url) {};

/**
 * Returns an item corresponding to the given URL without changing orders in
 * the cache list.
 * @param {string} url Entry URL.
 * @return {MetadataCacheItem}
 */
MetadataCacheSetStorage.prototype.peek = function(url) {};

/**
 * Saves an item corresponding to the given URL.
 * @param {string} url Entry URL.
 * @param {!MetadataCacheItem} item Item to be saved.
 */
MetadataCacheSetStorage.prototype.put = function(url, item) {};

/**
 * Removes an item from the cache.
 * @param {string} url Entry URL.
 */
MetadataCacheSetStorage.prototype.remove = function(url) {};

/**
 * Remove all items from the cache.
 */
MetadataCacheSetStorage.prototype.removeAll = function() {};

/**
 * Implementation of MetadataCacheSetStorage by using raw object.
 * @param {Object} items Map of URL and MetadataCacheItem.
 * @constructor
 * @implements {MetadataCacheSetStorage}
 * @struct
 */
function MetadataCacheSetStorageForObject(items) {
  this.items_ = items;
}

/**
 * @override
 */
MetadataCacheSetStorageForObject.prototype.get = function(url) {
  return this.items_[url];
};

/**
 * @override
 */
MetadataCacheSetStorageForObject.prototype.peek = function(url) {
  return this.items_[url];
};

/**
 * @override
 */
MetadataCacheSetStorageForObject.prototype.put = function(url, item) {
  this.items_[url] = item;
};

/**
 * @override
 */
MetadataCacheSetStorageForObject.prototype.remove = function(url) {
  delete this.items_[url];
};

/**
 * @override
 */
MetadataCacheSetStorageForObject.prototype.removeAll = function() {
  for (var url in this.items_) {
    delete this.items_[url];
  }
};

/**
 * Implementation of MetadataCacheSetStorage by using LRUCache.
 * TODO(hirono): Remove this class.
 * @param {!LRUCache<!MetadataCacheItem>} cache LRUCache.
 * @constructor
 * @implements {MetadataCacheSetStorage}
 * @struct
 */
function MetadataCacheSetStorageForLRUCache(cache) {
  /**
   * @private {!LRUCache<!MetadataCacheItem>}
   * @const
   */
  this.cache_ = cache;
}

/**
 * @override
 */
MetadataCacheSetStorageForLRUCache.prototype.get = function(url) {
  return this.cache_.get(url);
};

/**
 * @override
 */
MetadataCacheSetStorageForLRUCache.prototype.peek = function(url) {
  return this.cache_.peek(url);
};

/**
 * @override
 */
MetadataCacheSetStorageForLRUCache.prototype.put = function(url, item) {
  this.cache_.put(url, item);
};

/**
 * @override
 */
MetadataCacheSetStorageForLRUCache.prototype.remove = function(url) {
  this.cache_.remove(url);
};

/**
 * @override
 */
MetadataCacheSetStorageForLRUCache.prototype.removeAll = function() {
  assertNotReached('Not implemented.');
};