// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

'use strict';

class PopupController {
  constructor() {
    this._startButton = document.getElementById('start');
    this._nextSiteButton = document.getElementById('next-site');
    this._stopButton = document.getElementById('stop');
    this._cancelButton = document.getElementById('cancel');

    this._startListeners();
    this._getRecordingState();
  }

  startRecording() {
    chrome.runtime.sendMessage({
        type: 'start-recording'
      },
      (response) => this._handleRecordingResponse(response));
  }

  stopRecording() {
    chrome.runtime.sendMessage({
        type: 'stop-recording'
      },
      (response) => this._handleRecordingResponse(response));
  }

  cancelRecording() {
    chrome.runtime.sendMessage({
        type: 'cancel-recording'
      },
      (response) => this._handleRecordingResponse(response));
  }

  nextSite() {
    chrome.runtime.sendMessage({
      type: 'next-site'
    });
  }

  _getRecordingState() {
    chrome.runtime.sendMessage({
        type: 'recording-state-request'
      },
      (response) => this._handleRecordingResponse(response));
  }

  _handleRecordingResponse(response) {
    if (!response) {
      return;
    }

    this._setRecordingState(!!response.isRecording);
  }

  _startListeners() {
    this._startButton.addEventListener(
      'click', (event) => {
        this.startRecording();
      });

    this._stopButton.addEventListener(
      'click', (event) => {
        this.stopRecording();
      });

    this._cancelButton.addEventListener(
      'click', (event) => {
        this.cancelRecording();
      });

    this._nextSiteButton.addEventListener(
      'click', (event) => {
        this.nextSite();
      });
  }

  _setRecordingState(isRecording) {
    this._isRecording = isRecording;
    this._updateStyling();
  }

  _updateStyling() {
    let shownButton1, shownButton2, hiddenButton1, hiddenButton2;

    if (this._isRecording) {
      shownButton1 = this._stopButton;
      shownButton2 = this._cancelButton;
      hiddenButton1 = this._startButton;
      hiddenButton2 = this._nextSiteButton;
    } else {
      shownButton1 = this._startButton;
      shownButton2 = this._nextSiteButton;
      hiddenButton1 = this._stopButton;
      hiddenButton2 = this._cancelButton;
    }

    this._removeClass(shownButton1, 'hidden');
    this._removeClass(shownButton2, 'hidden');
    this._applyClass(hiddenButton1, 'hidden');
    this._applyClass(hiddenButton2, 'hidden');

    chrome.browserAction.setIcon({
      path: this._getIconUrl()
    });
  }


  _removeClass(element, className) {
    if (element.classList.contains(className)) {
      element.classList.remove(className);
    }
  }

  _applyClass(element, className) {
    element.classList.add(className);
  }

  _getIconUrl() {
    const iconUrlPrefix = '../icons/icon_' +
      (this._isRecording ? 'recording' : 'idle');

    return {
      '16': iconUrlPrefix + '16.png',
      '32': iconUrlPrefix + '32.png'
    };
  }
}

document.addEventListener('DOMContentLoaded', function() {
  const popupController = new PopupController();
});
