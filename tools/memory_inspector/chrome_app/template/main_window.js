// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

'use strict';

/**
 * Singleton object representing the main window of the Memory Inspector Chrome
 * App.
 * @constructor
 */
var MemoryInspectorWindow = function() {
  // HTML elements in the window.
  this.terminalElement_ = undefined;
  this.terminalButtonElement_ = undefined;
  this.contentsElement_ = undefined;
  this.inspectorViewElement_ = undefined;
  this.loadOverlayElement_ = undefined;
  this.loadMessageElement_ = undefined;
  this.loadDotsElement_ = undefined;
  this.phantomLoadDots_ = undefined;

  // Dots animation.
  this.loadDotsAnimationIntervalId_ = undefined;

  // Process manager.
  this.processManager_ = undefined;

  // Terminal.
  this.terminalVisible_ = false;
};

/** The threshold for terminal auto-scroll. */
MemoryInspectorWindow.AUTOSCROLL_THRESHOLD = 10;

/** The interval between steps of the loading dots animation (milliseconds). */
MemoryInspectorWindow.LOAD_DOTS_ANIMATION_INTERVAL = 500;

/** The maximum number of dots in the loading dots animation. */
MemoryInspectorWindow.MAX_LOAD_DOTS_COUNT = 3;

/** The interval between polls to the backend server (milliseconds). */
MemoryInspectorWindow.POLL_INTERVALL = 500;

/**
 * Initialize the main window. It will be shown once settings are retrieved
 * (asynchronous request).
 * @private
 */
MemoryInspectorWindow.prototype.initialize = function() {
  this.setUpGui_();
  this.startServerProcess_();
  this.retrieveSettings_();
  this.pollServer_();
};

/**
 * Set up the main window. This method retrieves relevant HTML elements, adds
 * corresponding event handlers, and starts animations.
 * @private
 */
MemoryInspectorWindow.prototype.setUpGui_ = function() {
  // Retrieve HTML elements in the window.
  this.terminalElement_ = document.getElementById('terminal');
  this.terminalButtonElement_ = document.getElementById('terminal_button');
  this.contentsElement_ = document.getElementById('contents');
  this.inspectorViewElement_ = document.getElementById('inspector_view');
  this.loadOverlayElement_ = document.getElementById('load_overlay');
  this.loadMessageElement_ = document.getElementById('load_message');
  this.loadDotsElement_ = document.getElementById('load_dots');
  this.phantomLoadDotsElement_ = document.getElementById('phantom_load_dots');

  // Hook up the terminal toggle button.
  this.terminalButtonElement_.addEventListener('click',
      this.toggleTerminal_.bind(this));

  // Print app name and version in the terminal.
  var manifest = chrome.runtime.getManifest();
  this.printInfo_(manifest.name + ' (version ' + manifest.version + ')\n');

  // Start the loading dots animation.
  this.loadDotsAnimationIntervalId_ = window.setInterval(
      this.animateLoadDots_.bind(this),
      MemoryInspectorWindow.LOAD_DOTS_ANIMATION_INTERVAL);
};

/**
 * Toggle the terminal.
 * @private
 */
MemoryInspectorWindow.prototype.toggleTerminal_ = function() {
  if (this.terminalVisible_) {
    this.hideTerminal_();
  } else {
    this.showTerminal_();
  }
  this.storeSettings_();
};

/**
 * Show the terminal.
 * @private
 */
MemoryInspectorWindow.prototype.showTerminal_ = function() {
  // Scroll to the bottom.
  this.terminalElement_.scrollTop = terminal.scrollHeight;
  document.body.classList.add('terminal_visible');
  this.terminalVisible_ = true;
};

/**
 * Hide the terminal.
 * @private
 */
MemoryInspectorWindow.prototype.hideTerminal_ = function() {
  document.body.classList.remove('terminal_visible');
  this.terminalVisible_ = false;
};

/**
 * Stop the loading dots animation.
 * @private
 */
MemoryInspectorWindow.prototype.stopLoadDotsAnimation_ = function() {
  window.clearInterval(this.loadDotsAnimationIntervalId_);
  this.loadDotsAnimationIntervalId_ = undefined;
};

/**
 * Animate the loading dots.
 * @private
 */
MemoryInspectorWindow.prototype.animateLoadDots_ = function() {
  if (this.loadDotsElement_.innerText.length >=
      MemoryInspectorWindow.MAX_LOAD_DOTS_COUNT) {
    this.loadDotsElement_.innerText = '.';
  } else {
    this.loadDotsElement_.innerText += '.';
  }
};

