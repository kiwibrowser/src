// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

function MockActionModel(title, entries) {
  this.title = title;
  this.entries = entries;
  this.actionsModel = null;
};

MockActionModel.prototype.getTitle = function() {
  return this.title;
};

MockActionModel.prototype.onCanExecute = function() {
};

MockActionModel.prototype.onExecute = function() {
  cr.ui.dispatchSimpleEvent('invalidated', this.actionsModel);
};

function MockActionsModel(actions) {
  this.actions_ = actions;
  Object.keys(actions).forEach(function(key) {
    actions[key].actionsModel = this;
  });
};

MockActionsModel.prototype = {
  __proto__: cr.EventTarget.prototype
};

MockActionsModel.prototype.initialize = function() {
  return new Promise.resolve();
};

MockActionsModel.prototype.getActions = function() {
  return this.actions_;
};
