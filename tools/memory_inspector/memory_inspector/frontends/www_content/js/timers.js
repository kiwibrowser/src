// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

timers = new (function() {

this.timers_ = {};

this.start = function(name, callback, intervalSeconds) {
  this.stop(name);

  var timerId = setInterval(callback, intervalSeconds * 1000);

  this.timers_[name] = {
    name: name,
    callback: callback,
    timerId: timerId,
    intervalSeconds: intervalSeconds
  };

  callback();
};

this.stop = function(name) {
  if (name in this.timers_) {
    clearInterval(this.timers_[name].timerId);
    delete this.timers_[name];
  }
};

this.stopAll = function() {
  for (var name in this.timers_) {
    clearInterval(this.timers_[name].timerId);
  }
  this.timers_ = {};
};

})();