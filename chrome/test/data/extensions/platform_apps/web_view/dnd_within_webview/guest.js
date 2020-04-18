// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

var embedder = null;
var $ = function(id) {
  return document.getElementById(id);
};

var LOG = function(msg) {
  window.console.log(msg);
};

var dragStarted = false;
var dropData = 'Uninitialized';
var dropped = false;
var isDragInterrupted = false;
var sentConnectedMessage = false;

// Number of mousemove events recorded after (most recent) mousedown and before
// (most recent) mouseup.
var numDocumentMouseMoves = 0;

var sendGuestGotDrop = function() {
  embedder.postMessage(JSON.stringify(['guest-got-drop', dropData]), '*');
};

window.addEventListener('resize', function(e) {
  if (!embedder) {
    return;
  }
  resized = true;
  embedder.postMessage(JSON.stringify(['resized']), '*');
});

var maybeSendConnected = function() {
  LOG('maybeSendConnected, sentConnectedMessage: ' + sentConnectedMessage);
  if (!sentConnectedMessage && embedder) {
    LOG('Inside');
    sentConnectedMessage = true;
    embedder.postMessage(JSON.stringify(['connected']), '*');
  }
};

var resetState = function() {
  LOG('guest.resetState');
  resetSelection();

  dropped = false;
  dragStarted = false;
  isDragInterrupted = false;

  embedder.postMessage(JSON.stringify(['resetStateReply']), '*');
};

window.addEventListener('message', function(e) {
  embedder = e.source;
  var data = JSON.parse(e.data)[0];
  if (data == 'create-channel') {
    maybeSendConnected();
  } else if (data == 'resetState') {
    resetState();
  }
});

var doPostMessage = function(msg) {
  embedder.postMessage(JSON.stringify([msg]), '*');
};

var resetSelection = function() {
  var selection = window.getSelection();
  var range = document.createRange();
  range.selectNodeContents($('dragMe'));
  selection.removeAllRanges();
  selection.addRange(range);
  LOG('Restored selection');
};

var setUpGuest = function() {
  window.console.log('n.guest.setUpGuest');

  document.body.style.width = '300px';
  document.body.style.background = '#CCCCCC';

  document.body.innerHTML +=
      '<div class="dragMe" id="dragMe">Drop me</div>' +
      '<div class="destination" id="dest"></div>';

  document.body.addEventListener('mousemove', function(e) {
    LOG('document.mousemove: ' + e.clientX + ', ' + e.clientY);
    ++numDocumentMouseMoves;
  });
  document.body.addEventListener('mousedown', function(e) {
    LOG('document.mousedown: ' + e.clientX + ', ' + e.clientY);
    numDocumentMouseMoves = 0;
  });
  document.body.addEventListener('mouseup', function(e) {
    LOG('document.mouseup: ' + e.clientX + ', ' + e.clientY);
    LOG('numDocumentMouseMoves: ' + numDocumentMouseMoves);
    LOG('dragStarted: ' + dragStarted +
        ', isDragInterrupted: ' + isDragInterrupted);
    if (!dragStarted && !isDragInterrupted) {
      LOG('Guest drag operation was interrupted by an unexpected mouseup');
      isDragInterrupted = true;
      embedder.postMessage(JSON.stringify(
          ['guest-got-drop', 'drag-interrupted-by-mouseup']), '*');
    }

    numDocumentMouseMoves = 0;
  });

  // Select the text to drag.
  resetSelection();

  // Whole bunch of drag/drop events on destination node follows.
  // We only need to make sure we preventDefault in dragenter and
  // dragover.
  var destNode = $('dest');
  var srcNode = $('dragMe');

  srcNode.addEventListener('mousemove', function(e) {
    LOG('s.mousemove ' + e.clientX + ', ' + e.clientY);
  });
  srcNode.addEventListener('mousedown', function(e) {
    LOG('s.mousedown ' + e.clientX + ', ' + e.clientY);
  });
  srcNode.addEventListener('mouseup', function(e) {
    LOG('s.mouseup ' + e.clientX + ', ' + e.clientY);
  });

  destNode.addEventListener('dragenter', function(e) {
    LOG('d.dragenter');
    e.preventDefault();
  }, false);

  destNode.addEventListener('dragover', function(e) {
    LOG('d.dragover');
    e.preventDefault();
  }, false);

  destNode.addEventListener('dragleave', function(e) {
    LOG('d.dragleave');
  }, false);

  destNode.addEventListener('drop', function (e) {
    LOG('d.drop');
    dropped = true;
    dropData = e.dataTransfer.getData('Text');
    sendGuestGotDrop();
  }, false);

  destNode.addEventListener('dragend', function (e) {
    LOG('d.dragend');
  }, false);

  srcNode.addEventListener('dragstart', function(e) {
    LOG('s.dragstart');
    dragStarted = true;
  }, false);
  srcNode.addEventListener('dragend', function(e) {
    LOG('s.dragend');
  }, false);
  srcNode.addEventListener('dragexit', function(e) {
    LOG('s.dragexit');
  }, false);
  srcNode.addEventListener('dragleave', function(e) {
    LOG('s.dragleave');
  }, false);

  destNode.addEventListener('dragexit', function(e) {
    LOG('s.dragexit');
  }, false);

  LOG('set up complete');
};

setUpGuest();
