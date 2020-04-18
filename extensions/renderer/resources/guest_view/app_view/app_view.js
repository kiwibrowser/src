// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

var DocumentNatives = requireNative('document_natives');
var GuestViewContainer = require('guestViewContainer').GuestViewContainer;
var IdGenerator = requireNative('id_generator');

function AppViewImpl(appviewElement) {
  $Function.call(GuestViewContainer, this, appviewElement, 'appview');

  this.app = '';
  this.data = '';
}

AppViewImpl.prototype.__proto__ = GuestViewContainer.prototype;

AppViewImpl.VIEW_TYPE = 'AppView';

// Add extra functionality to |this.element|.
AppViewImpl.setupElement = function(proto) {
  var apiMethods = [
    'connect'
  ];

  // Forward proto.foo* method calls to AppViewImpl.foo*.
  GuestViewContainer.forwardApiMethods(proto, apiMethods);
}

AppViewImpl.prototype.getErrorNode = function() {
  if (!this.errorNode) {
    this.errorNode = document.createElement('div');
    this.errorNode.innerText = 'Unable to connect to app.';
    this.errorNode.style.position = 'absolute';
    this.errorNode.style.left = '0px';
    this.errorNode.style.top = '0px';
    this.errorNode.style.width = '100%';
    this.errorNode.style.height = '100%';
    this.element.shadowRoot.appendChild(this.errorNode);
  }
  return this.errorNode;
};

AppViewImpl.prototype.buildContainerParams = function() {
  return {
    'appId': this.app,
    'data': this.data || {}
  };
};

AppViewImpl.prototype.connect = function(app, data, callback) {
  if (!this.elementAttached) {
    if (callback) {
      callback(false);
    }
    return;
  }

  this.app = app;
  this.data = data;

  this.guest.destroy($Function.bind(this.prepareForReattach_, this));
  this.guest.create(this.buildParams(), $Function.bind(function() {
    if (!this.guest.getId()) {
      var errorMsg = 'Unable to connect to app "' + app + '".';
      window.console.warn(errorMsg);
      this.getErrorNode().innerText = errorMsg;
      if (callback) {
        callback(false);
      }
      return;
    }
    this.attachWindow$();
    if (callback) {
      callback(true);
    }
  }, this));
};

GuestViewContainer.registerElement(AppViewImpl);
