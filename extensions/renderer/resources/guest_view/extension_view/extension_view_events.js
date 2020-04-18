// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Event management for ExtensionView.

var CreateEvent = require('guestViewEvents').CreateEvent;
var GuestViewEvents = require('guestViewEvents').GuestViewEvents;

function ExtensionViewEvents(extensionViewImpl) {
  $Function.call(GuestViewEvents, this, extensionViewImpl);
}

ExtensionViewEvents.prototype.__proto__ = GuestViewEvents.prototype;

ExtensionViewEvents.EVENTS = {
  'loadcommit': {
    evt: CreateEvent('extensionViewInternal.onLoadCommit'),
    handler: 'handleLoadCommitEvent',
    internal: true
  }
};

ExtensionViewEvents.prototype.getEvents = function() {
  return ExtensionViewEvents.EVENTS;
};

ExtensionViewEvents.prototype.handleLoadCommitEvent = function(event) {
  this.view.onLoadCommit(event.url);
};

exports.$set('ExtensionViewEvents', ExtensionViewEvents);