/**
 * Start the server process inside PNaCl.
 * @private
 */
MemoryInspectorWindow.prototype.startServerProcess_ = function() {
  // Create and hook up a NaCl process manager.
  var mgr = this.processManager = new NaClProcessManager();
  mgr.setStdoutListener(this.onServerStdout_.bind(this));
  mgr.setErrorListener(this.onServerError_.bind(this));
  mgr.setRootProgressListener(this.onServerProgress_.bind(this));
  mgr.setRootLoadListener(this.onServerLoad_.bind(this));

  // Set dummy terminal size.
  // (see https://code.google.com/p/naclports/issues/detail?id=186)
  mgr.onTerminalResize(200, 200);

  // Spawn the server process.
  this.printInfo_('Spawning ' + MemoryInspectorConfig.ARGV.join(' ') + '\n');
  mgr.spawn(
      MemoryInspectorConfig.NMF,
      MemoryInspectorConfig.ARGV,
      MemoryInspectorConfig.ENV,
      MemoryInspectorConfig.CWD,
      'pnacl',
      null /* parent */,
      this.onServerProcessSpawned_.bind(this));
};

/**
 * Listener called when the server process is spawned.
 * @private
 * @param {number} pid The PID of the spawned server process or error code if
 *     negative.
 */
MemoryInspectorWindow.prototype.onServerProcessSpawned_ = function(pid) {
  this.processManager.waitpid(pid, 0, this.onServerProcessExit_.bind(this));
};

/**
 * Listener called when the server process exits.
 * @private
 * @param {number} pid The PID of the server process or an error code on error.
 * @param {number} status The exit code of the server process.
 */
MemoryInspectorWindow.prototype.onServerProcessExit_ = function(pid, status) {
  this.printInfo_('NaCl module exited with status ' + status + '\n');
};

/**
 * Listener called when an stdout event is received from the server process.
 * @private
 * @param {string} msg The string sent to stdout.
 */
MemoryInspectorWindow.prototype.onServerStdout_ = function(msg) {
  this.printOutput_(msg);
};

/**
 * Listener called when an error event is received from the server process.
 * @private
 * @param {string} cmd The name of the process with the error.
 * @param {string} err The error message.
 */
MemoryInspectorWindow.prototype.onServerError_ = function(cmd, err) {
  this.printError_(cmd + ': ' + err + '\n');
};

/**
 * Listener called when a part of the server NaCl module has been loaded.
 * @private
 * @param {string} url The URL that is being loaded.
 * @param {boolean} lengthComputable Is our progress quantitatively measurable?
 * @param {number} loaded The number of bytes that have been loaded.
 * @param {number} total The total number of bytes to be loaded.
 */
MemoryInspectorWindow.prototype.onServerProgress_ = function(url,
    lengthComputable, loaded, total) {
  if (url === undefined) {
    return;
  }

  var message = 'Loading ' + url.substring(url.lastIndexOf('/') + 1);
  if (lengthComputable && total > 0) {
    var percentLoaded = Math.round(loaded / total * 100);
    var kbLoaded = Math.round(loaded / 1024);
    var kbTotal = Math.round(total / 1024);
    message += ' [' + kbLoaded + '/' + kbTotal + ' KiB ' + percentLoaded + '%]';
  }
  this.printInfo_(message + '\n');
};

/**
 * Listener called when the server NaCl module has been successfully loaded.
 * @private
 */
MemoryInspectorWindow.prototype.onServerLoad_ = function() {
  this.printInfo_('NaCl module loaded\n');
};

/**
 * Print an output message in the terminal.
 * @private
 * @param {string} msg The text of the message.
 */
MemoryInspectorWindow.prototype.printOutput_ = function(msg) {
  this.printMessage_(msg, 'terminal_message_output');
};

/**
 * Print an info message in the terminal.
 * @private
 * @param {string} msg The text of the message.
 */
MemoryInspectorWindow.prototype.printInfo_ = function(msg) {
  this.printMessage_(msg, 'terminal_message_info');
};

/**
 * Print an error message in the terminal.
 * @private
 * @param {string} msg The text of the message.
 */
MemoryInspectorWindow.prototype.printError_ = function(msg) {
  this.printMessage_(msg, 'terminal_message_error');
};

/**
 * Print a message of a given type in the terminal.
 * @private
 * @param {string} msg The text of the message.
 * @param {string} cls The CSS class of the message.
 */
