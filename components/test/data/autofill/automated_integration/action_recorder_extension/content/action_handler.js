// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

'use strict';

const LEFT_BUTTON = 0;
const RIGHT_BUTTON = 2;

/**
 * Create an editing timer that will execute a command if the timer is not
 * triggered within a given timeout interval.
 */
class EditingTimer {
  /**
   * @constructor
   * @param  {Element}  element  Target editable element
   * @param  {Function} callback Function to call if the timeout occurs
   * @param  {Number}   interval Timeout interval in ms
   */
  constructor(element, callback, interval) {
    this._element = element;
    this._timer = 0;
    this._callback = callback;
    this._interval = interval;

    this._mouseOutListener = () => { this.cancel(true); };

    this._addMouseOutListener();

    this._element.addEventListener('keyup', (event) => {
      this.trigger();
      this._addMouseOutListener();
    });
  }

  /**
   * Start/postpone the timeout
   */
  trigger() {
    this._removeMouseOutListener();

    clearTimeout(this._timer);
    this._timer = setTimeout(this._callback, this._interval);
  }

  /**
   * Cancel the timeout immediately and optionally fire the callback.
   * @param  {Boolean*} executeCallback Callback will be executed if truthy
   */
  cancel(executeCallback) {
    this._removeMouseOutListener();
    clearTimeout(this._timer);

    if (executeCallback) {
      this._callback();
    }
  }

  _addMouseOutListener() {
    this._element.addEventListener('mouseout', this._mouseOutListener);
  }

  _removeMouseOutListener() {
    this._element.removeEventListener('mouseout', this._mouseOutListener);
  }
}

class ActionHandler {
  constructor() { this._startListeners(); }

  /**
   * Returns true if |element| is probably a clickable element.
   *
   * @param  {Element} element The element to be checked
   * @return {boolean}         True if the element is probably clickable
   */
  _isClickableElementOrInput(element) {
    return (
        element.tagName == 'INPUT' || element.tagName == 'A' ||
        element.tagName == 'BUTTON' || element.tagName == 'SUBMIT' ||
        element.getAttribute('href'));
  }

  /**
   * Returns |element|, if |element| is clickable element. Otherwise, returns
   * clickable children or parent of the given element |element|.
   * Font element might consume a user click, but ChromeDriver will be unable to
   * click on the font element, so find an actually clickable element to target.
   *
   * @param  {Element} element The element where a clickable tag should be find.
   * @return {Element}         The clicable element.
   */
  _fixElementSelection(element) {
    if (this._isClickableElementOrInput(element)) {
      return element;
    }

    const clickableChildren = element.querySelectorAll(
        ':scope input, :scope a, :scope button, :scope submit, :scope [href]');

    // If any of the children are clickable, use them.
    if (clickableChildren.length > 0) {
      return clickableChildren[0];
    }

    // Check if any of the parent elements (within 5) are clickable.
    let parent = element;
    for (let i = 0; i < 5; i++) {
      parent = parent.parentElement;
      if (!parent) {
        break;
      }

      if (this._isClickableElementOrInput(parent)) {
        return parent;
      }
    }

    return element;
  }

  /**
   * Send the message object to the appropriate parent.
   *
   * If this is not the root frame in the tab then the message will be sent to
   * the direct parent frame. However, if this is the root frame, the message
   * is sent to the background script.
   *
   * @param  {Object} object Message payload
   */
  _sendMessageToParent(object) {
    if (this._frameId === 0) {
      chrome.runtime.sendMessage(object);
    } else {
      if (object) {
        object.url = document.location.href;
      }

      chrome.runtime.sendMessage({
        type: 'forward-message-to-tab',
        args: [this._tabId, object, {frameId: this._parentFrameId}]
      });
    }
  }

