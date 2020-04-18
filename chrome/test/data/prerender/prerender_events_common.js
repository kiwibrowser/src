// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Common js for prerender loaders; defines the helper functions that put
// event handlers on prerenders and track the events for browser tests.

// TODO(gavinp): Put more common loader logic in here.

// Currently only errors with the ordering of Prerender events are caught.
var hadPrerenderEventErrors = false;

var receivedPrerenderEvents = {
  'webkitprerenderstart': [],
  'webkitprerenderdomcontentloaded': [],
  'webkitprerenderload': [],
  'webkitprerenderstop': [],
}
// A list of callbacks to be called on every prerender event. Each callback
// returns true if it should never be called again, or false to remain in the
// list and be called on future events. These are used to implement
// WaitForPrerenderEventCount.
var prerenderEventCallbacks = [];

function GetPrerenderEventCount(index, type) {
  return receivedPrerenderEvents[type][index] || 0;
}

function PrerenderEventHandler(index, ev) {
  // Check for errors.
  if (ev.type == 'webkitprerenderstart') {
    // No event may preceed start.
    if (GetPrerenderEventCount(index, 'webkitprerenderstart') ||
        GetPrerenderEventCount(index, 'webkitprerenderdomcontentloaded') ||
        GetPrerenderEventCount(index, 'webkitprerenderload') ||
        GetPrerenderEventCount(index, 'webkitprerenderstop')) {
      hadPrerenderEventErrors = true;
    }
  } else {
    // There may be multiple load or domcontentloaded events, but they must not
    // come after start and must come before stop. And there may be at most one
    // start. Note that stop may be delivered without any load events.
    if (!GetPrerenderEventCount(index, 'webkitprerenderstart') ||
        GetPrerenderEventCount(index, 'webkitprerenderstop')) {
      hadPrerenderEventErrors = true;
    }
  }

  // Update count.
  receivedPrerenderEvents[ev.type][index] =
      (receivedPrerenderEvents[ev.type][index] || 0) + 1;

  // Run all callbacks. Remove the ones that are done.
  prerenderEventCallbacks = prerenderEventCallbacks.filter(function(callback) {
    return !callback();
  });
}

// Calls |callback| when at least |count| instances of event |type| have been
// observed for prerender |index|.
function WaitForPrerenderEventCount(index, type, count, callback) {
  var checkCount = function() {
    if (GetPrerenderEventCount(index, type) >= count) {
      callback();
      return true;
    }
    return false;
  };
  if (!checkCount())
    prerenderEventCallbacks.push(checkCount);
}

function AddEventHandlersToLinkElement(link, index) {
  link.addEventListener('webkitprerenderstart',
                        PrerenderEventHandler.bind(null, index), false);
  link.addEventListener('webkitprerenderdomcontentloaded',
                        PrerenderEventHandler.bind(null, index), false);
  link.addEventListener('webkitprerenderload',
                        PrerenderEventHandler.bind(null, index), false);
  link.addEventListener('webkitprerenderstop',
                        PrerenderEventHandler.bind(null, index), false);
}

function AddPrerender(url, index) {
  var link = document.createElement('link');
  link.id = 'prerenderElement' + index;
  link.rel = 'prerender';
  link.href = url;
  AddEventHandlersToLinkElement(link, index);
  document.body.appendChild(link);
  return link;
}

function RemoveLinkElement(index) {
  var link = document.getElementById('prerenderElement' + index);
  link.parentElement.removeChild(link);
}

function ExtractGetParameterBadlyAndInsecurely(param, defaultValue) {
  var re = RegExp('[&?]' + param + '=([^&?#]*)');
  var result = re.exec(document.location);
  if (result)
    return result[1];
  return defaultValue;
}

function AddAnchor(href, target) {
  var a = document.createElement('a');
  a.href = href;
  if (target)
    a.target = target;
  document.body.appendChild(a);
  return a;
}

function Click(url) {
  AddAnchor(url).dispatchEvent(new MouseEvent('click', {
    view: window,
    bubbles: true,
    cancelable: true,
    detail: 1
  }));
}

function ClickTarget(url) {
  var eventObject = new MouseEvent('click', {
    view: window,
    bubbles: true,
    cancelable: true,
    detail: 1
  });
  AddAnchor(url, '_blank').dispatchEvent(eventObject);
}

function ClickPing(url, pingUrl) {
  var a = AddAnchor(url);
  a.ping = pingUrl;
  a.dispatchEvent(new MouseEvent('click', {
    view: window,
    bubbles: true,
    cancelable: true,
    detail: 1
  }));
}

function ShiftClick(url) {
  AddAnchor(url).dispatchEvent(new MouseEvent('click', {
    view: window,
    bubbles: true,
    cancelable: true,
    detail: 1,
    shiftKey: true
  }));
}

function CtrlClick(url) {
  AddAnchor(url).dispatchEvent(new MouseEvent('click', {
    view: window,
    bubbles: true,
    cancelable: true,
    detail: 1,
    ctrlKey: true
  }));
}

function CtrlShiftClick(url) {
  AddAnchor(url).dispatchEvent(new MouseEvent('click', {
    view: window,
    bubbles: true,
    cancelable: true,
    detail: 1,
    ctrlKey: true,
    shiftKey: true
  }));
}

function MetaClick(url) {
  AddAnchor(url).dispatchEvent(new MouseEvent('click', {
    view: window,
    bubbles: true,
    cancelable: true,
    detail: 1,
    metaKey: true
  }));
}

function MetaShiftClick(url) {
  AddAnchor(url).dispatchEvent(new MouseEvent('click', {
    view: window,
    bubbles: true,
    cancelable: true,
    detail: 1,
    metaKey: true,
    shiftKey: true
  }));
}

function WindowOpen(url) {
  window.open(url);
}
