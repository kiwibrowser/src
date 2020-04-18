/* Copyright (c) 2014 The Chromium Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

function onLoad() {
  var pnaclLog = document.getElementById('pnacl-log');
  var pnaclPlugin = document.getElementById('pnacl-plugin');
  var pnaclListener = document.getElementById('pnacl-listener');
  var textLog = document.getElementById('text-log');
  var textLogContainer = document.getElementById('text-log-container');

  var eventListeners = new EventListeners(pnaclLog, textLog,
                                          pnaclPlugin, pnaclListener);
  eventListeners.activate();

  document.getElementById('clear-log').addEventListener(
      'click',
      function() {
        pnaclLog.innerText = '';
        textLog.innerText = '';
      },
      false);
  document.getElementById('show-log').addEventListener(
      'click',
      function() {
        eventListeners.deactivate();
        textLogContainer.hidden = false;

        var selection = window.getSelection();
        var range = document.createRange();
        range.selectNodeContents(textLog);
        selection.removeAllRanges();
        selection.addRange(range);
      },
      false);
  document.getElementById('hide-log').addEventListener(
      'click',
      function() {
        eventListeners.activate();
        textLogContainer.hidden = true;
      },
      false);
}

window.addEventListener('load', onLoad, false);
