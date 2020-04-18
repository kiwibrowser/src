// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// This module implements the attributes of the <extensionoptions> tag.

var GuestViewAttributes = require('guestViewAttributes').GuestViewAttributes;
var ExtensionOptionsConstants =
    require('extensionOptionsConstants').ExtensionOptionsConstants;
var ExtensionOptionsImpl = require('extensionOptions').ExtensionOptionsImpl;

// -----------------------------------------------------------------------------
// ExtensionAttribute object.

// Attribute that handles extension binded to the extensionoptions.
function ExtensionAttribute(view) {
  $Function.call(
      GuestViewAttributes.Attribute, this,
      ExtensionOptionsConstants.ATTRIBUTE_EXTENSION, view);
}

ExtensionAttribute.prototype.__proto__ =
    GuestViewAttributes.Attribute.prototype;

ExtensionAttribute.prototype.handleMutation = function(oldValue, newValue) {
  // Once this attribute has been set, it cannot be unset.
  if (!newValue && oldValue) {
    this.setValueIgnoreMutation(oldValue);
    return;
  }

  if (!newValue || !this.elementAttached)
    return;

  this.view.createGuest();
};

// -----------------------------------------------------------------------------

// Sets up all of the extensionoptions attributes.
ExtensionOptionsImpl.prototype.setupAttributes = function() {
  this.attributes[ExtensionOptionsConstants.ATTRIBUTE_EXTENSION] =
      new ExtensionAttribute(this);
};
