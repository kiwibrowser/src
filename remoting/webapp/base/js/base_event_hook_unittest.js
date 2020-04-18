// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

(function() {

'use strict';

/** @type {base.EventSourceImpl} */
var eventSource = null;

/** @type {HTMLElement} */
var domElement = null;

/** @type {chromeMocks.Event} */
var myChromeEvent = null;

/** @type {Listener} */
var listener = null;

/**
 * @param {HTMLElement} element
 * @constructor
 */
var Listener = function(element) {
  /** @type {(sinon.Spy|function(...?))} */
  this.onChromeEvent = sinon.spy();
  /** @type {(sinon.Spy|function(...?))} */
  this.onClickEvent = sinon.spy();
  /** @type {(sinon.Spy|function(...?))} */
  this.onCustomEvent = sinon.spy();

  this.eventHooks_ = new base.Disposables(
      new base.DomEventHook(element, 'click', this.onClickEvent.bind(this),
                            false),
      new base.EventHook(eventSource, 'customEvent',
                         this.onCustomEvent.bind(this)),
      new base.ChromeEventHook(myChromeEvent, this.onChromeEvent.bind(this)));
};

Listener.prototype.dispose = function() {
  this.eventHooks_.dispose();
};

function raiseAllEvents() {
  domElement.click();
  myChromeEvent.mock$fire();
  eventSource.raiseEvent('customEvent');
}

QUnit.module('base.EventHook', {
  beforeEach: function() {
    domElement = /** @type {HTMLElement} */ (document.createElement('div'));
    eventSource = new base.EventSourceImpl();
    eventSource.defineEvents(['customEvent']);
    myChromeEvent = new chromeMocks.Event();
    listener = new Listener(domElement);
  },
  afterEach: function() {
    domElement = null;
    eventSource = null;
    myChromeEvent = null;
    listener = null;
  }
});

QUnit.test('EventHook should hook events when constructed', function() {
  raiseAllEvents();
  sinon.assert.calledOnce(listener.onClickEvent);
  sinon.assert.calledOnce(listener.onChromeEvent);
  sinon.assert.calledOnce(listener.onCustomEvent);
  listener.dispose();
});

QUnit.test('EventHook should unhook events when disposed', function() {
  listener.dispose();
  raiseAllEvents();
  sinon.assert.notCalled(listener.onClickEvent);
  sinon.assert.notCalled(listener.onChromeEvent);
  sinon.assert.notCalled(listener.onCustomEvent);
});

})();
