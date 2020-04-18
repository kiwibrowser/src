// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

'use strict';

/**
 * Singleton object representing the Memory Inspector Chrome App.
 * @constructor
 */
var MemoryInspectorApp = function() {
  this.window_ = undefined;
};

/** Main window parameters. */
MemoryInspectorApp.WINDOW_URL = 'main_window.html';
MemoryInspectorApp.WINDOW_ID = 'main';
MemoryInspectorApp.WINDOW_WIDTH = Math.min(screen.width, 1200);
MemoryInspectorApp.WINDOW_HEIGHT = Math.min(screen.height, 800);

/**
 * Launch the Memory Inspector. If it is already running, focus the main window.
 */
MemoryInspectorApp.prototype.launch = function() {
  if (this.window_ === undefined) {
    this.start_();
  } else {
    this.focus_();
  }
};

/**
 * Start the Memory Inspector by creating the main window.
 * @private
 */
MemoryInspectorApp.prototype.start_ = function() {
  var options = {
    'id': MemoryInspectorApp.WINDOW_ID,
    'bounds': {
      'width': MemoryInspectorApp.WINDOW_WIDTH,
      'height': MemoryInspectorApp.WINDOW_HEIGHT
    },
    'hidden': true  // The main window shows itself after it retrieves settings.
  };
  chrome.app.window.create(MemoryInspectorApp.WINDOW_URL, options,
      this.onWindowCreated_.bind(this));
};

/**
 * Listener called when the main window is created.
 * @private
 * @param {AppWindow} createdWindow The created window.
 */
MemoryInspectorApp.prototype.onWindowCreated_ = function(createdWindow) {
  this.window_ = createdWindow;
  this.window_.onClosed.addListener(this.onWindowClosed_.bind(this));
};

/**
 * Listener called when the main window is closed.
 * @private
 */
MemoryInspectorApp.prototype.onWindowClosed_ = function() {
  this.window_ = undefined;
};

/**
 * Focus the main window.
 * @private
 */
MemoryInspectorApp.prototype.focus_ = function() {
  if (this.window_ !== undefined) {
    this.window_.focus();
  }
};

window.addEventListener('load', function() {
  // Create the singleton MemoryInspectorApp instance and hook it up with the
  // app launcher.
  var app = new MemoryInspectorApp();
  chrome.app.runtime.onLaunched.addListener(app.launch.bind(app));

  // Make the instance global for debugging purposes.
  window.app = app;
});

