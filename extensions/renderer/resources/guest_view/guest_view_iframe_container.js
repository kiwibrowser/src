// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// --site-per-process overrides for guest_view_container.js

var GuestViewContainer = require('guestViewContainer').GuestViewContainer;
var IdGenerator = requireNative('id_generator');

GuestViewContainer.prototype.createInternalElement$ = function() {
  var iframeElement = document.createElement('iframe');
  iframeElement.style.width = '100%';
  iframeElement.style.height = '100%';
  iframeElement.style.border = '0';
  privates(iframeElement).internal = this;
  return iframeElement;
};

GuestViewContainer.prototype.prepareForReattach_ = function() {
  // Since attachment swaps a local frame for a remote frame, we need our
  // internal iframe element to be local again before we can reattach.
  var newFrame = this.createInternalElement$();
  var oldFrame = privates(this).internalElement;
  privates(this).internalElement = newFrame;
  oldFrame.parentNode.replaceChild(newFrame, oldFrame);
};

GuestViewContainer.prototype.attachWindow$ = function() {
  var generatedId = IdGenerator.GetNextId();
  // Generate an instance id for the container.
  this.onInternalInstanceId(generatedId);
  return true;
};

GuestViewContainer.prototype.willAttachElement = function () {
  if (this.deferredAttachCallback) {
    this.deferredAttachCallback();
    this.deferredAttachCallback = null;
  }
};
