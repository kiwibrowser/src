// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

'use strict';

/**
 * Class to manipulate the window in the remote extension.
 *
 * @param {string} extensionId ID of extension to be manipulated.
 * @constructor
 */
function RemoteCall(extensionId) {
  this.extensionId_ = extensionId;
}

/**
 * Checks whether step by step tests are enabled or not.
 * @return {Promise<bool>}
 */
RemoteCall.isStepByStepEnabled = function() {
  return new Promise(function(fulfill) {
    chrome.commandLinePrivate.hasSwitch(
        'enable-file-manager-step-by-step-tests', fulfill);
  });
};

/**
 * Calls a remote test util in the Files app's extension. See:
 * registerRemoteTestUtils in test_util_base.js.
 *
 * @param {string} func Function name.
 * @param {?string} appId Target window's App ID or null for functions
 *     not requiring a window.
 * @param {Array<*>} args Array of arguments.
 * @param {function(*)=} opt_callback Callback handling the function's result.
 * @return {Promise} Promise to be fulfilled with the result of the remote
 *     utility.
 */
RemoteCall.prototype.callRemoteTestUtil =
    function(func, appId, args, opt_callback) {
  return RemoteCall.isStepByStepEnabled().then(function(stepByStep) {
    if (!stepByStep)
      return false;
    return new Promise(function(onFulfilled) {
      console.info('Executing: ' + func + ' on ' + appId + ' with args: ');
      console.info(args);
      console.info('Type step() to continue...');
      window.step = function() {
        window.step = null;
        onFulfilled(stepByStep);
      };
    });
  }).then(function(stepByStep) {
    return new Promise(function(onFulfilled) {
      chrome.runtime.sendMessage(
          this.extensionId_,
          {
            func: func,
            appId: appId,
            args: args
          },
          {},
          function(var_args) {
            if (stepByStep) {
              console.info('Returned value:');
              console.info(JSON.stringify(arguments));
            }
            if (opt_callback)
              opt_callback.apply(null, arguments);
            onFulfilled(arguments[0]);
          });
    }.bind(this));
  }.bind(this));
};

/**
 * Waits until a window having the given ID prefix appears.
 * @param {string} windowIdPrefix ID prefix of the requested window.
 * @return {Promise} promise Promise to be fulfilled with a found window's ID.
 */
RemoteCall.prototype.waitForWindow = function(windowIdPrefix) {
  var caller = getCaller();
  return repeatUntil(function() {
    return this.callRemoteTestUtil('getWindows', null, []).
        then(function(windows) {
      for (var id in windows) {
        if (id.indexOf(windowIdPrefix) === 0)
          return id;
      }
      return pending(
          caller, 'Window with the prefix %s is not found.', windowIdPrefix);
    });
  }.bind(this));
};

/**
 * Closes a window and waits until the window is closed.
 *
 * @param {string} windowId ID of the window to close.
 * @return {Promise} promise Promise to be fulfilled with the result (true:
 *     success, false: failed).
 */
RemoteCall.prototype.closeWindowAndWait = function(windowId) {
  var caller = getCaller();

  // Closes the window.
  return this.callRemoteTestUtil('closeWindow', null, [windowId]).then(
      function(result) {
        // Returns false when the closing is failed.
        if (!result)
          return false;

        return repeatUntil(function() {
          return this.callRemoteTestUtil('getWindows', null, []).then(
              function(windows) {
                for (var id in windows) {
                  if (id === windowId) {
                    // Window is still available. Continues waiting.
                    return pending(
                        caller, 'Window with the prefix %s is not found.',
                        windowId);
                  }
                }
                // Window is not available. Closing is done successfully.
                return true;
              }
          );
        }.bind(this));
      }.bind(this)
  );
};

/**
 * Waits until the window turns to the given size.
 * @param {string} windowId Target window ID.
 * @param {number} width Requested width in pixels.
 * @param {number} height Requested height in pixels.
 */
