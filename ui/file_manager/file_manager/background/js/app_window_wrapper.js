// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * Wrapper for an app window.
 *
 * Expects the following from the app scripts:
 * 1. The page load handler should initialize the app using |window.appState|
 *    and call |util.saveAppState|.
 * 2. Every time the app state changes the app should update |window.appState|
 *    and call |util.saveAppState| .
 * 3. The app may have |unload| function to persist the app state that does not
 *    fit into |window.appState|.
 *
 * @param {string} url App window content url.
 * @param {string} id App window id.
 * @param {Object} options Options object to create it.
 * @constructor
 * @struct
 */
function AppWindowWrapper(url, id, options) {
  this.url_ = url;
  this.id_ = id;
  // Do deep copy for the template of options to assign customized params later.
  this.options_ = /** @type {!chrome.app.window.CreateWindowOptions} */(
      JSON.parse(JSON.stringify(options)));
  this.window_ = null;
  this.appState_ = null;
  this.openingOrOpened_ = false;
  this.queue = new AsyncUtil.Queue();
}

AppWindowWrapper.prototype = /** @struct */ {
  /**
   * @return {chrome.app.window.AppWindow} Wrapped application window.
   */
  get rawAppWindow() {
    return this.window_;
  }
};

/**
 * Key for getting and storing the last window state (maximized or not).
 * @const
 * @private
 */
AppWindowWrapper.MAXIMIZED_KEY_ = 'isMaximized';

/**
 * Make a key of window geometry preferences for the given initial URL.
 * @param {string} url Initialize URL that the window has.
 * @return {string} Key of window geometry preferences.
 */
AppWindowWrapper.makeGeometryKey = function(url) {
  return 'windowGeometry' + ':' + url;
};

/**
 * Shift distance to avoid overlapping windows.
 * @type {number}
 * @const
 */
AppWindowWrapper.SHIFT_DISTANCE = 40;

/**
 * Sets the icon of the window.
 * @param {string} iconPath Path of the icon.
 */
AppWindowWrapper.prototype.setIcon = function(iconPath) {
  this.window_.setIcon(iconPath);
};

/**
 * Opens the window.
 *
 * @param {Object} appState App state.
 * @param {boolean} reopen True if the launching is triggered automatically.
 *     False otherwise.
 * @param {function()=} opt_callback Completion callback.
 */
AppWindowWrapper.prototype.launch = function(appState, reopen, opt_callback) {
  // Check if the window is opened or not.
  if (this.openingOrOpened_) {
    console.error('The window is already opened.');
    if (opt_callback)
      opt_callback();
    return;
  }
  this.openingOrOpened_ = true;

  // Save application state.
  this.appState_ = appState;

  // Get similar windows, it means with the same initial url, eg. different
  // main windows of the Files app.
  var similarWindows = window.getSimilarWindows(this.url_);

  // Restore maximized windows, to avoid hiding them to tray, which can be
  // confusing for users.
  this.queue.run(function(callback) {
    for (var index = 0; index < similarWindows.length; index++) {
      if (similarWindows[index].isMaximized()) {
        var createWindowAndRemoveListener = function() {
          similarWindows[index].onRestored.removeListener(
              createWindowAndRemoveListener);
          callback();
        };
        similarWindows[index].onRestored.addListener(
            createWindowAndRemoveListener);
        similarWindows[index].restore();
        return;
      }
    }
    // If no maximized windows, then create the window immediately.
    callback();
  });

  // Obtains the last geometry and window state (maximized or not).
  var lastBounds;
  var isMaximized = false;
  this.queue.run(function(callback) {
    var boundsKey = AppWindowWrapper.makeGeometryKey(this.url_);
    var maximizedKey = AppWindowWrapper.MAXIMIZED_KEY_;
    chrome.storage.local.get([boundsKey, maximizedKey], function(preferences) {
      if (!chrome.runtime.lastError) {
        lastBounds = preferences[boundsKey];
        isMaximized = preferences[maximizedKey];
      }
      callback();
    });
  }.bind(this));

  // Closure creating the window, once all preprocessing tasks are finished.
  this.queue.run(function(callback) {
    // Apply the last bounds.
    if (lastBounds)
      this.options_.bounds = lastBounds;

    // Overwrite maximized state with remembered last window state.
    if (isMaximized !== undefined)
      this.options_.state = isMaximized ? 'maximized' : undefined;

    // Create a window.
    chrome.app.window.create(this.url_, this.options_, function(appWindow) {
      // Exit full screen state if it's created as a full screen window.
      if (appWindow.isFullscreen())
        appWindow.restore();

      // This is a temporary workaround for crbug.com/452737.
      // {state: 'maximized'} in CreateWindowOptions is ignored when a window is
      // launched with hidden option, so we maximize the window manually here.
      if (this.options_.hidden && this.options_.state === 'maximized')
        appWindow.maximize();
      this.window_ = appWindow;
      callback();
    }.bind(this));
  }.bind(this));

  // After creating.
  this.queue.run(function(callback) {
    // If there is another window in the same position, shift the window.
    var makeBoundsKey = function(bounds) {
      return bounds.left + '/' + bounds.top;
    };
    var notAvailablePositions = {};
    for (var i = 0; i < similarWindows.length; i++) {
      var key = makeBoundsKey(similarWindows[i].getBounds());
      notAvailablePositions[key] = true;
    }
    var candidateBounds = this.window_.getBounds();
    while (true) {
      var key = makeBoundsKey(candidateBounds);
      if (!notAvailablePositions[key])
        break;
      // Make the position available to avoid an infinite loop.
      notAvailablePositions[key] = false;
      var nextLeft = candidateBounds.left + AppWindowWrapper.SHIFT_DISTANCE;
      var nextRight = nextLeft + candidateBounds.width;
      candidateBounds.left = nextRight >= screen.availWidth ?
          nextRight % screen.availWidth : nextLeft;
      var nextTop = candidateBounds.top + AppWindowWrapper.SHIFT_DISTANCE;
      var nextBottom = nextTop + candidateBounds.height;
      candidateBounds.top = nextBottom >= screen.availHeight ?
          nextBottom % screen.availHeight : nextTop;
    }
    this.window_.moveTo(candidateBounds.left, candidateBounds.top);

    // Save the properties.
    var appWindow = this.window_;
    window.appWindows[this.id_] = appWindow;
    var contentWindow = appWindow.contentWindow;
    contentWindow.appID = this.id_;
    contentWindow.appState = this.appState_;
    contentWindow.appReopen = reopen;
    contentWindow.appInitialURL = this.url_;
    if (window.IN_TEST)
      contentWindow.IN_TEST = true;

    // Register event listeners.
    appWindow.onBoundsChanged.addListener(this.onBoundsChanged_.bind(this));
    appWindow.onClosed.addListener(this.onClosed_.bind(this));

    // Callback.
    if (opt_callback)
      opt_callback();
    callback();
  }.bind(this));
};

