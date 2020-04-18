// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * Add support for tab pannels on custom elements (tabs, header and content)
 *
 */
(function() {

function registerEvent(target, eventType, handler) {
  if (target.addEventListener) {
    target.addEventListener(eventType, handler);
  } else {
    target.attachEvent(eventType, handler);
  }
}

function getSessionKey(key) {
  return window.sessionStorage.getItem("__tab_"+key);
}

function setSessionKey(key, value) {
  window.sessionStorage.setItem("__tab_"+key, value);
}

function onTabHeaderKeyDown(e) {
  if (e.keyCode == 13) {
    e.preventDefault();
    onTabClicked(e);
  }
}

function onTabClicked(e) {
  var tabs = e.target.parentNode;
  if (!tabs || tabs.tagName !== 'TABS')
    return;

  var headers = tabs.getElementsByTagName('header'),
    contents = tabs.getElementsByTagName('content'),
    tabGroup = tabs.getAttribute("data-group"),
    tabValue = e.target.getAttribute("data-value");

  if (tabGroup && tabValue && window.sessionStorage)
    setSessionKey(tabGroup, tabValue);

  for (var i=0; i<headers.length; i++) {
    if (headers[i] === e.target) {
      headers[i].classList.remove('unselected');
      if (contents.length > i)
        contents[i].classList.remove('unselected');
    } else {
      headers[i].classList.add('unselected');
      if (contents.length > i)
        contents[i].classList.add('unselected');
    }
  }
}

function initTabPane(tab) {
  var tabGroup = tab.getAttribute("data-group");
  if (tabGroup && window.sessionStorage)
    var tabGroupSelectedValue = getSessionKey(tabGroup);

  var headers = tab.getElementsByTagName('header');
  var contents = tab.getElementsByTagName('content');
  var hasSelected = false;

  if (headers.length==0 || contents.length==0)
    return;

  for (var j=0; j<headers.length; j++) {
    var selected = tabGroup && tabGroupSelectedValue
      && tabGroupSelectedValue===headers[j].getAttribute("data-value");

    if (!hasSelected && selected) {
      headers[j].classList.remove("unselected");
      contents[j].classList.remove("unselected");
      hasSelected = true;
    } else {
      headers[j].classList.add("unselected");
      contents[j].classList.add("unselected");
    }

    headers[j].addEventListener('click', onTabClicked);
    headers[j].addEventListener('keydown', onTabHeaderKeyDown);
  }

  if (!hasSelected) {
    headers[0].classList.remove("unselected");
    contents[0].classList.remove("unselected");
  }
}

function onLoad() {
  var tabs = document.getElementsByTagName('tabs');
  for (var i=0; i<tabs.length; i++) {
    initTabPane(tabs[i]);
  }
}

window.addEventListener('DOMContentLoaded', onLoad);

})();
