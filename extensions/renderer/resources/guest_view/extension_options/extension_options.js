// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

var ExtensionOptionsConstants =
    require('extensionOptionsConstants').ExtensionOptionsConstants;
var ExtensionOptionsEvents =
    require('extensionOptionsEvents').ExtensionOptionsEvents;
var GuestViewContainer = require('guestViewContainer').GuestViewContainer;

function ExtensionOptionsImpl(extensionoptionsElement) {
  $Function.call(
      GuestViewContainer, this, extensionoptionsElement, 'extensionoptions');

  new ExtensionOptionsEvents(this);
};

ExtensionOptionsImpl.prototype.__proto__ = GuestViewContainer.prototype;

ExtensionOptionsImpl.VIEW_TYPE = 'ExtensionOptions';

ExtensionOptionsImpl.prototype.onElementAttached = function() {
  this.createGuest();
}

ExtensionOptionsImpl.prototype.buildContainerParams = function() {
  var params = {};
  for (var i in this.attributes) {
    params[i] = this.attributes[i].getValue();
  }
  return params;
};

ExtensionOptionsImpl.prototype.createGuest = function() {
  // Destroy the old guest if one exists.
  this.guest.destroy($Function.bind(this.prepareForReattach_, this));

  this.guest.create(this.buildParams(), $Function.bind(function() {
    if (!this.guest.getId()) {
      // Fire a createfailed event here rather than in ExtensionOptionsGuest
      // because the guest will not be created, and cannot fire an event.
      var createFailedEvent = new Event('createfailed', { bubbles: true });
      this.dispatchEvent(createFailedEvent);
    } else {
      this.attachWindow$();
    }
  }, this));
};

GuestViewContainer.registerElement(ExtensionOptionsImpl);

// Exports.
exports.$set('ExtensionOptionsImpl', ExtensionOptionsImpl);