  /**
   * Construct action an object.
   *
   * @param  {String} type     Action type code (ex. 'type', 'left-click')
   * @param  {String} selector XPath reference to the element within the current
   *                           frame
   * @param  {...*}   ...args  Remaining args that're to be applied to the
   *                           action's constructor
   */
  _createAction(type, selector, ...args) {
    return {type: type, selector: selector, args: args};
  }

  _registerTypeAction(element) {
    const selector = xPathTools.xPath(element, true);
    const value = element.value;

    this._log(`Typing detected on: ${selector} with '${value}'`);

    this._sendMessageToParent(
        {type: 'action', data: this._createAction('type', selector, value)});
  }

  _registerSelectAction(event) {
    const element = event.target;
    const selector = xPathTools.xPath(element, true);

    this._log('Select detected on:', selector);

    this._sendMessageToParent({
      type: 'action',
      data: this._createAction('select', selector, element.value)
    });
  }

  /**
   * Register the child frame's action by switching context to and from that of
   * the frame and sending it to the parent of this frame.
   *
   * @param  {Object} action Action data object to wrap with context switching
   * @param  {String} url    URL of child iframe (used to select element)
   */
  _registerChildAction(action, url) {
    this._registerChildActions([action], url);
  }

  /**
   * Register the child frame's actions by switching context to and from that of
   * the frame and sending it to the parent of this frame.
   *
   * @param  {Array} actions Array of action data objects to wrap with context
   *                         switching
   * @param  {String} url    URL of child iframe (used to select element)
   */
  _registerChildActions(actions, url) {
    const element = document.querySelector(`iframe[src="${url}"]`);

    if (element === null) {
      return console.error(
          `[Frame: ${this._frameId}] Unable to find iframe for child actions:`,
          url);
    }

    const selector = xPathTools.xPath(element, true);

    actions.unshift(this._createAction('set-context', selector));
    actions.push(
        this._createAction('set-context', 'None', undefined, selector));

    this._sendMessageToParent({type: 'actions', data: actions});
  }

  /**
   * Create a listener for a given editable element.
   *
   * If the element is a text input field then typing events will be generated
   * after 1000ms of inactivity (once typing has commenced), or if the mouse
   * leaves the field.
   *
   * @param {Element} element Target DOM Element
   */
  _addEditableElementListener(element) {
    switch (element.localName) {
      case 'input':
      case 'textarea':
        switch (element.getAttribute('type')) {
          case 'radio':
          case 'submit':
            break;
          default:
            const editTimer = new EditingTimer(
                element, () => { this._registerTypeAction(element); }, 1000);
        }
        break;
      case 'select':
        element.addEventListener(
            'change', (event) => { this._registerSelectAction(event); });
        break;
    }
  }

  /**
   * Create a mutation observer that watches for elements that are added
   * post-DOMContentLoaded by the site's custom js.
   *
   * This will attach change listeners on all new editable elements.
   *
   * Note: You must also attach listeners to existing elements.
   */
  _setupMutationObserver() {
    this._mutationObserver = new MutationObserver((mutations) => {
      mutations.forEach(mutation => {
        mutation.addedNodes.forEach(newNode => {
          if (newNode.nodeType === Node.ELEMENT_NODE) {
            this._addEditableElementListener(newNode);
          }
        });
      });
    });

    this._mutationObserver.observe(document, {childList: true});
  }

  /**
   * Retrieves data about this tab & frame from the background script.
   * @param  {Function} cb Called once the data has been retrieved
   */
  _setupChildMessageHandler(cb) {
    chrome.runtime.sendMessage({type: 'get-frame-info'}, (response) => {
      this._tabId = response.tabId;
      this._frameId = response.frameId;
      this._parentFrameId = response.parentFrameId;
      this._log(`Parent frame id is ${this._parentFrameId}`);

      cb();
    });
  }

