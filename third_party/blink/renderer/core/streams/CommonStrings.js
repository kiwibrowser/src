// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// User-visible strings shared between ReadableStream and WritableStream.

// eslint-disable-next-line no-unused-vars
(function(global, binding, v8) {
  'use strict';

  binding.streamErrors = {
    illegalInvocation: 'Illegal invocation',
    illegalConstructor: 'Illegal constructor',
    invalidType: 'Invalid type is specified',
    invalidSize: 'The return value of a queuing strategy\'s size function ' +
        'must be a finite, non-NaN, non-negative number',
    sizeNotAFunction: 'A queuing strategy\'s size property must be a function',
    invalidHWM:
    'A queueing strategy\'s highWaterMark property must be a nonnegative, ' +
        'non-NaN number',
  };
});
