// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

(function() {

'use strict';

/** @type {(sinon.Spy|function():void)} */
var onShow = null;
/** @type {(sinon.Spy|function():void)} */
var onHide = null;
/** @type {remoting.MenuButton} */
var menuButton = null;

QUnit.module('MenuButton', {
  beforeEach: function() {
    var fixture = document.getElementById('qunit-fixture');
    fixture.innerHTML =
        '<span class="menu-button" id="menu-button-container">' +
          '<button class="menu-button-activator">Click me</button>' +
          '<ul>' +
            '<li id="menu-option-1">Option 1</li>' +
          '</ul>' +
        '</span>';
    onShow = /** @type {(sinon.Spy|function():void)} */ (sinon.spy());
    onHide = /** @type {(sinon.Spy|function():void)} */ (sinon.spy());
    menuButton = new remoting.MenuButton(
        document.getElementById('menu-button-container'),
        /** @type {function():void} */ (onShow),
        /** @type {function():void} */ (onHide));
  },
  afterEach: function() {
    onShow = null;
    onHide = null;
    menuButton = null;
  }
});

QUnit.test('should display on click', function(assert) {
  var menu = menuButton.menu();
  assert.ok(menu.offsetWidth == 0 && menu.offsetHeight == 0);
  menuButton.button().click();
  assert.ok(menu.offsetWidth != 0 && menu.offsetHeight != 0);
});

QUnit.test('should dismiss when the menu is clicked', function(assert) {
  var menu = menuButton.menu();
  menuButton.button().click();
  menu.click();
  assert.ok(menu.offsetWidth == 0 && menu.offsetHeight == 0);
});

QUnit.test('should dismiss when anything outside the menu is clicked',
    function(assert) {
  var menu = menuButton.menu();
  menuButton.button().click();
  var x = menu.offsetLeft + menu.offsetWidth + 1;
  var y = menu.offsetTop + menu.offsetHeight + 1;
  var notMenu = document.elementFromPoint(x, y);
  console.assert(notMenu != menu, 'Unable to click outside menu.');
  notMenu.click();
  assert.ok(menu.offsetWidth == 0 && menu.offsetHeight == 0);
});

QUnit.test('should dismiss when menu item is clicked', function(assert) {
  var menu = menuButton.menu();
  menuButton.button().click();
  var element = document.getElementById('menu-option-1');
  element.click();
  assert.ok(menu.offsetWidth == 0 && menu.offsetHeight == 0);
});

QUnit.test('should invoke callbacks', function(assert) {
  assert.ok(!onShow.called);
  menuButton.button().click();
  assert.ok(onShow.called);
  assert.ok(!onHide.called);
  menuButton.menu().click();
  assert.ok(onHide.called);
});

QUnit.test('select method should set/unset background image', function(assert) {
  var element = document.getElementById('menu-option-1');
  var style = window.getComputedStyle(element);
  assert.ok(style.backgroundImage == 'none');
  remoting.MenuButton.select(element, true);
  style = window.getComputedStyle(element);
  assert.ok(style.backgroundImage != 'none');
  remoting.MenuButton.select(element, false);
  style = window.getComputedStyle(element);
  assert.ok(style.backgroundImage == 'none');
});

}());