/**
 * Handles the onClosed extension API event.
 * @private
 */
AppWindowWrapper.prototype.onClosed_ = function() {
  // Remember the last window state (maximized or normal).
  var preferences = {};
  preferences[AppWindowWrapper.MAXIMIZED_KEY_] = this.window_.isMaximized();
  chrome.storage.local.set(preferences);

  // Unload the window.
  var appWindow = this.window_;
  var contentWindow = this.window_.contentWindow;
  if (contentWindow.unload)
    contentWindow.unload();
  this.window_ = null;
  this.openingOrOpened_ = false;

  // Updates preferences.
  if (contentWindow.saveOnExit) {
    contentWindow.saveOnExit.forEach(function(entry) {
      util.AppCache.update(entry.key, entry.value);
    });
  }
  chrome.storage.local.remove(this.id_);  // Forget the persisted state.

  // Remove the window from the set.
  delete window.appWindows[this.id_];
};

/**
 * Handles onBoundsChanged extension API event.
 * @private
 */
AppWindowWrapper.prototype.onBoundsChanged_ = function() {
  if (!this.window_.isMaximized()) {
    var preferences = {};
    preferences[AppWindowWrapper.makeGeometryKey(this.url_)] =
        this.window_.getBounds();
    chrome.storage.local.set(preferences);
  }
};

/**
 * Wrapper for a singleton app window.
 *
 * In addition to the AppWindowWrapper requirements the app scripts should
 * have |reload| method that re-initializes the app based on a changed
 * |window.appState|.
 *
 * @param {string} url App window content url.
 * @param {Object|function()} options Options object or a function to return it.
 * @constructor
 * @extends {AppWindowWrapper}
 */
function SingletonAppWindowWrapper(url, options) {
  AppWindowWrapper.call(this, url, url, options);
}

/**
 * Inherits from AppWindowWrapper.
 */
SingletonAppWindowWrapper.prototype = {__proto__: AppWindowWrapper.prototype};

/**
 * Open the window.
 *
 * Activates an existing window or creates a new one.
 *
 * @param {Object} appState App state.
 * @param {boolean} reopen True if the launching is triggered automatically.
 *     False otherwise.
 * @param {function()=} opt_callback Completion callback.
 */
SingletonAppWindowWrapper.prototype.launch =
    function(appState, reopen, opt_callback) {
  // If the window is not opened yet, just call the parent method.
  if (!this.openingOrOpened_) {
    AppWindowWrapper.prototype.launch.call(
        this, appState, reopen, opt_callback);
    return;
  }

  // If the window is already opened, reload the window.
  // The queue is used to wait until the window is opened.
  this.queue.run(function(nextStep) {
    this.window_.contentWindow.appState = appState;
    this.window_.contentWindow.appReopen = reopen;
    if (!this.window_.contentWindow.reload) {
      // Currently the queue is not made to wait for window loading after
      // creating window. Therefore contentWindow might not have the reload()
      // function ready yet. This happens when launching the same app twice
      // quickly. See crbug.com/789226.
      console.error('Window reload requested before loaded. Skiping.');
    } else {
      this.window_.contentWindow.reload();
    }
    if (opt_callback)
      opt_callback();
    nextStep();
  }.bind(this));
};

/**
 * Reopen a window if its state is saved in the local storage.
 * @param {function()=} opt_callback Completion callback.
 */
SingletonAppWindowWrapper.prototype.reopen = function(opt_callback) {
  chrome.storage.local.get(this.id_, function(items) {
    var value = items[this.id_];
    if (!value) {
      opt_callback && opt_callback();
      return;  // No app state persisted.
    }

    try {
      var appState = assertInstanceof(JSON.parse(value), Object);
    } catch (e) {
      console.error('Corrupt launch data for ' + this.id_, value);
      opt_callback && opt_callback();
      return;
    }
    this.launch(appState, true, opt_callback);
  }.bind(this));
};
