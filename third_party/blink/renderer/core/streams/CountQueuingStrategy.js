// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// eslint-disable-next-line no-unused-vars
(function(global, binding, v8) {
  'use strict';

  const defineProperty = global.Object.defineProperty;

  class CountQueuingStrategy {
    constructor(options) {
      defineProperty(this, 'highWaterMark', {
        value: options.highWaterMark,
        enumerable: true,
        configurable: true,
        writable: true
      });
    }

    size() {
      return 1;
    }
  }

  defineProperty(global, 'CountQueuingStrategy', {
    value: CountQueuingStrategy,
    enumerable: false,
    configurable: true,
    writable: true
  });

  // Export a separate copy that doesn't need options objects and can't be
  // interfered with.
  class BuiltInCountQueuingStrategy {
    constructor(highWaterMark) {
      defineProperty(this, 'highWaterMark', {value: highWaterMark});
    }

    size() {
      return 1;
    }
  }

  binding.createBuiltInCountQueuingStrategy = highWaterMark =>
      new BuiltInCountQueuingStrategy(highWaterMark);
});
