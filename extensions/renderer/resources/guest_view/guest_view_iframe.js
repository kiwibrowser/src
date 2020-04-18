// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// --site-per-process overrides for guest_view.js.

var GuestView = require('guestView').GuestView;
var GuestViewImpl = require('guestView').GuestViewImpl;
var GuestViewInternalNatives = requireNative('guest_view_internal');
var ResizeEvent = require('guestView').ResizeEvent;

var getIframeContentWindow = function(viewInstanceId) {
  var view = GuestViewInternalNatives.GetViewFromID(viewInstanceId);
  if (!view)
    return null;

  var internalIframeElement = privates(view).internalElement;
  if (internalIframeElement)
    return internalIframeElement.contentWindow;

  return null;
};

// Internal implementation of attach().
GuestViewImpl.prototype.attachImpl$ = function(
    internalInstanceId, viewInstanceId, attachParams, callback) {
  var view = GuestViewInternalNatives.GetViewFromID(viewInstanceId);
  if (!view.elementAttached) {
    // Defer the attachment until the <webview> element is attached.
    view.deferredAttachCallback = $Function.bind(this.attachImpl$,
        this, internalInstanceId, viewInstanceId, attachParams, callback);
    return;
  };

  // Check the current state.
  if (!this.checkState('attach')) {
    this.handleCallback(callback);
    return;
  }

  // Callback wrapper function to set the contentWindow following attachment,
  // and advance the queue.
  var callbackWrapper = function(callback) {
    var contentWindow = getIframeContentWindow(viewInstanceId);
    // Check if attaching failed.
    if (!contentWindow) {
      this.state = GuestViewImpl.GuestState.GUEST_STATE_CREATED;
      this.internalInstanceId = 0;
    } else {
      // Only update the contentWindow if attaching is successful.
      this.contentWindow = contentWindow;
    }

    this.handleCallback(callback);
  };

  attachParams['instanceId'] = viewInstanceId;
  var contentWindow = getIframeContentWindow(viewInstanceId);
  // |contentWindow| is used to retrieve the RenderFrame in cpp.
  GuestViewInternalNatives.AttachIframeGuest(
      internalInstanceId, this.id, attachParams, contentWindow,
      $Function.bind(callbackWrapper, this, callback));

  this.internalInstanceId = internalInstanceId;
  this.state = GuestViewImpl.GuestState.GUEST_STATE_ATTACHED;

  // Detach automatically when the container is destroyed.
  GuestViewInternalNatives.RegisterDestructionCallback(
      internalInstanceId, this.weakWrapper(function() {
    if (this.state != GuestViewImpl.GuestState.GUEST_STATE_ATTACHED ||
        this.internalInstanceId != internalInstanceId) {
      return;
    }

    this.internalInstanceId = 0;
    this.state = GuestViewImpl.GuestState.GUEST_STATE_CREATED;
  }, viewInstanceId));
};

// Internal implementation of create().
GuestViewImpl.prototype.createImpl$ = function(createParams, callback) {
  // Check the current state.
  if (!this.checkState('create')) {
    this.handleCallback(callback);
    return;
  }

  // Callback wrapper function to store the guestInstanceId from the
  // createGuest() callback, handle potential creation failure, and advance the
  // queue.
  var callbackWrapper = function(callback, guestInfo) {
    this.id = guestInfo.id;

    // Check if creation failed.
    if (this.id === 0) {
      this.state = GuestViewImpl.GuestState.GUEST_STATE_START;
      this.contentWindow = null;
    }

    ResizeEvent.addListener(this.callOnResize, {instanceId: this.id});
    this.handleCallback(callback);
  };

  this.sendCreateRequest(
      createParams, $Function.bind(callbackWrapper, this, callback));

  this.state = GuestViewImpl.GuestState.GUEST_STATE_CREATED;
};

// Internal implementation of destroy().
GuestViewImpl.prototype.destroyImpl = function(callback) {
  // Check the current state.
  if (!this.checkState('destroy')) {
    this.handleCallback(callback);
    return;
  }

  if (this.state == GuestViewImpl.GuestState.GUEST_STATE_START) {
    // destroy() does nothing in this case.
    this.handleCallback(callback);
    return;
  }

  // Reset the state of the destroyed guest;
  this.contentWindow = null;
  this.id = 0;
  this.internalInstanceId = 0;
  this.state = GuestViewImpl.GuestState.GUEST_STATE_START;
  if (ResizeEvent.hasListener(this.callOnResize)) {
    ResizeEvent.removeListener(this.callOnResize);
  }

  // Handle callback at end to avoid handling items in the action queue out of
  // order, since the callback is run synchronously here.
  this.handleCallback(callback);
};
