// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * @fileoverview Implements a helper using USB gnubbies.
 */
'use strict';

/**
 * @constructor
 * @extends {GenericRequestHelper}
 */
function UsbHelper() {
  GenericRequestHelper.apply(this, arguments);

  var self = this;
  this.registerHandlerFactory('enroll_helper_request', function(request) {
    return new UsbEnrollHandler(/** @type {EnrollHelperRequest} */ (request));
  });
  this.registerHandlerFactory('sign_helper_request', function(request) {
    return new UsbSignHandler(/** @type {SignHelperRequest} */ (request));
  });
}

inherits(UsbHelper, GenericRequestHelper);