  /**
   * Setup all the event & message listeners that're required to detect and
   * report actions.
   */
  _startListeners() {
    this._setupChildMessageHandler(() => {
      this._setupMutationObserver();

      const editableElements =
          document.querySelectorAll('input,textarea,select');
      editableElements.forEach(
          (element) => { this._addEditableElementListener(element); });

      /**
       * Listen for messages from the background script
       */
      chrome.runtime.onMessage.addListener((request, sender, sendResponse) => {
        if (!request) {
          console.error('Invalid request from background script.', request);
          return;
        }

        let element = null;

        switch (request.type) {
          case 'fill-element':
            if (!request.selector) {
              return console.error(
                  'Unable to fill element, invalid request selector', request);
            }

            this._log('Filling element', request.selector);

            element = this._getElementByXPath(request.selector);

            if (element === null) {
              return console.error('Unable to fill element, not found');
            }

            this._log(`Filling element with '${request.content}'`);

            element.value = request.content;

            this._registerTypeAction(element);
            break;
          case 'fill-email':
            if (!request.selector) {
              return console.error(
                  'Unable to fill element, invalid request selector', request);
            }

            this._log('Filling element', request.selector);

            element = this._getElementByXPath(request.selector);

            if (element === null) {
              return console.error('Unable to fill element, not found');
            }

            this._log('Filling element with random email');

            element.value = this._generateEmail();

            this._sendMessageToParent({
              type: 'action',
              data: this._createAction('fill-email', request.selector)
            });
            break;
          case 'fill-password':
            if (!request.selector) {
              return console.error(
                  'Unable to fill element, invalid request selector', request);
            }

            this._log('Filling element', request.selector);

            element = this._getElementByXPath(request.selector);

            if (element === null) {
              return console.error('Unable to fill element, not found');
            }

            this._log('Filling element with random password');

            element.value = this._generatePassword();

            this._sendMessageToParent({
              type: 'action',
              data: this._createAction('fill-password', request.selector)
            });
            break;
          case 'action':
            // Handle a child frame's action
            this._log('Child frame action received:', request.data);
            this._registerChildAction(request.data, request.url);
            break;
          case 'actions':
            // Handle a child frame's actions
            this._log('Child frame actions received:', request.data);
            this._registerChildActions(request.data, request.url);
            break;
          default:
            console.error(`Unknown request type: ${request.type}`);
        }
      });
      /**
       * Click event listener
       */
      document.addEventListener('mousedown', (event) => {
        const element = this._fixElementSelection(event.target);
        const selector = xPathTools.xPath(element, true);

        let type;

        switch (event.button) {
          case LEFT_BUTTON:
            this._log(`Left-click detected on: ${selector}`);

            type = 'left-click';

            this._sendMessageToParent(
                {type: 'action', data: this._createAction(type, selector)});
            break;
          case RIGHT_BUTTON:
            this._log(`Right-click detected on: ${selector}`);

            type = 'right-click';

            chrome.runtime.sendMessage(
                {type: 'action', data: this._createAction(type, selector)});
            break;
          default:
            return console.error(
                'Unknown button used for mousedown:', event.button);
        }
      }, true);
    });
  }

  _getElementByXPath(xpath) {
    return document
        .evaluate(
            xpath, document, null, XPathResult.FIRST_ORDERED_NODE_TYPE, null)
        .singleNodeValue;
  }

  _generateString(length) {
    length = length || 8;

    const lowerCaseChars = 'abcdefghijklmnopqrstuvwxyz';
    let result = '';
    for (let i = 0; i < length; i++) {
      let charIndex = Math.floor(Math.random() * lowerCaseChars.length);
      result += lowerCaseChars[charIndex];
    }
    return result;
  }

  _generateEmail() {
    return `${this._generateString()}@${this._generateString()}.co.uk`;
  }

  _generatePassword() { return `${this._generateString()}!234&`; }

  _log(message, ...args) {
    console.log(`[Frame: ${this._frameId}] ${message}`, ...args);
  }
}

const actionHandler = new ActionHandler();