RemoteCall.prototype.waitForWindowGeometry = function(windowId, width, height) {
  var caller = getCaller();
  return repeatUntil(function() {
    return this.callRemoteTestUtil('getWindows', null, []).
        then(function(windows) {
      if (!windows[windowId])
        return pending(caller, 'Window %s is not found.', windowId);
      if (windows[windowId].outerWidth !== width ||
          windows[windowId].outerHeight !== height) {
        return pending(
            caller, 'Expected window size is %j, but it is %j',
            {width: width, height: height}, windows[windowId]);
      }
    });
  }.bind(this));
};

/**
 * Waits for the specified element appearing in the DOM.
 * @param {string} windowId Target window ID.
 * @param {string} query Query string for the element.
 * @param {string=} opt_iframeQuery Query string for the iframe containing the
 *     element.
 * @return {Promise} Promise to be fulfilled when the element appears.
 */
RemoteCall.prototype.waitForElement = function(
    windowId, query, opt_iframeQuery) {
  var caller = getCaller();
  return repeatUntil(function() {
    return this.callRemoteTestUtil(
        'queryAllElements',
        windowId,
        [query, opt_iframeQuery]
    ).then(function(elements) {
      if (elements.length > 0)
        return elements[0];
      else
        return pending(
            caller, 'Element %s (maybe in iframe %s) is not found.', query,
            opt_iframeQuery);
    });
  }.bind(this));
};

/**
 * Waits for the specified element leaving from the DOM.
 * @param {string} windowId Target window ID.
 * @param {string} query Query string for the element.
 * @param {string=} opt_iframeQuery Query string for the iframe containing the
 *     element.
 * @return {Promise} Promise to be fulfilled when the element is lost.
 */
RemoteCall.prototype.waitForElementLost = function(
    windowId, query, opt_iframeQuery) {
  var caller = getCaller();
  return repeatUntil(function() {
    return this.callRemoteTestUtil(
        'queryAllElements',
        windowId,
        [query, opt_iframeQuery]
    ).then(function(elements) {
      if (elements.length > 0)
        return pending(caller, 'Elements %j is still exists.', elements);
      return true;
    });
  }.bind(this));
};

/**
 * Sends a fake key down event.
 * @param {string} windowId Window ID.
 * @param {string} query Query for the target element.
 * @param {string} key DOM UI Events Key value.
 * @param {string} keyIdentifer Key identifier.
 * @param {boolean} ctrlKey Control key flag.
 * @param {boolean} shiftKey Shift key flag.
 * @param {boolean} altKey Alt key flag.
 * @return {Promise} Promise to be fulfilled or rejected depending on the
 *     result.
 */
RemoteCall.prototype.fakeKeyDown =
    function(windowId, query, key, keyIdentifer, ctrlKey, shiftKey, altKey) {
  var resultPromise = this.callRemoteTestUtil(
      'fakeKeyDown', windowId,
      [query, key, keyIdentifer, ctrlKey, shiftKey, altKey]);
  return resultPromise.then(function(result) {
    if (result)
      return true;
    else
      return Promise.reject('Fail to fake key down.');
  });
};

/**
 * Gets file entries just under the volume.
 *
 * @param {VolumeManagerCommon.VolumeType} volumeType Volume type.
 * @param {Array<string>} names File name list.
 * @return {Promise} Promise to be fulfilled with file entries or rejected
 *     depending on the result.
 */
RemoteCall.prototype.getFilesUnderVolume = function(volumeType, names) {
  return this.callRemoteTestUtil(
      'getFilesUnderVolume', null, [volumeType, names]);
};

/**
 * Waits for a single file.
 * @param {VolumeManagerCommon.VolumeType} volumeType Volume type.
 * @param {string} name File name.
 * @return {!Promise} Promise to be fulfilled when the file had found.
 */
RemoteCall.prototype.waitForAFile = function(volumeType, name) {
  var caller = getCaller();
  return repeatUntil(function() {
    return this.getFilesUnderVolume(volumeType, [name])
        .then(function(urls) {
          if (urls.length === 1)
            return true;
          return pending(caller, '"' + name + '" is not found.');
        });
  }.bind(this));
};

/**
 * Class to manipulate the window in the remote extension.
 *
 * @param {string} extensionId ID of extension to be manipulated.
 * @extends {RemoteCall}
 * @constructor
 */
