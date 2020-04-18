// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * Casts an |object| to sinon.TestStub verifying that it's really a stub.
 * @return {sinon.TestStub}
 */
function $testStub(/** Object */ object) {
  console.assert(object.hasOwnProperty("getCall"),
                 'Object is not a stub', object);
  return /** @type {sinon.TestStub} */ (object);
};
