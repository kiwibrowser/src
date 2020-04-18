/**
 * @license
 * Copyright (c) 2014 The Polymer Project Authors. All rights reserved.
 * This code may only be used under the BSD style license found at http://polymer.github.io/LICENSE.txt
 * The complete set of authors may be found at http://polymer.github.io/AUTHORS.txt
 * The complete set of contributors may be found at http://polymer.github.io/CONTRIBUTORS.txt
 * Code distributed by Google as part of the polymer project is also
 * subject to an additional IP rights grant found at http://polymer.github.io/PATENTS.txt
 */

// We declare it as a namespace to be compatible with JSCompiler.
/** @const */ var MockInteractions = {};

(function(scope, global) {
  'use strict';

  // In case the var above was not global, or if it was renamed.
  global.MockInteractions = scope;

  var HAS_NEW_MOUSE = (function() {
    var has = false;
    try {
      has = Boolean(new MouseEvent('x'));
    } catch (_) {}
    return has;
  })();
  
  var HAS_NEW_TOUCH = (function() {
    var has = false;
    try {
      has = Boolean(new TouchEvent('x'));
    } catch (_) {}
    return has;
  })();

  /**
   * Returns the (x,y) coordinates representing the middle of a node.
   *
   * @param {!Element} node An element.
   */
  function middleOfNode(node) {
    var bcr = node.getBoundingClientRect();
    return {
      y: bcr.top + (bcr.height / 2),
      x: bcr.left + (bcr.width / 2)
    };
  }

  /**
   * Returns the (x,y) coordinates representing the top left corner of a node.
   *
   * @param {!Element} node An element.
   */
  function topLeftOfNode(node) {
    var bcr = node.getBoundingClientRect();
    return {
      y: bcr.top,
      x: bcr.left
    };
  }

  /**
   * Returns a list of Touch objects that correspond to an array of positions
   * and a target node. The Touch instances will each have a unique Touch
   * identifier.
   *
   * @param {!Array<{ x: number, y: number }>} xyList A list of (x,y) coordinate objects.
   * @param {!Element} node A target element node.
   */
  function makeTouches(xyList, node) {
    var id = 0;

    return xyList.map(function(xy) {
      var touchInit = {
        identifier: id++,
        target: node,
        clientX: xy.x,
        clientY: xy.y
      };

      return HAS_NEW_TOUCH ? new window.Touch(touchInit) : touchInit;
    });
  }

  /**
   * Generates and dispatches a TouchEvent of a given type, at a specified
   * position of a target node.
   *
   * @param {string} type The type of TouchEvent to generate.
   * @param {{ x: number, y: number }} xy An (x,y) coordinate for the generated
   * TouchEvent.
   * @param {!Element} node The target element node for the generated
   * TouchEvent to be dispatched on.
   */
  function makeSoloTouchEvent(type, xy, node) {
    xy = xy || middleOfNode(node);
    var touches = makeTouches([xy], node);
    var touchEventInit = {
      touches: touches,
      targetTouches: touches,
      changedTouches: touches
    };
    var event;

    if (HAS_NEW_TOUCH) {
      touchEventInit.bubbles = true;
      touchEventInit.cancelable = true;
      event = new TouchEvent(type, touchEventInit);
    } else {
      event = new CustomEvent(type, {
        bubbles: true,
        cancelable: true,
        // Allow event to go outside a ShadowRoot.
        composed: true
      });
      for (var property in touchEventInit) {
        event[property] = touchEventInit[property];
      }
    }

    node.dispatchEvent(event);
  }

  /**
   * Fires a mouse event on a specific node, at a given set of coordinates.
   * This event bubbles and is cancellable.
   *
   * @param {string} type The type of mouse event (such as 'tap' or 'down').
   * @param {{ x: number, y: number }} xy The (x,y) coordinates the mouse event should be fired from.
   * @param {!Element} node The node to fire the event on.
   */
  function makeMouseEvent(type, xy, node) {
    var props = {
      bubbles: true,
      cancelable: true,
      clientX: xy.x,
      clientY: xy.y,
      // Allow event to go outside a ShadowRoot.
      composed: true,
      // Make this a primary input.
      buttons: 1 // http://developer.mozilla.org/en-US/docs/Web/API/MouseEvent/buttons
    };
    var e;
    if (HAS_NEW_MOUSE) {
      e = new MouseEvent(type, props);
    } else {
      e = document.createEvent('MouseEvent');
      e.initMouseEvent(
        type, props.bubbles, props.cancelable,
        null, /* view */
        null, /* detail */
        0,    /* screenX */
        0,    /* screenY */
        props.clientX, props.clientY,
        false, /*ctrlKey */
        false, /*altKey */
        false, /*shiftKey */
        false, /*metaKey */
        0,     /*button */
        null   /*relatedTarget*/);
    }
    node.dispatchEvent(e);
  }

  /**
   * Simulates a mouse move action by firing a `move` mouse event on a
   * specific node, between a set of coordinates.
   *
   * @param {!Element} node The node to fire the event on.
   * @param {Object} fromXY The (x,y) coordinates the dragging should start from.
   * @param {Object} toXY The (x,y) coordinates the dragging should end at.
   * @param {?number=} steps Optional. The numbers of steps in the move motion.
   *    If not specified, the default is 5.
   */
  function move(node, fromXY, toXY, steps) {
    steps = steps || 5;
    var dx = Math.round((fromXY.x - toXY.x) / steps);
    var dy = Math.round((fromXY.y - toXY.y) / steps);
    var xy = {
      x: fromXY.x,
      y: fromXY.y
    };
    for (var i = steps; i > 0; i--) {
      makeMouseEvent('mousemove', xy, node);
      xy.x += dx;
      xy.y += dy;
    }
    makeMouseEvent('mousemove', {
      x: toXY.x,
      y: toXY.y
    }, node);
  }

  /**
   * Simulates a mouse dragging action originating in the middle of a specific node.
   *
   * @param {!Element} target The node to fire the event on.
   * @param {?number} dx The horizontal displacement.
   * @param {?number} dy The vertical displacement
   * @param {?number=} steps Optional. The numbers of steps in the dragging motion.
   *    If not specified, the default is 5.
   */
  function track(target, dx, dy, steps) {
    dx = dx | 0;
    dy = dy | 0;
    steps = steps || 5;
    down(target);
    var xy = middleOfNode(target);
    var xy2 = {
      x: xy.x + dx,
      y: xy.y + dy
    };
    move(target, xy, xy2, steps);
    up(target, xy2);
  }

  /**
   * Fires a `down` mouse event on a specific node, at a given set of coordinates.
   * This event bubbles and is cancellable. If the (x,y) coordinates are
   * not specified, the middle of the node will be used instead.
   *
   * @param {!Element} node The node to fire the event on.
   * @param {{ x: number, y: number }=} xy Optional. The (x,y) coordinates the mouse event should be fired from.
   */
  function down(node, xy) {
    xy = xy || middleOfNode(node);
    makeMouseEvent('mousedown', xy, node);
  }

  /**
   * Fires an `up` mouse event on a specific node, at a given set of coordinates.
   * This event bubbles and is cancellable. If the (x,y) coordinates are
   * not specified, the middle of the node will be used instead.
   *
   * @param {!Element} node The node to fire the event on.
   * @param {{ x: number, y: number }=} xy Optional. The (x,y) coordinates the mouse event should be fired from.
   */
  function up(node, xy) {
    xy = xy || middleOfNode(node);
    makeMouseEvent('mouseup', xy, node);
  }

  /**
   * Generate a click event on a given node, optionally at a given coordinate.
   * @param {!Element} node The node to fire the click event on.
   * @param {{ x: number, y: number }=} xy Optional. The (x,y) coordinates the mouse event should
   * be fired from.
   */
  function click(node, xy) {
    xy = xy || middleOfNode(node);
    makeMouseEvent('click', xy, node);
  }

  /**
   * Generate a touchstart event on a given node, optionally at a given coordinate.
   * @param {!Element} node The node to fire the click event on.
   * @param {{ x: number, y: number }=} xy Optional. The (x,y) coordinates the touch event should
   * be fired from.
   */
  function touchstart(node, xy) {
    xy = xy || middleOfNode(node);
    makeSoloTouchEvent('touchstart', xy, node);
  }


  /**
   * Generate a touchend event on a given node, optionally at a given coordinate.
   * @param {!Element} node The node to fire the click event on.
   * @param {{ x: number, y: number }=} xy Optional. The (x,y) coordinates the touch event should
   * be fired from.
   */
  function touchend(node, xy) {
    xy = xy || middleOfNode(node);
    makeSoloTouchEvent('touchend', xy, node);
  }

  /**
   * Simulates a complete mouse click by firing a `down` mouse event, followed
   * by an asynchronous `up` and `tap` events on a specific node. Calls the
   *`callback` after the `tap` event is fired.
   *
   * @param {!Element} target The node to fire the event on.
   * @param {?Function=} callback Optional. The function to be called after the action ends.
   * @param {?{
   *   emulateTouch: boolean
   * }=} options Optional. Configure the emulation fidelity of the mouse events.
   */
  function downAndUp(target, callback, options) {
    if (options && options.emulateTouch) {
      touchstart(target);
      touchend(target);
    }

    down(target);
    Polymer.Base.async(function() {
      up(target);
      click(target);
      callback && callback();
    });
  }

  /**
   * Fires a 'tap' mouse event on a specific node. This respects the pointer-events
   * set on the node, and will not fire on disabled nodes.
   *
   * @param {!Element} node The node to fire the event on.
   * @param {?{
   *   emulateTouch: boolean
   * }=} options Optional. Configure the emulation fidelity of the mouse event.
   */
  function tap(node, options) {
    // Respect nodes that are disabled in the UI.
    if (window.getComputedStyle(node)['pointer-events'] === 'none')
      return;

    var xy = middleOfNode(node);

    if (options && options.emulateTouch) {
      touchstart(node, xy);
      touchend(node, xy);
    }

    down(node, xy);
    up(node, xy);
    click(node, xy);
  }

  /**
   * Focuses a node by firing a `focus` event. This event does not bubble.
   *
   * @param {!Element} target The node to fire the event on.
   */
  function focus(target) {
    Polymer.Base.fire('focus', {}, {
      bubbles: false,
      node: target
    });
  }

  /**
   * Blurs a node by firing a `blur` event. This event does not bubble.
   *
   * @param {!Element} target The node to fire the event on.
   */
  function blur(target) {
    Polymer.Base.fire('blur', {}, {
      bubbles: false,
      node: target
    });
  }

  /**
   * Returns a keyboard event. This event bubbles and is cancellable.
   *
   * @param {string} type The type of keyboard event (such as 'keyup' or 'keydown').
   * @param {number} keyCode The keyCode for the event.
   * @param {(string|Array<string>)=} modifiers The key modifiers for the event.
   *     Accepted values are shift, ctrl, alt, meta.
   * @param {string=} key The KeyboardEvent.key value for the event.
   */
  function keyboardEventFor(type, keyCode, modifiers, key) {
    var event = new CustomEvent(type, {
      detail: 0,
      bubbles: true,
      cancelable: true,
      // Allow event to go outside a ShadowRoot.
      composed: true
    });

    event.keyCode = keyCode;
    event.code = keyCode;

    modifiers = modifiers || [];
    if (typeof modifiers === 'string') {
      modifiers = [modifiers];
    }
    event.shiftKey = modifiers.indexOf('shift') !== -1;
    event.altKey = modifiers.indexOf('alt') !== -1;
    event.ctrlKey = modifiers.indexOf('ctrl') !== -1;
    event.metaKey = modifiers.indexOf('meta') !== -1;

    event.key = key;

    return event;
  }

  /**
   * Fires a keyboard event on a specific node. This event bubbles and is cancellable.
   *
   * @param {!Element} target The node to fire the event on.
   * @param {string} type The type of keyboard event (such as 'keyup' or 'keydown').
   * @param {number} keyCode The keyCode for the event.
   * @param {(string|Array<string>)=} modifiers The key modifiers for the event.
   *     Accepted values are shift, ctrl, alt, meta.
   * @param {string=} key The KeyboardEvent.key value for the event.
   */
  function keyEventOn(target, type, keyCode, modifiers, key) {
    target.dispatchEvent(keyboardEventFor(type, keyCode, modifiers, key));
  }

  /**
   * Fires a 'keydown' event on a specific node. This event bubbles and is cancellable.
   *
   * @param {!Element} target The node to fire the event on.
   * @param {number} keyCode The keyCode for the event.
   * @param {(string|Array<string>)=} modifiers The key modifiers for the event.
   *     Accepted values are shift, ctrl, alt, meta.
   * @param {string=} key The KeyboardEvent.key value for the event.
   */
  function keyDownOn(target, keyCode, modifiers, key) {
    keyEventOn(target, 'keydown', keyCode, modifiers, key);
  }

  /**
   * Fires a 'keyup' event on a specific node. This event bubbles and is cancellable.
   *
   * @param {!Element} target The node to fire the event on.
   * @param {number} keyCode The keyCode for the event.
   * @param {(string|Array<string>)=} modifiers The key modifiers for the event.
   *     Accepted values are shift, ctrl, alt, meta.
   * @param {string=} key The KeyboardEvent.key value for the event.
   */
  function keyUpOn(target, keyCode, modifiers, key) {
    keyEventOn(target, 'keyup', keyCode, modifiers, key);
  }

  /**
   * Simulates a complete key press by firing a `keydown` keyboard event, followed
   * by an asynchronous `keyup` event on a specific node.
   *
   * @param {!Element} target The node to fire the event on.
   * @param {number} keyCode The keyCode for the event.
   * @param {(string|Array<string>)=} modifiers The key modifiers for the event.
   *     Accepted values are shift, ctrl, alt, meta.
   * @param {string=} key The KeyboardEvent.key value for the event.
   */
  function pressAndReleaseKeyOn(target, keyCode, modifiers, key) {
    keyDownOn(target, keyCode, modifiers, key);
    Polymer.Base.async(function() {
      keyUpOn(target, keyCode, modifiers, key);
    }, 1);
  }

  /**
   * Simulates a complete 'enter' key press by firing a `keydown` keyboard event,
   * followed by an asynchronous `keyup` event on a specific node.
   *
   * @param {!Element} target The node to fire the event on.
   */
  function pressEnter(target) {
    pressAndReleaseKeyOn(target, 13);
  }

  /**
   * Simulates a complete 'space' key press by firing a `keydown` keyboard event,
   * followed by an asynchronous `keyup` event on a specific node.
   *
   * @param {!Element} target The node to fire the event on.
   */
  function pressSpace(target) {
    pressAndReleaseKeyOn(target, 32);
  }

  scope.focus = focus;
  scope.blur = blur;
  scope.down = down;
  scope.up = up;
  scope.downAndUp = downAndUp;
  scope.tap = tap;
  scope.move = move;
  scope.touchstart = touchstart;
  scope.touchend = touchend;
  scope.makeSoloTouchEvent = makeSoloTouchEvent;
  scope.track = track;
  scope.pressAndReleaseKeyOn = pressAndReleaseKeyOn;
  scope.pressEnter = pressEnter;
  scope.pressSpace = pressSpace;
  scope.keyDownOn = keyDownOn;
  scope.keyUpOn = keyUpOn;
  scope.keyboardEventFor = keyboardEventFor;
  scope.keyEventOn = keyEventOn;
  scope.middleOfNode = middleOfNode;
  scope.topLeftOfNode = topLeftOfNode;
})(MockInteractions, this);
