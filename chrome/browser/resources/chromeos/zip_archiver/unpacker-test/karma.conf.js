// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

'use strict';

/**
 * Defines the configuration for the karma test runner.
 * @param {Object} config The karma config object.
 */
module.exports = function(config) {
  config.set({
    /**
     * Base path that will be used to resolve all patterns (eg. files, exclude).
     * @type {string}
     */
    basePath: '../unpacker/debug',

    /**
     * @type {!Object<string, string>}
     */
    proxies: {
      // Hack for a bug in Karma, which doesn't allow to access static files
      // which are not under the base path using relative paths.
      '/test-files/': '/absolute' + process.env['PWD'] + '/test-files/',

      // For the passphrase dialog.
      '/js/': '/base/js/',
      '/html/': '/base/html/',
      '/css/': '/base/css/',
      '/third-party/': '/base/third-party/'
    },

    /**
     * Frameworks to use. Available frameworks:
     * https://npmjs.org/browse/keyword/karma-adapter
     * @type {!Array<string>}
     */
    frameworks: ['mocha', 'chai', 'sinon'],

    /**
     * List of files / patterns to load in the browser.
     * In case any file that is not a .js is changed, the tests must be run
     * again.
     * @type {!Array<string>}
     */
    files: [
      // Application files. Only *.js files are included as <script>, the rest
      // only served.
      {pattern: 'module.nmf', watched: false, included: false, served: true},
      {pattern: 'module.pexe', watched: false, included: false, served: true},
      // unpacker.js served before as it contains the main namespace.
      {pattern: 'js/unpacker.js', watched: true, included: true, served: true},
      {pattern: 'js/app.js', watched: true, included: true, served: true}, {
        pattern: 'js/decompressor.js',
        watched: true,
        included: true,
        served: true
      },
      {
        pattern: 'js/passphrase-manager.js',
        watched: true,
        included: true,
        served: true
      },
      {pattern: 'js/request.js', watched: true, included: true, served: true},
      {pattern: 'js/types.js', watched: true, included: true, served: true},
      {pattern: 'js/volume.js', watched: true, included: true, served: true},
      // Not included as Polymer is undefined at this moment. Polymer will be
      // loaded at a later time.
      {
        pattern: 'js/passphrase-dialog.js',
        watched: true,
        included: false,
        served: true
      },
      {pattern: 'html/*.html', watched: true, included: false, served: true},
      {pattern: 'css/*.css', watched: true, included: false, served: true},
      {pattern: 'third-party/*', watched: true, included: false, served: true},

      // Test files.
      {
        pattern: '../../unpacker-test/test-files/**/*',
        watched: false,
        included: false,
        served: true
      },

      // These 2 files must be included before integration_test.js. They define
      // helper functions for the integration tests so by the time
      // integration_test.js file is parsed those functions must be already
      // available.
      '../../unpacker-test/js/integration_test_helper.js',
      '../../unpacker-test/js/integration_specific_archives_tests.js',

      // All other test files, including integration_test.js.
      '../../unpacker-test/js/*.js'
    ],

    /**
     * Test results reporter to use. Possible values: 'dots', 'progress'.
     * available reporters: https://npmjs.org/browse/keyword/karma-reporter
     * @type {!Array<string>}
     */
    reporters: ['progress'],

    /**
     * Web server port. Update ARCHIVE_BASE_URL in integration_test_helper.js if
     * modified.
     * @type {number}
     */
    port: 9876,

    /**
     * Enable / disable colors in the output (reporters and logs).
     * @type {boolean}
     */
    colors: true,

    /**
     * The level of logging. Possible values:
     *     config.LOG_DISABLE || config.LOG_ERROR || config.LOG_WARN ||
     *     config.LOG_INFO || config.LOG_DEBUG
     * @type {string}
     */
    logLevel: config.LOG_INFO,

    /**
     * Enable / disable watching file and executing tests whenever any file
     * changes.
     * In order for autoWatch to work correctly in vim use ":set backupcopy=yes"
     * workaround (see https://github.com/paulmillr/chokidar/issues/35).
     * @type {boolean}
     */
    autoWatch: true,

    /**
     * Custom launchers to be used in browsers. A custom launcher is required in
     * order to enable Nacl for every application, even those that are not
     * installed from the Chrome market.
     * DO NOT use '--enable-nacl-debug'. The module will block until typing
     * 'continue' in the gdb console, but tests have a timeout limit.
     * @type {!Object<string, !Object>}
     */
    customLaunchers: {
      'Chrome-dev': {
        base: 'Chrome',
        flags: [
          '--disable-setuid-sandbox', '--enable-nacl', '--enable-pnacl',
          '--user-data-dir=user-data-dir-karma',
          // Required for redirecting NaCl module stdout and stderr outputs
          // using NACL_EXE_STDOUT and NACL_EXE_STDERR environment variables.
          // See run_js_tests.js.
          '--no-sandbox', '--enable-logging', '--v=1'
        ]
      }
    },

    /**
     * The browsers to start. Only 'Chrome-dev' defined above is required
     * for the unpacker extension.
     * @type {!Array<string>}
     */
    browsers: ['Chrome-dev'],

    /**
     * Continuous Integration mode. If true, Karma captures browsers,
     * runs the tests and exits.
     * @type {boolean}
     */
    singleRun: false
  });
};
