// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

cr.define('cr', function() {
  function ErrorStore() {
    var self = this;
    window.addEventListener('error', function(e) {
      self.store_.push(e);
    });
  }

  cr.addSingletonGetter(ErrorStore);

  ErrorStore.prototype = {
    store_: [],

    get length() {
      return this.store_.length;
    },
  };

  return {
    ErrorStore: ErrorStore,
  };
});

cr.ErrorStore.getInstance();
