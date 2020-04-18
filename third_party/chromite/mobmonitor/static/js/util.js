// Copyright 2015 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

'use strict';

var SERVICE_CONTAINER_PREFIX = 'serviceHealthContainer';


function InvalidRpcArgumentError(message) {
  this.message = message;
}
InvalidRpcArgumentError.prototype = new Error;

function isEmpty(x) {
  if (typeof(x) === 'undefined' || x === null)
    return true;

  return $.isEmptyObject(x);
}
