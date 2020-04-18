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
goog.provide('i18n.input.chrome.inputview.handler.PointerHandler');

goog.require('goog.Timer');
goog.require('goog.events.EventHandler');
goog.require('goog.events.EventTarget');
goog.require('goog.events.EventType');
goog.require('i18n.input.chrome.inputview.handler.PointerActionBundle');
goog.require('i18n.input.chrome.inputview.handler.Util');

goog.scope(function() {



/**
 * The pointer controller.
 *
 * @param {!Element=} opt_target .
 * @constructor
 * @extends {goog.events.EventTarget}
 */
i18n.input.chrome.inputview.handler.PointerHandler = function(opt_target) {
  goog.base(this);

  /**
   * The pointer handlers.
   *
   * @type {!Object.<number, !i18n.input.chrome.inputview.handler.PointerActionBundle>}
   * @private
   */
  this.pointerActionBundles_ = {};

  /**
   * The event handler.
   *
   * @type {!goog.events.EventHandler}
   * @private
   */
  this.eventHandler_ = new goog.events.EventHandler(this);

  var target = opt_target || document;
  this.eventHandler_.
      listen(target, [goog.events.EventType.MOUSEDOWN,
          goog.events.EventType.TOUCHSTART], this.onPointerDown_, true).
      listen(document, goog.events.EventType.TOUCHEND, this.onPointerUp_, true).
      listen(document, goog.events.EventType.MOUSEUP, this.onPointerUp_, true).
      listen(target, [goog.events.EventType.TOUCHMOVE,
          goog.events.EventType.MOUSEMOVE], this.onPointerMove_,
          true);
};
goog.inherits(i18n.input.chrome.inputview.handler.PointerHandler,
    goog.events.EventTarget);
var PointerHandler = i18n.input.chrome.inputview.handler.PointerHandler;
var PointerActionBundle =
    i18n.input.chrome.inputview.handler.PointerActionBundle;
var Util = i18n.input.chrome.inputview.handler.Util;


/**
 * The canvas class name.
 * @const {string}
 * @private
 */
PointerHandler.CANVAS_CLASS_NAME_ = 'ita-hwt-canvas';


/**
 * Mouse down tick, which is for delayed pointer up for tap action on touchpad.
 *
 * @private {Date}
 */
PointerHandler.prototype.mouseDownTick_ = null;


/**
 * Event handler for previous mousedown or touchstart target.
 *
 * @private {i18n.input.chrome.inputview.handler.PointerActionBundle}
 */
PointerHandler.prototype.previousPointerActionBundle_ = null;


/**
 * Pointer action bundle for mouse down.
 * This is used in mouse up handler because mouse up event may have different
 * target than the mouse down event.
 *
 * @private {i18n.input.chrome.inputview.handler.PointerActionBundle}
 */
PointerHandler.prototype.pointerActionBundleForMouseDown_ = null;


/**
 * Gets the pointer handler for |view|. If not exists, creates a new one.
 *
 * @param {!i18n.input.chrome.inputview.elements.Element} view .
 * @return {!i18n.input.chrome.inputview.handler.PointerActionBundle} .
 * @private
 */
PointerHandler.prototype.getPointerActionBundle_ = function(view) {
  var uid = goog.getUid(view);
  if (!this.pointerActionBundles_[uid]) {
    this.pointerActionBundles_[uid] = new i18n.input.chrome.inputview.handler.
        PointerActionBundle(view, this);
  }
  return this.pointerActionBundles_[uid];
};


/**
 * Callback for mouse/touch down on the target.
 *
 * @param {!goog.events.BrowserEvent} e The event.
 * @private
 */
PointerHandler.prototype.onPointerDown_ = function(e) {
  var view = Util.getView(/** @type {!Node} */ (e.target));
  if (!view) {
    return;
  }
  var pointerActionBundle = this.getPointerActionBundle_(view);
  if (this.previousPointerActionBundle_ &&
      this.previousPointerActionBundle_ != pointerActionBundle) {
    this.previousPointerActionBundle_.cancelDoubleClick();
  }
  this.previousPointerActionBundle_ = pointerActionBundle;
  pointerActionBundle.handlePointerDown(e);
  if (view.pointerConfig.preventDefault) {
    e.preventDefault();
  }
  if (view.pointerConfig.stopEventPropagation) {
    e.stopPropagation();
  }
  if (e.type == goog.events.EventType.MOUSEDOWN) {
    this.mouseDownTick_ = new Date();
    this.pointerActionBundleForMouseDown_ = pointerActionBundle;
  }
};


/**
 * Callback for pointer out.
 *
 * @param {!goog.events.BrowserEvent} e The event.
 * @private
 */
PointerHandler.prototype.onPointerUp_ = function(e) {
  var pointerActionBundle;
  if (e.type == goog.events.EventType.MOUSEUP) {
    // If mouseup happens too fast after mousedown, it may be a tap action on
    // touchpad, so delay the pointer up action so user can see the visual
    // flash.
    if (this.mouseDownTick_ && new Date() - this.mouseDownTick_ < 10) {
      goog.Timer.callOnce(this.onPointerUp_.bind(this, e), 50);
      return;
    }
    if (this.pointerActionBundleForMouseDown_) {
      this.pointerActionBundleForMouseDown_.handlePointerUp(e);
      pointerActionBundle = this.pointerActionBundleForMouseDown_;
      this.pointerActionBundleForMouseDown_ = null;
    }
  } else {
    var view = Util.getView(/** @type {!Node} */ (e.target));
    if (!view) {
      return;
    }
    pointerActionBundle = this.pointerActionBundles_[goog.getUid(view)];
    if (pointerActionBundle) {
      pointerActionBundle.handlePointerUp(e);
    }
    e.preventDefault();
  }
  if (pointerActionBundle && pointerActionBundle.view &&
      pointerActionBundle.view.pointerConfig.stopEventPropagation) {
    e.stopPropagation();
  }
};


/**
 * Callback for touchmove or mousemove.
 *
 * @param {!goog.events.BrowserEvent} e The event.
 * @private
 */
PointerHandler.prototype.onPointerMove_ = function(e) {
  if (e.type == goog.events.EventType.MOUSEMOVE) {
    if (this.pointerActionBundleForMouseDown_) {
      this.pointerActionBundleForMouseDown_.handlePointerMove(
          /** @type {!Event} */ (e.getBrowserEvent()));
    }
    return;
  }
  var touches = e.getBrowserEvent()['touches'];
  if (!touches || touches.length == 0) {
    return;
  }
  if (touches.length > 1) {
    e.preventDefault();
  }
  var shouldPreventDefault = false;
  var shouldStopEventPropagation = false;
  for (var i = 0; i < touches.length; i++) {
    var view = Util.getView(/** @type {!Node} */ (touches[i].target));
    if (view) {
      var pointerActionBundle = this.pointerActionBundles_[goog.getUid(view)];
      if (pointerActionBundle) {
        pointerActionBundle.handlePointerMove(touches[i]);
      }
      if (view.pointerConfig.preventDefault) {
        shouldPreventDefault = true;
      }
      if (view.pointerConfig.stopEventPropagation) {
        shouldStopEventPropagation = true;
      }
    }
  }
  if (shouldPreventDefault) {
    e.preventDefault();
  }
  if (shouldStopEventPropagation) {
    e.stopPropagation();
  }
};


/** @override */
PointerHandler.prototype.disposeInternal = function() {
  for (var bundle in this.pointerActionBundles_) {
    goog.dispose(bundle);
  }
  goog.dispose(this.eventHandler_);

  goog.base(this, 'disposeInternal');
};

});  // goog.scope
