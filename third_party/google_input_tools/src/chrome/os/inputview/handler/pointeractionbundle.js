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
goog.provide('i18n.input.chrome.inputview.handler.PointerActionBundle');

goog.require('goog.Timer');
goog.require('goog.events.EventTarget');
goog.require('goog.events.EventType');
goog.require('goog.math.Coordinate');
goog.require('i18n.input.chrome.inputview.SwipeDirection');
goog.require('i18n.input.chrome.inputview.events.DragEvent');
goog.require('i18n.input.chrome.inputview.events.EventType');
goog.require('i18n.input.chrome.inputview.events.PointerEvent');
goog.require('i18n.input.chrome.inputview.events.SwipeEvent');
goog.require('i18n.input.chrome.inputview.handler.SwipeState');
goog.require('i18n.input.chrome.inputview.handler.Util');



goog.scope(function() {



/**
 * The handler for long press.
 *
 * @param {!i18n.input.chrome.inputview.elements.Element} view The view for this
 *     pointer event.
 * @param {goog.events.EventTarget=} opt_parentEventTarget The parent event
 *     target.
 * @constructor
 * @extends {goog.events.EventTarget}
 */
i18n.input.chrome.inputview.handler.PointerActionBundle = function(view,
    opt_parentEventTarget) {
  goog.base(this);
  this.setParentEventTarget(opt_parentEventTarget || null);

  /**
   * The target.
   *
   * @type {!i18n.input.chrome.inputview.elements.Element}
   */
  this.view = view;

  /**
   * The swipe offset.
   *
   * @type {!i18n.input.chrome.inputview.handler.SwipeState}
   * @private
   */
  this.swipeState_ = new i18n.input.chrome.inputview.handler.SwipeState();
};
goog.inherits(i18n.input.chrome.inputview.handler.PointerActionBundle,
    goog.events.EventTarget);
var PointerActionBundle = i18n.input.chrome.inputview.handler.
    PointerActionBundle;
var Util = i18n.input.chrome.inputview.handler.Util;


/**
 * The current target after the touch point is moved.
 *
 * @type {!Node | Element}
 * @private
 */
PointerActionBundle.prototype.currentTarget_;


/**
 * How many milli-seconds to evaluate a double click event.
 *
 * @type {number}
 * @private
 */
PointerActionBundle.DOUBLE_CLICK_INTERVAL_ = 500;


/**
 * The timer ID.
 *
 * @type {number}
 * @private
 */
PointerActionBundle.prototype.longPressTimer_;


/**
 * The minimum swipe distance.
 *
 * @type {number}
 * @private
 */
PointerActionBundle.MINIMUM_SWIPE_DISTANCE_ = 20;


/**
 * The timestamp of the pointer down.
 *
 * @type {number}
 * @private
 */
PointerActionBundle.prototype.pointerDownTimeStamp_ = 0;


/**
 * The timestamp of the pointer up.
 *
 * @type {number}
 * @private
 */
PointerActionBundle.prototype.pointerUpTimeStamp_ = 0;


/**
 * True if it is double clicking.
 *
 * @type {boolean}
 * @private
 */
PointerActionBundle.prototype.isDBLClicking_ = false;


/**
 * True if it is long pressing.
 *
 * @type {boolean}
 * @private
 */
PointerActionBundle.prototype.isLongPressing_ = false;


/**
 * True if it is flickering.
 *
 * @type {boolean}
 * @private
 */
PointerActionBundle.prototype.isFlickering_ = false;


/**
 * Handles touchmove event for one target.
 *
 * @param {!Touch | !Event} e .
 */
PointerActionBundle.prototype.handlePointerMove = function(e) {
  var identifier = Util.getEventIdentifier(e);
  var direction = 0;
  var deltaX = this.swipeState_.previousX == 0 ? 0 : (e.pageX -
      this.swipeState_.previousX);
  var deltaY = this.swipeState_.previousY == 0 ? 0 :
      (e.pageY - this.swipeState_.previousY);
  this.swipeState_.offsetX += deltaX;
  this.swipeState_.offsetY += deltaY;
  this.dispatchEvent(new i18n.input.chrome.inputview.events.DragEvent(
      this.view, direction, /** @type {!Node} */ (e.target),
      e.pageX, e.pageY, deltaX, deltaY, identifier));

  var minimumSwipeDist = PointerActionBundle.
      MINIMUM_SWIPE_DISTANCE_;

  if (this.swipeState_.offsetX > minimumSwipeDist) {
    direction |= i18n.input.chrome.inputview.SwipeDirection.RIGHT;
    this.swipeState_.offsetX = 0;
  } else if (this.swipeState_.offsetX < -minimumSwipeDist) {
    direction |= i18n.input.chrome.inputview.SwipeDirection.LEFT;
    this.swipeState_.offsetX = 0;
  }

  if (Math.abs(deltaY) > Math.abs(deltaX)) {
    if (this.swipeState_.offsetY > minimumSwipeDist) {
      direction |= i18n.input.chrome.inputview.SwipeDirection.DOWN;
      this.swipeState_.offsetY = 0;
    } else if (this.swipeState_.offsetY < -minimumSwipeDist) {
      direction |= i18n.input.chrome.inputview.SwipeDirection.UP;
      this.swipeState_.offsetY = 0;
    }
  }

  this.swipeState_.previousX = e.pageX;
  this.swipeState_.previousY = e.pageY;

  if (direction > 0) {
    // If there is any movement, cancel the longpress timer.
    goog.Timer.clear(this.longPressTimer_);
    this.dispatchEvent(new i18n.input.chrome.inputview.events.SwipeEvent(
        this.view, direction, /** @type {!Node} */ (e.target),
        e.pageX, e.pageY, identifier));
    var currentTargetView = Util.getView(this.currentTarget_);
    this.isFlickering_ = !this.isLongPressing_ && !!(this.view.pointerConfig.
        flickerDirection & direction) && currentTargetView == this.view;
  }

  this.maybeSwitchTarget_(
      new goog.math.Coordinate(e.pageX, e.pageY), identifier);
};


/**
 * If the target is switched to a new one, sends out a pointer_over for the new
 * target and sends out a pointer_out for the old target.
 *
 * @param {!goog.math.Coordinate} pageOffset .
 * @param {number} identifier .
 * @private
 */
PointerActionBundle.prototype.maybeSwitchTarget_ = function(pageOffset,
    identifier) {
  if (!this.isFlickering_) {
    var actualTarget = document.elementFromPoint(pageOffset.x, pageOffset.y);
    var currentTargetView = Util.getView(this.currentTarget_);
    var actualTargetView = Util.getView(actualTarget);
    if (currentTargetView != actualTargetView) {
      if (currentTargetView) {
        this.dispatchEvent(new i18n.input.chrome.inputview.events.PointerEvent(
            currentTargetView,
            i18n.input.chrome.inputview.events.EventType.POINTER_OUT,
            this.currentTarget_, pageOffset.x, pageOffset.y, identifier));
      }
      if (actualTargetView) {
        this.dispatchEvent(new i18n.input.chrome.inputview.events.PointerEvent(
            actualTargetView,
            i18n.input.chrome.inputview.events.EventType.POINTER_OVER,
            actualTarget, pageOffset.x, pageOffset.y, identifier));
      }
      this.currentTarget_ = actualTarget;
    }
  }
};


/**
 * Handles pointer up, e.g., mouseup/touchend.
 *
 * @param {!goog.events.BrowserEvent} e The event.
 */
PointerActionBundle.prototype.handlePointerUp = function(e) {
  goog.Timer.clear(this.longPressTimer_);
  var pageOffset = this.getPageOffset_(e);
  var identifier = Util.getEventIdentifier(e);
  this.dispatchEvent(new i18n.input.chrome.inputview.events.PointerEvent(
      this.view, i18n.input.chrome.inputview.events.EventType.LONG_PRESS_END,
      e.target, pageOffset.x, pageOffset.y, identifier));
  if (this.isDBLClicking_) {
    this.dispatchEvent(new i18n.input.chrome.inputview.events.PointerEvent(
        this.view, i18n.input.chrome.inputview.events.EventType.
        DOUBLE_CLICK_END, e.target, pageOffset.x, pageOffset.y, identifier));
  } else if (!(this.isLongPressing_ && this.view.pointerConfig.
      longPressWithoutPointerUp)) {
    this.maybeSwitchTarget_(pageOffset, identifier);
    var view = Util.getView(this.currentTarget_);
    var target = this.currentTarget_;
    if (this.isFlickering_) {
      view = this.view;
      target = e.target;
    }
    this.pointerUpTimeStamp_ = new Date().getTime();
    // Note |view| can be null if the finger moves outside of keyboard window
    // area. This is possible when user try to select an accent character
    // which is displayed outside of keyboard window. We need to dispatch a
    // POINTER_UP event to keyboard to commit the selected accent character.
    this.dispatchEvent(new i18n.input.chrome.inputview.events.PointerEvent(
        view, i18n.input.chrome.inputview.events.EventType.POINTER_UP,
        target, pageOffset.x, pageOffset.y, identifier,
        this.pointerUpTimeStamp_));
  }
  if (Util.getView(this.currentTarget_) == this.view) {
    this.dispatchEvent(new i18n.input.chrome.inputview.events.PointerEvent(
        this.view, i18n.input.chrome.inputview.events.EventType.CLICK,
        e.target, pageOffset.x, pageOffset.y, identifier));
  }
  this.isDBLClicking_ = false;
  this.isLongPressing_ = false;
  this.isFlickering_ = false;
  this.swipeState_.reset();
};


/**
 * Cancel double click recognition on this target.
 */
PointerActionBundle.prototype.cancelDoubleClick = function() {
  this.pointerDownTimeStamp_ = 0;
};


/**
 * Handles pointer down, e.g., mousedown/touchstart.
 *
 * @param {!goog.events.BrowserEvent} e The event.
 */
PointerActionBundle.prototype.handlePointerDown = function(e) {
  this.currentTarget_ = e.target;
  goog.Timer.clear(this.longPressTimer_);
  var identifier = Util.getEventIdentifier(e);
  if (e.type == goog.events.EventType.TOUCHSTART) {
    this.maybeTriggerKeyDownLongPress_(e, identifier);
  }
  this.maybeHandleDBLClick_(e, identifier);
  if (!this.isDBLClicking_) {
    var pageOffset = this.getPageOffset_(e);
    this.dispatchEvent(new i18n.input.chrome.inputview.events.PointerEvent(
        this.view, i18n.input.chrome.inputview.events.EventType.POINTER_DOWN,
        e.target, pageOffset.x, pageOffset.y, identifier,
        this.pointerDownTimeStamp_));
  }
};


/**
 * Gets the page offset from the event which may be mouse event or touch event.
 *
 * @param {!goog.events.BrowserEvent} e .
 * @return {!goog.math.Coordinate} .
 * @private
 */
PointerActionBundle.prototype.getPageOffset_ = function(e) {
  var nativeEvt = e.getBrowserEvent();
  if (nativeEvt.pageX && nativeEvt.pageY) {
    return new goog.math.Coordinate(nativeEvt.pageX, nativeEvt.pageY);
  }


  var touchEventList = nativeEvt['changedTouches'];
  if (!touchEventList || touchEventList.length == 0) {
    touchEventList = nativeEvt['touches'];
  }
  if (touchEventList && touchEventList.length > 0) {
    var touchEvent = touchEventList[0];
    return new goog.math.Coordinate(touchEvent.pageX, touchEvent.pageY);
  }

  return new goog.math.Coordinate(0, 0);
};


/**
 * Maybe triggers the long press timer when pointer down.
 *
 * @param {!goog.events.BrowserEvent} e The event.
 * @param {number} identifier .
 * @private
 */
PointerActionBundle.prototype.maybeTriggerKeyDownLongPress_ = function(e,
    identifier) {
  if (this.view && (this.view.pointerConfig.longPressWithPointerUp ||
      this.view.pointerConfig.longPressWithoutPointerUp)) {
    this.longPressTimer_ = goog.Timer.callOnce(
        goog.bind(this.triggerLongPress_, this, e, identifier),
        this.view.pointerConfig.longPressDelay, this);
  }
};


/**
 * Maybe handle the double click.
 *
 * @param {!goog.events.BrowserEvent} e .
 * @param {number} identifier .
 * @private
 */
PointerActionBundle.prototype.maybeHandleDBLClick_ = function(e, identifier) {
  if (this.view && this.view.pointerConfig.dblClick) {
    var timeInMs = new Date().getTime();
    var interval = this.view.pointerConfig.dblClickDelay ||
        PointerActionBundle.DOUBLE_CLICK_INTERVAL_;
    var nativeEvt = e.getBrowserEvent();
    if ((timeInMs - this.pointerDownTimeStamp_) < interval) {
      this.dispatchEvent(new i18n.input.chrome.inputview.events.PointerEvent(
          this.view, i18n.input.chrome.inputview.events.EventType.DOUBLE_CLICK,
          e.target, nativeEvt.pageX, nativeEvt.pageY, identifier));
      this.isDBLClicking_ = true;
    }
    this.pointerDownTimeStamp_ = timeInMs;
  }
};


/**
 * Triggers long press event.
 *
 * @param {!goog.events.BrowserEvent} e The event.
 * @param {number} identifier .
 * @private
 */
PointerActionBundle.prototype.triggerLongPress_ = function(e, identifier) {
  var nativeEvt = e.getBrowserEvent();
  if (nativeEvt.touches.length > 1) {
    return;
  }
  this.dispatchEvent(new i18n.input.chrome.inputview.events.PointerEvent(
      this.view, i18n.input.chrome.inputview.events.EventType.LONG_PRESS,
      e.target, nativeEvt.pageX, nativeEvt.pageY, identifier));
  this.isLongPressing_ = true;
};


/** @override */
PointerActionBundle.prototype.disposeInternal = function() {
  goog.dispose(this.longPressTimer_);

  goog.base(this, 'disposeInternal');
};

});  // goog.scope
