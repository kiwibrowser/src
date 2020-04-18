// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

var Event = require('event_bindings').Event;

// The EasyUnlockProximityRequired object is just a stub that has an onChange
// event, which is never triggered.
// TODO(devlin): Remove this once the preferencesPrivate API is fully removed.
// https://crbug.com/593166
function EasyUnlockProximityRequired(prefKey, valueSchema, schema) {
  // Note: technically, extensions could intercept this through a setter on
  // Object.prototype(). We don't really care, because a) this is only for a
  // private API, so we shouldn't have to worry about untrusted code, and b)
  // this is an anonymous event, which exposes no attack surface and will be
  // exposed to the extension anyway.
  this.onChange = new Event();
};

exports.$set('EasyUnlockProximityRequired', EasyUnlockProximityRequired);
