// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/** @fileoverview Suite of tests for route-controls. */
cr.define('route_controls', function() {
  function registerTests() {
    suite('RouteControls', function() {
      /**
       * Route Controls created before each test.
       * @type {RouteControls}
       */
      var controls;

      /**
       * First fake route created before each test.
       * @type {media_router.Route}
       */
      var fakeRouteOne;

      /**
       * Second fake route created before each test.
       * @type {media_router.Route}
       */
      var fakeRouteTwo;

      var assertElementText = function(expected, elementId) {
        assertEquals(expected, controls.$$('#' + elementId).innerText);
      };

      var isElementShown = function(elementId) {
        return !controls.$$('#' + elementId).hasAttribute('hidden');
      };

      var assertElementShown = function(elementId) {
        assertTrue(isElementShown(elementId));
      };

      var assertElementHidden = function(elementId) {
        assertFalse(isElementShown(elementId));
      };

      // Creates an instance of RouteStatus with the given parameters. If a
      // parameter is not set, it defaults to an empty string, zero, or false.
      var createRouteStatus = function(params = {}) {
        return new media_router.RouteStatus(
            params.title ? params.title : '', !!params.canPlayPause,
            !!params.canMute, !!params.canSetVolume, !!params.canSeek,
            params.playState ? params.playState :
                               media_router.PlayState.PLAYING,
            !!params.isPaused, !!params.isMuted,
            params.volume ? params.volume : 0,
            params.duration ? params.duration : 0,
            params.currentTime ? params.currentTime : 0);
      };

      // Import route_controls.html before running suite.
      suiteSetup(function() {
        return PolymerTest.importHtml(
            'chrome://media-router/elements/route_controls/' +
            'route_controls.html');
      });

      // Initialize a route-controls before each test.
      setup(function(done) {
        PolymerTest.clearBody();
        controls = document.createElement('route-controls');
        document.body.appendChild(controls);

        // Initialize routes and sinks.
        fakeRouteOne = new media_router.Route(
            'route id 1', 'sink id 1', 'Video 1', 1, true, false);
        fakeRouteTwo = new media_router.Route(
            'route id 2', 'sink id 2', 'Video 2', 2, false, true);

        // Allow for the route controls to be created and attached.
        setTimeout(done);
      });

      // Tests the initial expected text.
      test('initial text setting', function() {
        // Set |route|.
        controls.onRouteUpdated_(fakeRouteOne);
        assertElementText(fakeRouteOne.description, 'route-description');

        // Set |route| to a different route.
        controls.onRouteUpdated_(fakeRouteTwo);
        assertElementText(fakeRouteTwo.description, 'route-description');
      });

      // Tests that the route status title is shown when RouteStatus is
      // updated.
      test('update route text', function() {
        // Set |route|.
        controls.onRouteUpdated_(fakeRouteOne);
        assertElementText(fakeRouteOne.description, 'route-description');

        // Set the route status title.
        var title = 'test title';
        controls.routeStatus = createRouteStatus({title: title});

        assertElementText(fakeRouteOne.description, 'route-description');
        assertElementText(title, 'route-title');
      });

      // Tests that media controls are shown and hidden when RouteStatus is
      // updated.
      test('media controls visibility', function() {
        // Create a RouteStatus with no controls.
        controls.routeStatus = createRouteStatus();
        assertElementHidden('route-play-pause-button');
        assertElementHidden('route-time-controls');
        assertElementHidden('route-volume-button');
        assertElementHidden('volume-holder');

        controls.routeStatus =
            createRouteStatus({canPlayPause: true, canSeek: true});

        assertElementShown('route-play-pause-button');
        assertElementShown('route-time-controls');
        assertElementHidden('route-volume-button');
        assertElementHidden('volume-holder');

        controls.routeStatus =
            createRouteStatus({canMute: true, canSetVolume: true});

        assertElementHidden('route-play-pause-button');
        assertElementHidden('route-time-controls');
        assertElementShown('route-volume-button');
        assertElementShown('volume-holder');
      });

      // Tests that the play button sends a command to the browser API.
      test('send play command', function(done) {
        var waitForPlayEvent = function(data) {
          document.removeEventListener(
              'mock-play-current-media', waitForPlayEvent);
          done();
        };
        document.addEventListener('mock-play-current-media', waitForPlayEvent);

        controls.routeStatus = createRouteStatus(
            {canPlayPause: true, playState: media_router.PlayState.PAUSED});
        MockInteractions.tap(controls.$$('#route-play-pause-button'));
      });

      // Tests that the pause button sends a command to the browser API.
      test('send pause command', function(done) {
        var waitForPauseEvent = function(data) {
          document.removeEventListener(
              'mock-pause-current-media', waitForPauseEvent);
          done();
        };
        document.addEventListener(
            'mock-pause-current-media', waitForPauseEvent);

        controls.routeStatus = createRouteStatus(
            {canPlayPause: true, playState: media_router.PlayState.PLAYING});
        MockInteractions.tap(controls.$$('#route-play-pause-button'));
      });

      // Tests that the mute button sends a command to the browser API.
      test('send mute command', function(done) {
        var waitForMuteEvent = function(data) {
          // Remove the event listener to avoid interfering with other tests.
          document.removeEventListener(
              'mock-set-current-media-mute', waitForMuteEvent);
          if (data.detail.mute) {
            done();
          } else {
            done('Expected the "Mute" command but received "Unmute".');
          }
        };
        document.addEventListener(
            'mock-set-current-media-mute', waitForMuteEvent);

        controls.routeStatus =
            createRouteStatus({canMute: true, isMuted: false});
        MockInteractions.tap(controls.$$('#route-volume-button'));
      });

      // Tests that the unmute button sends a command to the browser API.
      test('send unmute command', function(done) {
        var waitForUnmuteEvent = function(data) {
          // Remove the event listener to avoid interfering with other tests.
          document.removeEventListener(
              'mock-set-current-media-mute', waitForUnmuteEvent);
          if (data.detail.mute) {
            done('Expected the "Unmute" command but received "Mute".');
          } else {
            done();
          }
        };
        document.addEventListener(
            'mock-set-current-media-mute', waitForUnmuteEvent);

        controls.routeStatus =
            createRouteStatus({canMute: true, isMuted: true});
        MockInteractions.tap(controls.$$('#route-volume-button'));
      });

      // // Tests that the seek slider sends a command to the browser API.
      test('send seek command', function(done) {
        var currentTime = 500;
        var duration = 1200;
        var waitForSeekEvent = function(data) {
          document.removeEventListener(
              'mock-seek-current-media', waitForSeekEvent);
          if (data.detail.time == currentTime) {
            done();
          } else {
            done(
                'Expected the time to be ' + currentTime + ' but instead got ' +
                data.detail.time);
          }
        };
        document.addEventListener('mock-seek-current-media', waitForSeekEvent);

        controls.routeStatus =
            createRouteStatus({canSeek: true, duration: duration});

        // In actual usage, the change event gets fired when the user interacts
        // with the slider.
        controls.$$('#route-time-slider').value = currentTime;
        controls.$$('#route-time-slider').fire('change');
      });

      // Tests that the volume slider sends a command to the browser API.
      test('send set volume command', function(done) {
        var volume = 0.45;
        var waitForSetVolumeEvent = function(data) {
          document.removeEventListener(
              'mock-set-current-media-volume', waitForSetVolumeEvent);
          if (data.detail.volume == volume) {
            done();
          } else {
            done(
                'Expected the volume to be ' + volume + ' but instead got ' +
                data.detail.volume);
          }
        };
        document.addEventListener(
            'mock-set-current-media-volume', waitForSetVolumeEvent);

        controls.routeStatus = createRouteStatus({canSetVolume: true});

        // In actual usage, the change event gets fired when the user interacts
        // with the slider.
        controls.$$('#route-volume-slider').value = volume;
        controls.$$('#route-volume-slider').fire('change');
      });

      test('increment current time while playing', function(done) {
        var initialTime = 50;
        controls.routeStatus = createRouteStatus({
          canSeek: true,
          playState: media_router.PlayState.PLAYING,
          duration: 100,
          currentTime: initialTime,
        });

        // Check that the current time has been incremented after a second.
        setTimeout(function() {
          controls.routeStatus.playState = media_router.PlayState.PAUSED;
          var pausedTime = controls.displayedCurrentTime_;
          assertTrue(pausedTime > initialTime);

          // Check that the current time stayed the same after a second, now
          // that the media is paused.
          setTimeout(function() {
            assertEquals(pausedTime, controls.displayedCurrentTime_);
            done();
          }, 1000);
        }, 1000);
      });

      test('set media remoting enabled', function(done) {
        assertElementHidden('mirroring-fullscreen-video-controls');
        let routeStatus = createRouteStatus();
        controls.routeStatus = routeStatus;
        assertElementHidden('mirroring-fullscreen-video-controls');

        routeStatus = createRouteStatus();
        routeStatus.mirroringExtraData = {mediaRemotingEnabled: true};
        controls.routeStatus = routeStatus;
        assertElementShown('mirroring-fullscreen-video-controls');
        assertEquals(
            controls.FullscreenVideoOption_.REMOTE_SCREEN,
            controls.$$('#mirroring-fullscreen-video-dropdown').value);

        document.addEventListener(
            'mock-set-media-remoting-enabled', function(e) {
              assertFalse(e.detail.enabled);
              done();
            });

        // Simulate changing the dropdown menu value.
        controls.$$('#mirroring-fullscreen-video-dropdown').value =
            controls.FullscreenVideoOption_.BOTH_SCREENS;
        controls.$$('#mirroring-fullscreen-video-dropdown')
            .dispatchEvent(new Event('change'));
      });

      test('hangouts local present mode', function(done) {
        assertElementHidden('hangouts-local-present-controls');
        let routeStatus = createRouteStatus();
        controls.routeStatus = routeStatus;
        assertElementHidden('hangouts-local-present-controls');

        routeStatus = createRouteStatus();
        routeStatus.hangoutsExtraData = {localPresent: false};
        controls.routeStatus = routeStatus;
        assertElementShown('hangouts-local-present-controls');

        routeStatus = createRouteStatus();
        routeStatus.hangoutsExtraData = {localPresent: true};
        controls.routeStatus = routeStatus;
        assertElementShown('hangouts-local-present-controls');
        assertTrue(controls.$$('#hangouts-local-present-checkbox').checked);

        document.addEventListener(
            'mock-set-hangouts-local-present', function(e) {
              done();
            });
        MockInteractions.tap(controls.$$('#hangouts-local-present-checkbox'));
        assertFalse(controls.$$('#hangouts-local-present-checkbox').checked);
      });

      test('ignore external updates right after using sliders', function(done) {
        var currentTime = 500;
        var externalCurrentTime = 800;
        var volume = 0.45;
        var externalVolume = 0.72;
        var duration = 1200;
        var doExternalUpdate = function() {
          controls.routeStatus = createRouteStatus({
            canSeek: true,
            canSetVolume: true,
            currentTime: externalCurrentTime,
            duration: duration,
            volume: externalVolume
          });
        };

        controls.routeStatus = createRouteStatus(
            {canSeek: true, canSetVolume: true, duration: duration});

        // In actual usage, the change event gets fired when the user interacts
        // with the slider.
        controls.$$('#route-time-slider').value = currentTime;
        controls.$$('#route-time-slider').fire('change');
        controls.$$('#route-volume-slider').value = volume;
        controls.$$('#route-volume-slider').fire('change');

        // External updates right after slider interaction should be ignored.
        doExternalUpdate();
        assertEquals(controls.$$('#route-time-slider').value, currentTime);
        assertEquals(controls.$$('#route-volume-slider').value, volume);

        setTimeout(function() {
          // External updates after some time should get applied to the sliders.
          doExternalUpdate();
          assertEquals(
              controls.$$('#route-time-slider').value, externalCurrentTime);
          assertEquals(
              controls.$$('#route-volume-slider').value, externalVolume);
          done();
        }, 1001);
      });
    });
  }

  return {
    registerTests: registerTests,
  };
});
