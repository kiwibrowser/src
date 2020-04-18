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
goog.provide('i18n.input.chrome.inputview.elements.content.SoftKey');

goog.require('goog.dom.classlist');
goog.require('i18n.input.chrome.inputview.Css');
goog.require('i18n.input.chrome.inputview.elements.Element');
goog.require('i18n.input.chrome.inputview.util');



goog.scope(function() {



/**
 * The base soft key class.
 *
 * @param {string} id The id.
 * @param {!i18n.input.chrome.ElementType} type The element
 *     type.
 * @param {goog.events.EventTarget=} opt_eventTarget The event target.
 * @constructor
 * @extends {i18n.input.chrome.inputview.elements.Element}
 */
i18n.input.chrome.inputview.elements.content.SoftKey = function(id, type,
    opt_eventTarget) {
  goog.base(this, id, type, opt_eventTarget);

  /**
   * The available width.
   *
   * @type {number}
   */
  this.availableWidth = 0;

  /**
   * The available height.
   *
   * @type {number}
   */
  this.availableHeight = 0;

  /**
   * The nearby keys.
   *
   * @type {!Array.<!SoftKey>}
   */
  this.nearbyKeys = [];
};
goog.inherits(i18n.input.chrome.inputview.elements.content.SoftKey,
    i18n.input.chrome.inputview.elements.Element);
var SoftKey = i18n.input.chrome.inputview.elements.content.SoftKey;


/**
 * The coordinate of the center point.
 *
 * @type {!goog.math.Coordinate}
 */
SoftKey.prototype.centerCoordinate;


/**
 * The coordinate of the top-left point.
 *
 * @type {!goog.math.Coordinate}
 */
SoftKey.prototype.topLeftCoordinate;


/**
 * The gaussian estimator.
 *
 * @type {!i18n.input.chrome.inputview.elements.content.GaussianEstimator}
 */
SoftKey.prototype.estimator;


/** @override */
SoftKey.prototype.createDom = function() {
  goog.base(this, 'createDom');

  goog.dom.classlist.add(this.getElement(),
      i18n.input.chrome.inputview.Css.SOFT_KEY);
};


/** @override */
SoftKey.prototype.resize = function(width, height) {
  goog.base(this, 'resize', width, height);

  var elem = this.getElement();
  var borderWidth = i18n.input.chrome.inputview.util.getPropertyValue(
      elem, 'borderWidth');
  var marginTop = i18n.input.chrome.inputview.util.getPropertyValue(
      elem, 'marginTop');
  var marginBottom = i18n.input.chrome.inputview.util.getPropertyValue(
      elem, 'marginBottom');
  var marginLeft = i18n.input.chrome.inputview.util.getPropertyValue(
      elem, 'marginLeft');
  var marginRight = i18n.input.chrome.inputview.util.getPropertyValue(
      elem, 'marginRight');
  var w = width - borderWidth * 2 - marginLeft - marginRight;
  var h = height - borderWidth * 2 - marginTop - marginBottom;
  elem.style.width = w + 'px';
  elem.style.height = h + 'px';

  this.availableWidth = w;
  this.availableHeight = h;
};

});  // goog.scope
