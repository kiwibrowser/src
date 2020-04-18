// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/** @fileoverview Tests for settings.animation. */

/** @const {string} Path to root from chrome/test/data/webui/settings/. */
const ROOT_PATH = '../../../../../';

/**
 * @constructor
 * @extends testing.Test
 */
function SettingsAnimationBrowserTest() {}

SettingsAnimationBrowserTest.prototype = {
  __proto__: testing.Test.prototype,

  /** @override */
  browsePreload: 'chrome://settings/animation/animation.html',

  /** @override */
  extraLibraries: [
    ROOT_PATH + 'third_party/mocha/mocha.js',
    '../mocha_adapter.js',
  ],

  /** @override */
  isAsync: true,

  /** @override */
  runAccessibilityChecks: false,
};

TEST_F('SettingsAnimationBrowserTest', 'Animation', function() {
  const self = this;

  const Animation = settings.animation.Animation;

  const onFinishBeforePromise = function() {
    assertNotReached('Animation fired finish event before resolving promise');
  };

  const onCancelUnexpectedly = function() {
    assertNotReached('Animation should have finished, but fired cancel event');
  };

  // Register mocha tests.
  suite('settings.animation.Animation', function() {
    let div;
    let keyframes;
    let options;

    setup(function() {
      keyframes = [
        {
          height: '100px',
          easing: 'ease-in',
        },
        {
          height: '200px',
        }
      ];

      options = {
        duration: 1000,
        // Use fill: both so we can test the animation start and end states.
        fill: 'both',
      };

      div = document.createElement('div');
      document.body.appendChild(div);
    });

    teardown(function() {
      div.remove();
    });

    test('Animation plays', function(done) {
      const animation = new Animation(div, keyframes, options);
      animation.addEventListener('cancel', onCancelUnexpectedly);
      animation.addEventListener('finish', onFinishBeforePromise);

      requestAnimationFrame(function() {
        expectEquals(100, div.clientHeight);

        animation.finished.then(function() {
          expectEquals(200, div.clientHeight);
          animation.removeEventListener('finish', onFinishBeforePromise);
          animation.addEventListener('finish', function() {
            done();
          });
        });
      });
    });

    test('Animation finishes', function(done) {
      // Absurdly large finite value to ensure we call finish() before the
      // animation finishes automatically.
      options.duration = Number.MAX_VALUE;
      const animation = new Animation(div, keyframes, options);
      animation.addEventListener('cancel', onCancelUnexpectedly);
      animation.addEventListener('finish', onFinishBeforePromise);

      // TODO(michaelpg): rAF seems more appropriate, but crbug.com/620160.
      setTimeout(function() {
        expectEquals(100, div.clientHeight);

        animation.finish();

        // The promise should resolve before the finish event is scheduled.
        animation.finished.then(function() {
          expectEquals(200, div.clientHeight);
          animation.removeEventListener('finish', onFinishBeforePromise);
          animation.addEventListener('finish', function() {
            done();
          });
        });
      });
    });

    test('Animation cancels', function(done) {
      // Absurdly large finite value to ensure we call cancel() before the
      // animation finishes automatically.
      options.duration = Number.MAX_VALUE;
      const animation = new Animation(div, keyframes, options);
      animation.addEventListener('cancel', onCancelUnexpectedly);
      animation.addEventListener('finish', onFinishBeforePromise);

      // TODO(michaelpg): rAF seems more appropriate, but crbug.com/620160.
      setTimeout(function() {
        expectEquals(100, div.clientHeight);

        animation.cancel();

        // The promise should be rejected before the cancel event is scheduled.
        animation.finished.catch(function() {
          expectEquals(0, div.clientHeight);
          animation.removeEventListener('cancel', onCancelUnexpectedly);
          animation.addEventListener('cancel', function() {
            done();
          });
        });
      });
    });
  });

  // Run all registered tests.
  mocha.run();
});
