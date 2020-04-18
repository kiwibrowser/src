// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

'use strict';

var chrome = {
  metricsPrivate: {
    MetricTypeType: {
      HISTOGRAM_LOG: 'histogram-log',
      HISTOGRAM_LINEAR: 'histogram-linear'
    },
    recordPercentage: function() {},
    recordValue: function() {}
  },
  i18n: {
    getMessage: function() {}
  }
};

/**
 * Lets the client to load URL and returns the local cache (not caches in the
 * image loader extension) is used or not.
 *
 * @param {ImageLoaderClient} client
 * @param {string} url URL
 * @param {Object} options load options.
 * @return {Promise<boolean>} True if the local cache is used.
 */
function loadAndCheckCacheUsed(client, url, options) {
  var cacheUsed = true;

  ImageLoaderClient.sendMessage_ = function(message, callback) {
    cacheUsed = false;
    if (callback)
      callback({data: 'ImageData', width: 100, height: 100, status: 'success'});
  };

  return new Promise(function(fulfill) {
    client.load(url, function() {
      fulfill(cacheUsed);
    }, options);
  });
}

function testCache(callback) {
  var client = new ImageLoaderClient();
  reportPromise(
      loadAndCheckCacheUsed(
          client, 'http://example.com/image.jpg', {cache: true}).
      then(function(cacheUsed) {
        assertFalse(cacheUsed);
        return loadAndCheckCacheUsed(
            client, 'http://example.com/image.jpg', {cache: true});
      }).
      then(function(cacheUsed) {
        assertTrue(cacheUsed);
      }),
      callback);
}

function testNoCache(callback) {
  var client = new ImageLoaderClient();
  reportPromise(
      loadAndCheckCacheUsed(
          client, 'http://example.com/image.jpg', {cache: false}).
      then(function(cacheUsed) {
        assertFalse(cacheUsed);
        return loadAndCheckCacheUsed(
            client, 'http://example.com/image.jpg', {cache: false});
      }).
      then(function(cacheUsed) {
        assertFalse(cacheUsed);
      }),
      callback);
}

function testDataURLCache(callback) {
  var client = new ImageLoaderClient();
  reportPromise(
      loadAndCheckCacheUsed(client, 'data:URI', {cache: true}).
      then(function(cacheUsed) {
        assertFalse(cacheUsed);
        return loadAndCheckCacheUsed(client, 'data:URI', {cache: true});
      }).
      then(function(cacheUsed) {
        assertFalse(cacheUsed);
      }),
      callback);
}
