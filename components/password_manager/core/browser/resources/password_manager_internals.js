// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

function addSavePasswordProgressLog(logText) {
  var logDiv = $('log-entries');
  if (!logDiv)
    return;
  logDiv.appendChild(document.createElement('hr'));
  var textDiv = document.createElement('div');
  textDiv.innerText = logText;
  logDiv.appendChild(textDiv);
}

function notifyAboutIncognito(isIncognito) {
  document.body.dataset.incognito = isIncognito;
}
