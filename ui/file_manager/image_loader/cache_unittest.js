// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

'use strict';

function testCreateCacheKey() {
  var key = ImageCache.createKey({url: 'http://example.com/image.jpg'});
  assertTrue(!!key);
}

function testNotCreateCacheKey() {
  var key = ImageCache.createKey({url: 'data:xxx'});
  assertFalse(!!key);

  var key = ImageCache.createKey({url: 'DaTa:xxx'});
  assertFalse(!!key);
}