function RemoteCallFilesApp() {
  RemoteCall.apply(this, arguments);
}

RemoteCallFilesApp.prototype.__proto__ = RemoteCall.prototype;

/**
 * Waits for the file list turns to the given contents.
 * @param {string} windowId Target window ID.
 * @param {Array<Array<string>>} expected Expected contents of file list.
 * @param {{orderCheck:boolean=, ignoreLastModifiedTime:boolean=}=} opt_options
 *     Options of the comparison. If orderCheck is true, it also compares the
 *     order of files. If ignoreLastModifiedTime is true, it compares the file
 *     without its last modified time.
 * @return {Promise} Promise to be fulfilled when the file list turns to the
 *     given contents.
 */
RemoteCallFilesApp.prototype.waitForFiles =
    function(windowId, expected, opt_options) {
  var options = opt_options || {};
  var caller = getCaller();
  return repeatUntil(function() {
    return this.callRemoteTestUtil(
        'getFileList', windowId, []).then(function(files) {
      if (!options.orderCheck) {
        files.sort();
        expected.sort();
      }
      for (var i = 0; i < Math.min(files.length, expected.length); i++) {
        if (options.ignoreFileSize) {
          files[i][1] = '';
          expected[i][1] = '';
        }
        if (options.ignoreLastModifiedTime) {
          files[i][3] = '';
          expected[i][3] = '';
        }
      }
      if (!chrome.test.checkDeepEq(expected, files)) {
        return pending(
            caller, 'waitForFiles: expected: %j actual %j.', expected, files);
      }
    });
  }.bind(this));
};

/**
 * Waits until the number of files in the file list is changed from the given
 * number.
 * TODO(hirono): Remove the function.
 *
 * @param {string} windowId Target window ID.
 * @param {number} lengthBefore Number of items visible before.
 * @return {Promise} Promise to be fulfilled with the contents of files.
 */
RemoteCallFilesApp.prototype.waitForFileListChange = function(
    windowId, lengthBefore) {
  var caller = getCaller();
  return repeatUntil(function() {
    return this.callRemoteTestUtil(
        'getFileList', windowId, []).then(function(files) {
      files.sort();
      var notReadyRows = files.filter(function(row) {
        return row.filter(function(cell) { return cell == '...'; }).length;
      });
      if (notReadyRows.length === 0 &&
          files.length !== lengthBefore &&
          files.length !== 0) {
        return files;
      } else {
        return pending(
            caller, 'The number of file is %d. Not changed.', lengthBefore);
      }
    });
  }.bind(this));
};

/**
 * Waits until the given taskId appears in the executed task list.
 * @param {string} windowId Target window ID.
 * @param {string} taskId Task ID to watch.
 * @return {Promise} Promise to be fulfilled when the task appears in the
 *     executed task list.
 */
RemoteCallFilesApp.prototype.waitUntilTaskExecutes = function(
    windowId, taskId) {
  var caller = getCaller();
  return repeatUntil(function() {
    return this.callRemoteTestUtil('getExecutedTasks', windowId, []).
        then(function(executedTasks) {
          if (executedTasks.indexOf(taskId) === -1)
            return pending(caller, 'Executed task is %j', executedTasks);
        });
  }.bind(this));
};

/**
 * Check if the next tabforcus'd element has the given ID or not.
 * @param {string} windowId Target window ID.
 * @param {string} elementId String of 'id' attribute which the next tabfocus'd
 *     element should have.
 * @return {Promise} Promise to be fulfilled with the result.
 */
RemoteCallFilesApp.prototype.checkNextTabFocus =
    function(windowId, elementId) {
  return remoteCall.callRemoteTestUtil('fakeKeyDown',
                                       windowId,
                                       ['body', 'Tab', 'U+0009', false]).then(
  function(result) {
    chrome.test.assertTrue(result);
    return remoteCall.callRemoteTestUtil('getActiveElement',
                                         windowId,
                                         []);
  }).then(function(element) {
    if (!element || !element.attributes['id'])
      return false;

    if (element.attributes['id'] === elementId) {
      return true;
    } else {
      console.error('The ID of the element should be "' + elementId +
                    '", but "' + element.attributes['id'] + '"');
      return false;
    }
  });
};

