// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

var menu = null;
var submenu = null;
var separator = null;

function setUp() {
  menu = util.queryDecoratedElement('#menu', cr.ui.Menu);
  separator = queryRequiredElement('#actions-separator', menu);
  submenu = new ActionsSubmenu(menu);
}

function tearDown() {
  var items = document.querySelectorAll('#menu cr-menu-item');
  for (var i = 0; i < items.length; i++) {
    items[i].parentNode.removeChild(items[i]);
  }
  separator.hidden = true;
}

function testSeparator() {
  assertTrue(separator.hidden);

  submenu.setActionsModel(new MockActionsModel({
    id: new MockActionModel('title', null)
  }));
  assertFalse(separator.hidden);

  submenu.setActionsModel(new MockActionsModel([]));
  assertTrue(separator.hidden);
}

function testNullModel() {
  submenu.setActionsModel(new MockActionsModel({
    id: new MockActionModel('title', null)
  }));
  var item = menu.querySelector('cr-menu-item');
  assertTrue(!!item);

  submenu.setActionsModel(null);
  var item = menu.querySelector('cr-menu-item');
  assertFalse(!!item);
}

function testCustomActionRendering() {
  submenu.setActionsModel(new MockActionsModel({
    id: new MockActionModel('title', null)
  }));
  var item = menu.querySelector('cr-menu-item');
  assertTrue(!!item);
  assertEquals('title', item.textContent);
  assertEquals(null, item.command);
}

function testCommandActionRendering() {
  submenu.setActionsModel(new MockActionsModel({
    SHARE: new MockActionModel('share with me!', null)
  }));
  var item = menu.querySelector('cr-menu-item');
  assertTrue(!!item);
  assertEquals('Share', item.textContent);
  assertEquals('share', item.command.id);
}
