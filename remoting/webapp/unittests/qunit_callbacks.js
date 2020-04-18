// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

(function() {

'use strict';

QUnit.testStart(function(){
  chromeMocks.activate();
});

QUnit.testDone(function(){
  chromeMocks.restore();
});

})();
