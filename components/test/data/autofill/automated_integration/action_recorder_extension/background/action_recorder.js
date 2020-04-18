// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// global FIELD_TYPES, SITES_TO_VISIT, IndentedTextFactory, ActionSet, ByXPath,
// Open, SetContext, Type, Select, Click, TypedField, ValidateFields
'use strict';

class ActionRecorder {
  constructor() {
    this._siteIndex = 0;
    this.cancel();

    this._startListeners();
  }

  start(url) {
    this._actionSet = new ActionSet(url);
    this._createRecordingContextMenus();

    console.log('Started recording.');
  }

  stop() {
    if (!this.isRecording()) {
      console.error(
        'Attempted to stop recording when no action set was active.');
      return;
    }

    this._createIdleContextMenus();

    // Remove all redundant actions
    this._actionSet.optimize();

    this._copyText(this._actionSet.toString());
    this.cancel();
    console.log('Stopped recording.');
  }

  cancel() {
    this._actionSet = null;
    this._lastRightClick = null;
    this._typedFields = [];

    this._createIdleContextMenus();
  }

  isRecording() {
    return this._actionSet !== null;
  }

  /**
   * Copies a string to the system clipboard.
   * Multiline strings are supported.
   *
   * @param  {String} text Target string
   */
  _copyText(text) {
    const input = document.createElement('textarea');
    document.body.appendChild(input);
    input.value = text;
    input.focus();
    input.select();
    document.execCommand('Copy');
    input.remove();

    const iconUrl = 'icons/icon_' +
      (this.isRecording() ? 'recording' : 'idle') +
      '128.png';

    chrome.notifications.create(undefined, {
      type: 'basic',
      iconUrl: iconUrl,
      title: 'Action Recorder',
      message: 'Recorded script copied to clipboard.'
    });
  }

  /**
   * Process an action data event and add the appropriate Action object to the
   * active ActionSet.
   * @param  {Object}        actionData Data object to construct an Action
   *                                    instance with
   * @param  {MessageSender} sender     Chrome sender data object
   */
  _handleAction(actionData, sender) {
    if (!this.isRecording()) {
      console.warn('Actions cannot be added when recording is not active.');
      return;
    }
    if (!actionData) {
      console.error('Invalid action data.');
      return;
    }

    console.log('Action data received: ', actionData);

    let actionObject;

    let selector = new ByXPath(actionData.selector);

    switch (actionData.type) {
      case 'set-context':
        if (actionData.selector === 'None') {
          selector = actionData.selector;
        }
        if (actionData.args.length >= 2) {
          actionData.args[1] = new ByXPath(actionData.args[1]);
        }
        actionObject = new SetContext(selector, ...actionData.args);
        break;
      case 'type':
        actionObject = new Type(selector, ...actionData.args);
        break;
      case 'fill-email':
        actionObject = new Type(selector, 'GenEmail()', undefined, true);
        break;
      case 'fill-password':
        actionObject = new Type(selector, 'GenPassword()', undefined, true);
        break;
      case 'select':
        actionObject = new Select(selector, ...actionData.args);
        break;
      case 'left-click':
        actionObject = new Click(selector);
        break;
      case 'right-click':
        this._lastRightClick = {
          selector: selector,
          tabId: sender.tab.id,
          frameId: sender.frameId
        };
        return;
        break;
      default:
        console.error(`Unsupported action type: ${actionData.type}`);
        return;
    }

    this._actionSet.addAction(actionObject);
  }

  _startListeners() {
    /**
     * Add listener for messages from content scripts.
     */
    chrome.runtime.onMessage.addListener((request, sender, sendResponse) => {
      if (!request) {
        console.error('Invalid request from content script.', request);
        return;
      }

      switch (request.type) {
        case 'get-frame-info':
          let frameInfo = {
            tabId: sender.tab.id,
            frameId: sender.frameId
          };

          if (frameInfo.frameId === 0) {
            frameInfo.parentFrameId = -1;
            return sendResponse(frameInfo);
          }

          chrome.webNavigation.getFrame(frameInfo, (details) => {
            frameInfo.parentFrameId = details.parentFrameId;
            sendResponse(frameInfo);
          });

          return true;
        case 'forward-message-to-tab':
          chrome.tabs.sendMessage(...request.args);
          break;
        case 'recording-state-request':
          sendResponse({
            isRecording: this.isRecording()
          });
          break;
        case 'start-recording':
          this._getCurrentTab((err, tab) => {
            if (err) {
              console.error(err);
            } else {
              this.start(tab.url);
            }

            sendResponse({
              isRecording: this.isRecording()
            });
          });

          return true;
        case 'stop-recording':
          this.stop();
          sendResponse({
            isRecording: this.isRecording()
          });
          break;
        case 'cancel-recording':
          this.cancel();
          sendResponse({
            isRecording: this.isRecording()
          });
          break;
        case 'next-site':
          if (this.isRecording()) {
            console.error('Cannot go to next site when recording.');
            return;
          }
          this._visitSite(this._nextSite());
          break;
        case 'action':
          this._handleAction(request.data, sender);
          break;
        case 'actions':
          for (let i = 0; i < request.data.length; i++) {
            this._handleAction(request.data[i], sender);
          }
          break;
        default:
          console.error(`Unknown request type: ${request.type}`);
      }
    });
  }

  _getCurrentTab(cb) {
    chrome.tabs.query({
      active: true,
      currentWindow: true
    }, (tabs) => {
      if (tabs.length === 0) {
        cb(new Error('Unable to retrieve current tab.'));
      } else {
        cb(null, tabs[0]);
      }
    });
  }