/**
 * Waits until the current directory is changed.
 * @param {string} windowId Target window ID.
 * @param {string} expectedPath Path to be changed to.
 * @return {Promise} Promise to be fulfilled when the current directory is
 *     changed to expectedPath.
 */
RemoteCallFilesApp.prototype.waitUntilCurrentDirectoryIsChanged = function(
    windowId, expectedPath) {
  var caller = getCaller();
  return repeatUntil(function () {
    return this.callRemoteTestUtil('getBreadcrumbPath', windowId, []).then(
      function(path) {
        if(path !== expectedPath)
          return pending(caller, 'Expected path is %s', expectedPath);
      });
  }.bind(this));
};

/**
 * Class to manipulate the window in the remote extension.
 *
 * @param {string} extensionId ID of extension to be manipulated.
 * @extends {RemoteCall}
 * @constructor
 */
function RemoteCallGallery() {
  RemoteCall.apply(this, arguments);
}

RemoteCallGallery.prototype.__proto__ = RemoteCall.prototype;

/**
 * Waits until the expected image is shown.
 *
 * @param {document} document Document.
 * @param {number} width Expected width of the image.
 * @param {number} height Expected height of the image.
 * @param {string|null} name Expected name of the image.
 * @return {Promise} Promsie to be fulfilled when the check is passed.
 */
RemoteCallGallery.prototype.waitForSlideImage =
    function(windowId, width, height, name) {
  var expected = {};
  if (width)
    expected.width = width;
  if (height)
    expected.height = height;
  if (name)
    expected.name = name;
  var caller = getCaller();

  return repeatUntil(function() {
    var query = '.gallery[mode="slide"] .image-container > .image';
    return Promise.all([
        this.waitForElement(windowId, '.filename-spacer input'),
        this.waitForElement(windowId, query)
    ]).then(function(args) {
      var nameBox = args[0];
      var image = args[1];
      var actual = {};
      if (width && image)
        actual.width = image.imageWidth;
      if (height && image)
        actual.height = image.imageHeight;
      if (name && nameBox)
        actual.name = nameBox.value;

      if (!chrome.test.checkDeepEq(expected, actual)) {
        return pending(
            caller, 'Slide mode state, expected is %j, actual is %j.', expected,
            actual);
      }
      return actual;
    });
  }.bind(this));
};

RemoteCallGallery.prototype.changeNameAndWait = function(windowId, newName) {
  return this.callRemoteTestUtil('changeName', windowId, [newName]
  ).then(function() {
    return this.waitForSlideImage(windowId, 0, 0, newName);
  }.bind(this));
};

/**
 * Shorthand for clicking an element.
 * @param {AppWindow} appWindow Application window.
 * @param {string} query Query for the element.
 * @param {Promise} Promise to be fulfilled with the clicked element.
 */
RemoteCallGallery.prototype.waitAndClickElement = function(windowId, query) {
  return this.waitForElement(windowId, query).then(function(element) {
    return this.callRemoteTestUtil('fakeMouseClick', windowId, [query])
    .then(function() { return element; });
  }.bind(this));
};

/**
 * Waits for the "Press Enter" message.
 *
 * @param {AppWindow} appWindow App window.
 * @return {Promise} Promise to be fulfilled when the element appears.
 */
RemoteCallGallery.prototype.waitForPressEnterMessage = function(appId) {
  return this.waitForElement(appId, '.prompt-wrapper .prompt').
      then(function(element) {
        chrome.test.assertEq(
            'Press Enter when done', element.text.trim());
      });
};

/**
 * Shorthand for selecting an image in thumbnail mode.
 * @param {string} appId App id.
 * @param {string} name File name to be selected.
 * @return {!Promise<boolean>} A promise which will be resolved with true if the
 *     thumbnail has clicked. This method does not guarantee whether the
 *     thumbnail has actually selected or not.
 */
RemoteCallGallery.prototype.selectImageInThumbnailMode = function(appId, name) {
  return this.callRemoteTestUtil('fakeMouseClick', appId,
      ['.thumbnail-view > ul > li[title="' + name + '"] > .selection.frame']);
};
