/* Copyright (c) 2014 The Chromium Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file. */

function setRadio(name, defaultValue) {
  chrome.storage.sync.get(name, function(result) {
    var value = result[name];
    if (value === undefined) {
      value = defaultValue;
      var obj = {};
      obj[name] = value;
      chrome.storage.sync.set(obj);
    }
    var controls = document.querySelectorAll(
        'input[type="radio"][name="' + name + '"]');
    for (var i = 0; i < controls.length; i++) {
      var c = controls[i];
      if (c.value == value) {
        c.checked = true;
      }
      c.addEventListener('change', function(evt) {
        if (evt.target.checked) {
          var obj = {};
          obj[evt.target.name] = evt.target.value;
          chrome.storage.sync.set(obj);
        }
      }, false);
    }
  });
}

function load() {
  var isMac = (navigator.appVersion.indexOf("Mac") != -1);
  if (isMac) {
    document.body.classList.add('mac');
  } else {
    document.body.classList.add('nonmac');
  }

  var isCros = (navigator.appVersion.indexOf("CrOS") != -1);
  if (isCros) {
    document.body.classList.add('cros');
  } else {
    document.body.classList.add('noncros');
  }

  setRadio('onenable', 'anim');
  setRadio('onjump', 'flash');

  var heading = document.querySelector('h1');
  var sel = window.getSelection();
  sel.setBaseAndExtent(heading, 0, heading, 0);

  document.title =
      chrome.i18n.getMessage('caret_browsing_caretBrowsingOptions');
  var i18nElements = document.querySelectorAll('*[i18n-content]');
  for (var i = 0; i < i18nElements.length; i++) {
    var elem = i18nElements[i];
    var msg = elem.getAttribute('i18n-content');
    elem.innerHTML = chrome.i18n.getMessage(msg);
  }
}

window.addEventListener('load', load, false);