  /**
   * Add a validation action to the active action set from the current
   * collection of typed fields.
   */
  _constructValidationAction() {
    if (!this.isRecording()) {
      console.warn('Actions cannot be added when recording is not active.');
      return;
    }

    const typedFields =
      Object.keys(this._typedFields).map(key => this._typedFields[key]);

    console.log('Creating field type validation action: ', typedFields);

    this._actionSet.addAction(new ValidateFields(typedFields));

    this._typedFields = {};
  }

  /**
   * Sets a field type for the last right-clicked element on the active tab
   * (thus for the element for which the context menu was created).
   *
   * @param {String} fieldType Identifier of the editable input's field type
   */
  _setFieldTypeForEditable(fieldType) {
    if (!this.isRecording()) {
      console.warn('Field types cannot be set when recording is not active.');
      return;
    }

    if (this._lastRightClick === null) {
      return console.error('No right click data available');
    }

    if (fieldType === 'NONE') {
      fieldType = null;
    }

    const selector = this._lastRightClick.selector;

    console.log(`Field Type '${fieldType}' received for: ${selector}`);

    this._typedFields[selector] = new TypedField(selector, fieldType);
  }

  /**
   * Generates a email fill action, and enters equivalent data to the target
   * element.
   */
  _fillEmail() {
    if (!this.isRecording()) {
      console.warn('Email can only be generated when recording is active.');
      return;
    }

    if (this._lastRightClick === null) {
      return console.error('No right click data available');
    }

    const clickData = this._lastRightClick;

    chrome.tabs.sendMessage(
      clickData.tabId, {
        type: 'fill-email',
        selector: clickData.selector._xPath
      }, {
        frameId: clickData.frameId
      });
  }

  /**
   * Generates a password fill action, and enters equivalent data to the target
   * element.
   */
  _fillPassword() {
    if (!this.isRecording()) {
      console.warn('Password can only be generated when recording is active.');
      return;
    }

    if (this._lastRightClick === null) {
      return console.error('No right click data available');
    }

    const clickData = this._lastRightClick;

    chrome.tabs.sendMessage(
      clickData.tabId, {
        type: 'fill-password',
        selector: clickData.selector._xPath
      }, {
        frameId: clickData.frameId
      });
  }

  /**
   * Generate context menus for managing field types.
   * A user must be able to select all available field types for an input
   * element, and then generate a field validation action once all fields have
   * been marked.
   */
  _createRecordingContextMenus() {
    chrome.contextMenus.removeAll(() => {
      const parentContextMenu = chrome.contextMenus.create({
        title: 'Input Field Type',
        contexts: ['all']
      });
      const specialFillContextMenu = chrome.contextMenus.create({
        title: 'Special Fill',
        contexts: ['editable']
      });
      chrome.contextMenus.create({
        title: 'Fill Email',
        contexts: ['editable'],
        parentId: specialFillContextMenu,
        onclick: (info, tab) => {
          this._fillEmail();
        }
      });
      chrome.contextMenus.create({
        title: 'Fill Password',
        contexts: ['editable'],
        parentId: specialFillContextMenu,
        onclick: (info, tab) => {
          this._fillPassword();
        }
      });
      chrome.contextMenus.create({
        title: 'Validate Field Types',
        contexts: ['all'],
        onclick: (info, tab) => {
          this._constructValidationAction();
        }
      });
      chrome.contextMenus.create({
        title: 'Forget Field Types',
        contexts: ['all'],
        onclick: (info, tab) => {
          this._typedFields = {};
          console.log('Field type knowledge reset.');
        }
      });
      chrome.contextMenus.create({
        title: 'Stop and Copy',
        contexts: ['all'],
        onclick: (info, tab) => {
          this.stop();
        }
      });
      chrome.contextMenus.create({
        title: 'Cancel',
        contexts: ['all'],
        onclick: (info, tab) => {
          this.cancel();
        }
      });

      for (let i = 0; i < FIELD_TYPES.length; i++) {
        const fieldType = FIELD_TYPES[i];

        chrome.contextMenus.create({
          title: fieldType,
          parentId: parentContextMenu,
          contexts: ['all'],
          onclick: (info, tab) => {
            this._setFieldTypeForEditable(fieldType);
          }
        });
      }
    });
  }

  _createIdleContextMenus() {
    chrome.contextMenus.removeAll(() => {
      chrome.contextMenus.create({
        title: 'Start',
        contexts: ['all'],
        onclick: (info, tab) => {
          this.start(info.pageUrl);
        }
      });
      chrome.contextMenus.create({
        title: 'Next Site',
        contexts: ['all'],
        onclick: (info, tab) => {
          this._visitSite(this._nextSite());
        }
      });
    });
  }

  /**
   * Retrieve the next recommended "top 100" website url.
   * Note that this will also advance the site index for the next call.
   *
   * @return {String} Complete url for the next top 100 site
   */
  _nextSite() {
    if (this._siteIndex >= SITES_TO_VISIT.length) {
      console.error('All recommended sites have been visited.');
      return;
    }

    const hostname = SITES_TO_VISIT[this._siteIndex++];
    return `http://${hostname}`;
  }

  /**
   * Redirect the currently active tab to a given url.
   * @param  {String} url Complete target url (including protocol)
   */
  _visitSite(url) {
    chrome.tabs.update({
      url: url
    });
  }
}

const actionRecorder = new ActionRecorder();
