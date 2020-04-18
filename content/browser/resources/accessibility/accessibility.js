// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

cr.define('accessibility', function() {
  'use strict';

  // Note: keep these values in sync with the values in
  // content/common/accessibility_mode_enums.h
  const AXMode = {
    kNativeAPIs: 1 << 0,
    kWebContents: 1 << 1,
    kInlineTextBoxes: 1 << 2,
    kScreenReader: 1 << 3,
    kHTML: 1 << 4,

    get kAXModeWebContentsOnly() {
      return AXMode.kWebContents |
        AXMode.kInlineTextBoxes | AXMode.kScreenReader |
        AXMode.kHTML;
    },

    get kAXModeComplete() {
      return AXMode.kNativeAPIs | AXMode.kWebContents |
        AXMode.kInlineTextBoxes | AXMode.kScreenReader |
        AXMode.kHTML;
    }
  };

  function requestData() {
    var xhr = new XMLHttpRequest();
    xhr.open('GET', 'targets-data.json', false);
    xhr.send(null);
    if (xhr.status === 200) {
      console.log(xhr.responseText);
      return JSON.parse(xhr.responseText);
    }
    return [];
  }

  function toggleAccessibility(data, element, mode) {
    chrome.send('toggleAccessibility',
                [String(data.processId), String(data.routeId), mode]);
    document.location.reload();
  }

  function requestWebContentsTree(data, element) {
    chrome.send('requestWebContentsTree',
                [String(data.processId), String(data.routeId)]);
  }

  function initialize() {
    console.log('initialize');
    var data = requestData();

    bindCheckbox('native', data['native']);
    bindCheckbox('web', data['web']);
    bindCheckbox('text', data['text']);
    bindCheckbox('screenreader', data['screenreader']);
    bindCheckbox('html', data['html']);
    bindCheckbox('internal', data['internal']);

    $('pages').textContent = '';

    var list = data['list'];
    for (var i = 0; i < list.length; i++) {
      addToPagesList(list[i]);
    }

    var showNativeUI = $('showNativeUI');
    showNativeUI.addEventListener('click', function() {
      chrome.send('requestNativeUITree', []);
    });
  }

  function bindCheckbox(name, value) {
    if (value == 'on')
      $(name).checked = true;
    if (value == 'disabled') {
      $(name).disabled = true;
      $(name).labels[0].classList.add('disabled');
    }
    $(name).addEventListener('change', function() {
      chrome.send('setGlobalFlag', [name, $(name).checked]);
      document.location.reload();
    });
  }

  function addToPagesList(data) {
    // TODO: iterate through data and pages rows instead
    var id = data['processId'] + '.' + data['routeId'];
    var row = document.createElement('div');
    row.className = 'row';
    row.id = id;
    formatRow(row, data);

    row.processId = data.processId;
    row.routeId = data.routeId;

    var list = $('pages');
    list.appendChild(row);
  }

  function formatRow(row, data) {
    if (!('url' in data)) {
      if ('error' in data) {
        row.appendChild(createErrorMessageElement(data, row));
        return;
      }
    }

    var siteInfo = document.createElement('div');
    var properties = ['favicon_url', 'name', 'url'];
    for (var j = 0; j < properties.length; j++)
      siteInfo.appendChild(formatValue(data, properties[j]));
    row.appendChild(siteInfo);

    row.appendChild(createModeElement(AXMode.kNativeAPIs, data))
    row.appendChild(createModeElement(AXMode.kWebContents, data))
    row.appendChild(createModeElement(AXMode.kInlineTextBoxes,
      data))
    row.appendChild(createModeElement(AXMode.kScreenReader, data))
    row.appendChild(createModeElement(AXMode.kHTML, data))

    row.appendChild(document.createTextNode(' | '));

    if ('tree' in data) {
      row.appendChild(createShowAccessibilityTreeElement(data, row, true));
      row.appendChild(createHideAccessibilityTreeElement(row.id));
      row.appendChild(createAccessibilityTreeElement(data));
    }
    else {
      row.appendChild(createShowAccessibilityTreeElement(data, row, false));
      if ('error' in data)
        row.appendChild(createErrorMessageElement(data, row));
    }
  }

  function formatValue(data, property) {
    var value = data[property];

    if (property == 'favicon_url') {
      var faviconElement = document.createElement('img');
      if (value)
        faviconElement.src = value;
      faviconElement.alt = "";
      return faviconElement;
    }

    var text = value ? String(value) : '';
    if (text.length > 100)
      text = text.substring(0, 100) + '\u2026';  // ellipsis

    var span = document.createElement('span');
    span.textContent = ' ' + text + ' ';
    span.className = property;
    return span;
  }

  function getNameForAccessibilityMode(mode) {
    switch (mode) {
      case AXMode.kNativeAPIs:
        return "native"
      case AXMode.kWebContents:
        return "web"
      case AXMode.kInlineTextBoxes:
        return "inline text"
      case AXMode.kScreenReader:
        return "screen reader"
      case AXMode.kHTML:
        return "html"
    }
    return "unknown"
  }

  function createModeElement(mode, data) {
    var currentMode = data['a11y_mode'];
    var link = document.createElement('a', 'action-link');
    link.setAttribute('role', 'button');

    var stateText = ((currentMode & mode) != 0) ? 'true' : 'false';
    link.textContent = getNameForAccessibilityMode(mode) + ": " + stateText;
    link.setAttribute('aria-pressed', stateText);
    link.addEventListener('click',
                          toggleAccessibility.bind(this, data, link, mode));
    return link;
  }

  function createShowAccessibilityTreeElement(data, row, opt_refresh) {
    var link = document.createElement('a', 'action-link');
    link.setAttribute('role', 'button');
    if (opt_refresh)
      link.textContent = 'refresh accessibility tree';
    else
      link.textContent = 'show accessibility tree';
    link.id = row.id + ':showTree';
    link.addEventListener('click',
                          requestWebContentsTree.bind(this, data, link));
    return link;
  }

  function createHideAccessibilityTreeElement(id) {
    var link = document.createElement('a', 'action-link');
    link.setAttribute('role', 'button');
    link.textContent = 'hide accessibility tree';
    link.addEventListener('click',
                          function() {
        $(id + ':showTree').textContent = 'show accessibility tree';
        var existingTreeElements = $(id).getElementsByTagName('pre');
        for (var i = 0; i < existingTreeElements.length; i++)
          $(id).removeChild(existingTreeElements[i]);
        var row = $(id);
        while (row.lastChild != $(id + ':showTree'))
          row.removeChild(row.lastChild);
    });
    return link;
  }

  function createErrorMessageElement(data) {
    var errorMessageElement = document.createElement('div');
    var errorMessage = data.error;
    errorMessageElement.innerHTML = errorMessage + '&nbsp;';
    var closeLink = document.createElement('a');
    closeLink.href='#';
    closeLink.textContent = '[close]';
    closeLink.addEventListener('click', function() {
        var parentElement = errorMessageElement.parentElement;
        parentElement.removeChild(errorMessageElement);
        if (parentElement.childElementCount == 0)
          parentElement.parentElement.removeChild(parentElement);
    });
    errorMessageElement.appendChild(closeLink);
    return errorMessageElement;
  }

  // Called from C++
  function showTree(data) {
    var id = data.processId + '.' + data.routeId;
    var row = $(id);
    if (!row)
      return;

    row.textContent = '';
    formatRow(row, data);
  }

  // Called from C++
  function showNativeUITree(data) {
    var treeElement = document.querySelector('#native_ui pre');
    if (!treeElement) {
      var treeElement = document.createElement('pre');
      $('native_ui').appendChild(treeElement);
    }
    treeElement.textContent = data.tree;
  }

  function createAccessibilityTreeElement(data) {
    var treeElement = document.createElement('pre');
    var tree = data.tree;
    treeElement.textContent = tree;
    return treeElement;
  }

  // These are the functions we export so they can be called from C++.
  return {
    initialize: initialize,
    showTree: showTree,
    showNativeUITree: showNativeUITree
  };
});

document.addEventListener('DOMContentLoaded', accessibility.initialize);