MemoryInspectorWindow.prototype.printMessage_ = function(msg, cls) {
  // Determine whether we are at the bottom of the terminal.
  var scrollBottom = this.terminalElement_.scrollTop +
      this.terminalElement_.clientHeight;
  var autoscrollBottomMin = this.terminalElement_.scrollHeight -
      MemoryInspectorWindow.AUTOSCROLL_THRESHOLD;
  var shouldScroll = scrollBottom >= autoscrollBottomMin;

  // Append the message to the terminal.
  var messageElement = document.createElement('span');
  messageElement.innerText = msg;
  messageElement.classList.add(cls);
  this.terminalElement_.appendChild(messageElement);

  // If we were at the bottom of the terminal, scroll to the bottom again.
  if (shouldScroll) {
    this.terminalElement_.scrollTop = this.terminalElement_.scrollHeight;
  }
};

/**
 * Asynchronously retrieve main window settings from local storage.
 * @private
 */
MemoryInspectorWindow.prototype.retrieveSettings_ = function() {
  chrome.storage.local.get('terminal_visible',
      this.onSettingsRetrieved_.bind(this));
};

/**
 * Asynchronously store main window settings in local storage.
 * @private
 */
MemoryInspectorWindow.prototype.storeSettings_ = function() {
  var settings = {};
  settings['terminal_visible'] = this.terminalVisible_;
  chrome.storage.local.set(settings);
};

/**
 * Listener called when main window settings were retrieved from local storage.
 * It saves them and finally shows the app window.
 * @private
 * @param {Object<*>} settings The retrieved settings.
 */
MemoryInspectorWindow.prototype.onSettingsRetrieved_ = function(settings) {
  if (chrome.runtime.lastError === undefined && settings['terminal_visible']) {
    // Skip CSS animations by temporarily hiding everything.
    document.body.style.display = 'none';
    this.showTerminal_();
    document.body.style.display = 'block';
  }

  // We are finally ready to show the window.
  chrome.app.window.current().show();
};

/**
 * Keep polling the backend server until it is reachable. The requests are
 * asynchronous.
 * @private
 */
MemoryInspectorWindow.prototype.pollServer_ = function() {
  var requestUrl = 'http://127.0.0.1:' + MemoryInspectorConfig.PORT +
      '?timestamp=' + new Date().getTime();
  var request = new XMLHttpRequest();
  request.addEventListener('readystatechange',
      this.onPollRequestStateChange_.bind(this, request));
  request.open('GET', requestUrl);
  request.send();
};

/**
 * Listener called when the state of the server request changes. If the request
 * is complete and successful, the inspector view is navigated to the server.
 * If unsuccessful, the request is repeated. Incomplete requests are ignored.
 * @private
 * @param {XMLHttpRequest} request The request.
 */
MemoryInspectorWindow.prototype.onPollRequestStateChange_ = function(request) {
  // We wait until the request is complete.
  if (request.readyState !== XMLHttpRequest.DONE) {
    return;
  }

  // If the request was not successful, try again.
  if (request.status !== 200) {
    console.log('Inspector server unreachable. Trying again...');
    setTimeout(this.pollServer_.bind(this),
        MemoryInspectorWindow.POLL_INTERVALL);
    return;
  }

  // Stop the animation, load the inspector, and display it.
  console.log('Inspector server ready. Loading the inspector view...');
  this.inspectorViewElement_.addEventListener('loadstop',
      this.onInspectorLoaded_.bind(this));
  this.inspectorViewElement_.addEventListener('loadcommit',
      this.onInspectorNavigated_.bind(this));
  this.inspectorViewElement_.src = 'http://127.0.0.1:' +
      MemoryInspectorConfig.PORT;
};

/**
 * Listener called when the inspector is loaded. It stops the loading dots
 * animation, shows the inspector view, and fades out the loading overlay.
 * @private
 */
MemoryInspectorWindow.prototype.onInspectorLoaded_ = function() {
  this.stopLoadDotsAnimation_();
  document.body.classList.add('inspector_view_visible');
};

/**
 * Listener called when the inspector view navigates to the inspector website.
 * @private
 */
MemoryInspectorWindow.prototype.onInspectorNavigated_ = function() {
  this.inspectorViewElement_.executeScript({
    'file': 'inject.js',
    'runAt': 'document_start'
  });
};

window.addEventListener('load', function() {
  // Create the singleton MemoryInspectorWindow instance and initialize it.
  var mainWindow = new MemoryInspectorWindow();
  mainWindow.initialize();

  // Make the instance global for debugging purposes.
  window.mainWindow = mainWindow;
});

