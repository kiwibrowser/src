// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

cr.define('chrome.sync.events_tab', function() {
  'use strict';

  function toggleDisplay(event) {
    var originatingButton = event.target;
    if (originatingButton.className != 'toggle-button') {
      return;
    }
    var detailsNode = originatingButton.parentNode.getElementsByClassName(
        'details')[0];
    var detailsColumn = detailsNode.parentNode;
    var detailsRow = detailsColumn.parentNode;

    if (!detailsRow.classList.contains('expanded')) {
      detailsRow.classList.toggle('expanded');
      detailsColumn.setAttribute('colspan', 4);
      detailsNode.removeAttribute('hidden');
    } else {
      detailsNode.setAttribute('hidden', '');
      detailsColumn.removeAttribute('colspan');
      detailsRow.classList.toggle('expanded');
    }
  };

  function displaySyncEvents() {
    var entries = chrome.sync.log.entries;
    var eventTemplateContext = {
      eventList: entries,
    };
    var context = new JsEvalContext(eventTemplateContext);
    jstProcess(context, $('sync-events'));
  };

  function onLoad() {
    $('sync-events').addEventListener('click', toggleDisplay);
    chrome.sync.log.addEventListener('append', function(event) {
      displaySyncEvents();
    });
  }

  return {
    onLoad: onLoad
  };
});

document.addEventListener(
    'DOMContentLoaded', chrome.sync.events_tab.onLoad, false);
