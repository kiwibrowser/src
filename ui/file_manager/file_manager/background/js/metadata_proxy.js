// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Namespace
var metadataProxy = {};

/**
 * Maximum number of entries whose metadata can be cached.
 * @const {number}
 * @private
 */
metadataProxy.MAX_CACHED_METADATA_ = 10000;

/**
 * @private {!LRUCache<!Metadata>}
 */
metadataProxy.cache_ = new LRUCache(metadataProxy.MAX_CACHED_METADATA_);

/**
 * Returns metadata for the given FileEntry. Uses cached metadata if possible.
 *
 * @param {!FileEntry} entry
 * @return {!Promise<!Metadata>}
 */
metadataProxy.getEntryMetadata = function(entry) {
  var entryURL = entry.toURL();
  if (metadataProxy.cache_.hasKey(entryURL)) {
    return Promise.resolve(metadataProxy.cache_.get(entryURL));
  } else {
    return new Promise(function(resolve, reject) {
      entry.getMetadata(function(metadata) {
        metadataProxy.cache_.put(entryURL, metadata);
        resolve(metadata);
      }, reject);
    });
  }
};
