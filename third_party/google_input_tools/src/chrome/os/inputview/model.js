// Copyright 2014 The ChromeOS IME Authors. All Rights Reserved.
// limitations under the License.
// See the License for the specific language governing permissions and
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// distributed under the License is distributed on an "AS-IS" BASIS,
// Unless required by applicable law or agreed to in writing, software
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// You may obtain a copy of the License at
// you may not use this file except in compliance with the License.
// Licensed under the Apache License, Version 2.0 (the "License");
//
goog.provide('i18n.input.chrome.inputview.Model');

goog.require('goog.array');
goog.require('goog.events.EventTarget');
goog.require('goog.net.jsloader');
goog.require('i18n.input.chrome.inputview.ConditionName');
goog.require('i18n.input.chrome.inputview.Settings');
goog.require('i18n.input.chrome.inputview.SpecNodeName');
goog.require('i18n.input.chrome.inputview.StateManager');
goog.require('i18n.input.chrome.inputview.events.ConfigLoadedEvent');
goog.require('i18n.input.chrome.inputview.events.LayoutLoadedEvent');

goog.scope(function() {
var SpecNodeName = i18n.input.chrome.inputview.SpecNodeName;



/**
 * The model.
 * @constructor
 * @extends {goog.events.EventTarget}
 */
i18n.input.chrome.inputview.Model = function() {
  goog.base(this);

  /**
   * The state manager.
   *
   * @type {!i18n.input.chrome.inputview.StateManager}
   */
  this.stateManager = new i18n.input.chrome.inputview.StateManager();

  /**
   * The configuration.
   *
   * @type {!i18n.input.chrome.inputview.Settings}
   */
  this.settings = new i18n.input.chrome.inputview.Settings();

  /** @private {!Array.<string>} */
  this.loadingResources_ = [];

  goog.exportSymbol('google.ime.chrome.inputview.onLayoutLoaded',
      goog.bind(this.onLayoutLoaded_, this));
  goog.exportSymbol('google.ime.chrome.inputview.onConfigLoaded',
      goog.bind(this.onConfigLoaded_, this));
};
var Model = i18n.input.chrome.inputview.Model;
goog.inherits(Model, goog.events.EventTarget);


/**
 * The path to the layouts directory.
 *
 * @type {string}
 * @private
 */
Model.LAYOUTS_PATH_ =
    '/inputview_layouts/';


/**
 * The path to the content directory.
 *
 * @type {string}
 * @private
 */
Model.CONTENT_PATH_ =
    '/config/';


/**
 * Callback when configuration is loaded.
 *
 * @param {!Object} data The configuration data.
 * @private
 */
Model.prototype.onConfigLoaded_ = function(data) {
  goog.array.remove(this.loadingResources_, this.getConfigUrl_(
      data[SpecNodeName.ID]));
  this.dispatchEvent(new i18n.input.chrome.inputview.events.ConfigLoadedEvent(
      data));
};


/**
 * Gets the layout url.
 *
 * @param {string} layout .
 * @private
 * @return {string} The url of the layout data.
 */
Model.prototype.getLayoutUrl_ = function(layout) {
  return Model.LAYOUTS_PATH_ + layout + '.js';
};


/**
 * Gets the keyset configuration url.
 *
 * @param {string} keyset .
 * @private
 * @return {string} .
 */
Model.prototype.getConfigUrl_ = function(keyset) {
  // Strips out all the suffixes in the keyboard code.
  var configId = keyset.replace(/\..*$/, '');
  return Model.CONTENT_PATH_ + configId + '.js';
};


/**
 * Callback when layout is loaded.
 *
 * @param {!Object} data The layout data.
 * @private
 */
Model.prototype.onLayoutLoaded_ = function(data) {
  goog.array.remove(this.loadingResources_, this.getLayoutUrl_(data[
      SpecNodeName.LAYOUT_ID]));
  this.dispatchEvent(new i18n.input.chrome.inputview.events.LayoutLoadedEvent(
      data));
};


/**
 * Loads a layout.
 *
 * @param {string} layout The layout name.
 */
Model.prototype.loadLayout = function(layout) {
  var url = this.getLayoutUrl_(layout);
  if (!goog.array.contains(this.loadingResources_, url)) {
    this.loadingResources_.push(url);
    goog.net.jsloader.load(url);
  }
};


/**
 * Loads the configuration for the keyboard code.
 *
 * @param {string} keyboardCode The keyboard code.
 */
Model.prototype.loadConfig = function(keyboardCode) {
  var url = this.getConfigUrl_(keyboardCode);
  if (!goog.array.contains(this.loadingResources_, url)) {
    this.loadingResources_.push(url);
    goog.net.jsloader.load(url);
  }
};

});  // goog.scope
