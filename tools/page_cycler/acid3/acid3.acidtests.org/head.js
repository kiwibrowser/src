// Copyright (c) 2006-2009 The Chromium Authors. All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
//    * Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
//    * Redistributions in binary form must reproduce the above
// copyright notice, this list of conditions and the following disclaimer
// in the documentation and/or other materials provided with the
// distribution.
//    * Neither the name of Google Inc. nor the names of its
// contributors may be used to endorse or promote products derived from
// this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

var __c = ""; // that's good enough for me.
var __td;
var __tf;
var __tl;
var __iterations;
var __cycle;
var __results = false;
var __page;
var __TIMEOUT = 15;
function __get_cookie(name) {
  var cookies = document.cookie.split("; ");
  for (var i = 0; i < cookies.length; ++i) {
    var t = cookies[i].split("=");
    if (t[0] == name && t[1])
      return t[1];
  }
  return "";
}
function __pages() {  // fetch lazily
  if (!("data" in this))
    this.data = __get_cookie("__pc_pages").split(",");
  return this.data;
}
function __get_timings() {
  return __get_cookie("__pc_timings");
}
function __set_timings(timings) {
  document.cookie = "__pc_timings=" + timings + "; path=/";
}
function __ontimeout() {
  var doc;

  // Call GC twice to cleanup JS heap before starting a new test.
  if (window.gc) {
    window.gc();
    window.gc();
  }

  var ts = (new Date()).getTime();
  var tlag = (ts - __te) - __TIMEOUT;
  if (tlag > 0)
    __tf = __tf + tlag;
  if (__cycle == (__pages().length * __iterations)) {
    document.cookie = "__pc_done=1; path=/";
    doc = "../../common/report.html";
  } else {
    doc = "../" + __pages()[__page] + "/index.html"
  }
  
  var timings = __tl;
  var oldTimings = __get_timings();
  if (oldTimings != "") { 
    timings = oldTimings + "," + timings;
  }
  __set_timings(timings);
  
  var url = doc + "?n=" + __iterations + "&i=" + __cycle + "&p=" + __page + "&ts=" + ts + "&td=" + __td + "&tf=" + __tf;
  document.location.href = url;
}

function test_complete(errors, elapsed_time) {
  if (__results)
    return;
  var unused = document.body.offsetHeight;  // force layout

  var ts = 0, td = 0, te = (new Date()).getTime(), tf = 0;

  var s = document.location.search;
  if (s) {
    var params = s.substring(1).split('&');
    for (var i = 0; i < params.length; ++i) {
      var f = params[i].split('=');
      switch (f[0]) {
      case 'skip':
        // No calculation, just viewing
        return;
      case 'n':
        __iterations = f[1];
        break;
      case 'i':
        __cycle = (f[1] - 0) + 1;
        break;
      case 'p':
        __page = ((f[1] - 0) + 1) % __pages().length;
        break;
      case 'ts':
        ts = (f[1] - 0);
        break;
      case 'td':
        td = (f[1] - 0);
        break;
      case 'tf':
        tf = (f[1] - 0);
        break;
      }
    }
  }
  __tl = (te - ts);
  __td = td + __tl;
  __te = te;
  __tf = tf;  // record t-fudge

  setTimeout("__ontimeout()", __TIMEOUT);
}

/*
if (window.attachEvent)
  window.attachEvent("onload", __onload);
else
  addEventListener("load", __onload, false);
  */
