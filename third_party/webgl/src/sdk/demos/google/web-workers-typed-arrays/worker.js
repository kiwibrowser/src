/*
 * Copyright (c) 2012 The Chromium Authors. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *    * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *    * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *    * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

self.importScripts('../nvidia-vertex-buffer-object/PeriodicIterator.js');
self.importScripts('calculate.js');
self.importScripts('capabilities.js');

// utility function to pass strings back to be logged
function log(string, conf) {
  postMessage({type: 'console', sender: conf.slab, data: string});
}

function result(conf) {
  // use webkitPostMessage if its available
  self.postMessage = self.webkitPostMessage || self.postMessage;
  if(conf.config.useTransferables) {
    // We use the new webkitPostMessage here with the transfer argument to pass
    // back the clientArray's buffer (it'll get re-wrapped to a typed array on
    // the other side)
    self.postMessage({type: 'result', sender: conf.slab, data: conf},
                [conf.clientArray.buffer]);
  } else {
    self.postMessage({type: 'result', sender: conf.slab, data: conf});
  }
}

var precalc;
var slabData;

self.onmessage = function(event) {
  if (event.data.id == "precalc") {
      precalc = event.data.precalc;
      return;
  } else if (event.data.id == "slabData") {
      slabData = event.data.slabData;
      return;
  }
  var conf = event.data;
  calculate(conf, precalc, slabData);

  result(conf);
};


