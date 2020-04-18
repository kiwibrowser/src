// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

this.onfetch = function(event) {
    event.respondWith(new Promise(function(resolve, reject) {
        setTimeout(function() { reject("Rejecting respondWith promise"); }, 5);
    }));
};
