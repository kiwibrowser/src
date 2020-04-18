// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

this.onsync = function(event) {
    if (!('waitUntil' in event)) {
      console.log("waitUntil missing on event");
      return;
    }
    event.waitUntil(new Promise(function(resolve, reject) {
        setTimeout(function() {
          console.log('Fulfilling onsync event');
          resolve();
        }, 5);
    }));
};
