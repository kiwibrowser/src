// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/** @fileoverview Suite of tests for route-details. */
cr.define('route_details', function() {
  function registerTests() {
    suite('RouteDetails', function() {
      /**
       * Route Details created before each test.
       * @type {RouteDetails}
       */
      var details;

      /**
       * First fake route created before each test.
       * @type {media_router.Route}
       */
      var fakeRouteOne;

      /**
       * The custom controller path for |fakeRouteOne|.
       * @const @type {string}
       */
      var fakeRouteOneControllerPath =
          'chrome-extension://123/custom_view.html';

      /**
       * Second fake route created before each test.
       * @type {media_router.Route}
       */
      var fakeRouteTwo;

      /**
       * Fake sink that corresponds to |fakeRouteOne|.
       * @type {media_router.Sink}
       */
      var fakeSinkOne;

      // Checks whether |expected| and the text in the span element in
      // the |elementId| element are equal.
      var checkSpanText = function(expected, elementId) {
        assertEquals(
            expected,
            details.$$('#' + elementId).querySelector('span').innerText);
      };

      // Checks whether |expected| and the text in the element in the
      // |elementId| element are equal.
      var checkElementText = function(expected, elementId) {
        assertEquals(expected, details.$$('#' + elementId).innerText);
      };

      // Checks the default route view is shown.
      var checkDefaultViewIsShown = function() {
        assertFalse(details.$$('#route-description').hasAttribute('hidden'));
        assertTrue(
            !details.$$('extension-view-wrapper') ||
            details.$$('extension-view-wrapper').hasAttribute('hidden'));
      };

      // Checks the start button is shown.
      var checkStartCastButtonIsShown = function() {
        assertFalse(details.$$('#start-casting-to-route-button')
                        .hasAttribute('hidden'));
      };

      // Checks the start button is not shown.
      var checkStartCastButtonIsNotShown = function() {
        assertTrue(details.$$('#start-casting-to-route-button')
                       .hasAttribute('hidden'));
      };

      // Import route_details.html before running suite.
      suiteSetup(function() {
        return PolymerTest.importHtml(
            'chrome://media-router/elements/route_details/' +
            'route_details.html');
      });

      // Initialize a route-details before each test.
      setup(function(done) {
        PolymerTest.clearBody();
        details = document.createElement('route-details');
        document.body.appendChild(details);

        // Initialize routes and sinks.
        fakeRouteOne = new media_router.Route(
            'route id 1', 'sink id 1', 'Video 1', 1, true, false,
            fakeRouteOneControllerPath);
        fakeRouteTwo = new media_router.Route(
            'route id 2', 'sink id 2', 'Video 2', 2, false, true);
        fakeSinkOne = new media_router.Sink(
            'sink id 1', 'sink 1', 'description', null,
            media_router.SinkIconType.CAST, media_router.SinkStatus.ACTIVE,
            2 | 4);

        // Allow for the route details to be created and attached.
        setTimeout(done);
      });

      // Tests that the cast button is shown under the correct circumstances and
      // that updating |replaceRouteAvailable| updates the cast button
      // visibility.
      test('cast button visibility', function() {
        details.route = fakeRouteTwo;
        checkStartCastButtonIsShown();

        details.route = fakeRouteOne;
        checkStartCastButtonIsNotShown();

        details.sink = fakeSinkOne;
        checkStartCastButtonIsShown();

        // Retrigger observer because it's not necessary for it to watch
        // |route.currentCastMode| in general.
        fakeRouteOne.currentCastMode = 2;
        details.route = null;
        details.route = fakeRouteOne;
        checkStartCastButtonIsNotShown();

        // Simulate user changing cast modes to be compatible or incompatible
        // with the route's sink.
        details.shownCastModeValue = 4;
        checkStartCastButtonIsShown();

        details.shownCastModeValue = 1;
        checkStartCastButtonIsNotShown();

        details.shownCastModeValue = 4;
        checkStartCastButtonIsShown();

        // Cast button should be hidden while another sink is launching.
        details.isAnySinkCurrentlyLaunching = true;
        checkStartCastButtonIsNotShown();

        details.isAnySinkCurrentlyLaunching = false;
        checkStartCastButtonIsShown();
      });

      // Tests for 'close-route-click' event firing when the
      // 'close-route-button' button is clicked.
      test('close route button click', function(done) {
        details.addEventListener('close-route', function() {
          done();
        });
        MockInteractions.tap(details.$$('#close-route-button'));
      });

      // Tests for 'join-route-click' event firing when the
      // 'start-casting-to-route-button' button is clicked when the current
      // route is joinable.
      test('start casting to route button click', function(done) {
        details.addEventListener('join-route-click', function() {
          done();
        });
        details.route = fakeRouteTwo;
        MockInteractions.tap(details.$$('#start-casting-to-route-button'));
      });

      // Tests for 'replace-route-click' event firing when the
      // 'start-casting-to-route-button' button is clicked when the current
      // route is not joinable.
      test('start casting button click replaces route', function(done) {
        details.addEventListener('change-route-source-click', function() {
          done();
        });
        details.route = fakeRouteOne;
        details.availableCastModes = 1;
        MockInteractions.tap(details.$$('#start-casting-to-route-button'));
      });

      // Tests the initial expected text.
      test('initial text setting', function() {
        // <paper-button> text is styled as upper case.
        checkSpanText(
            loadTimeData.getString('stopCastingButtonText').toUpperCase(),
            'close-route-button');
        checkSpanText(
            loadTimeData.getString('startCastingButtonText').toUpperCase(),
            'start-casting-to-route-button');
        checkElementText('', 'route-description');
      });

      // Tests when |route| is undefined or set.
      test('route is undefined or set', function() {
        // |route| is initially undefined.
        assertEquals(undefined, details.route);
        checkDefaultViewIsShown();

        // Set |route|.
        details.route = fakeRouteOne;
        assertEquals(fakeRouteOne, details.route);
        checkElementText(fakeRouteOne.description, 'route-description');
        checkDefaultViewIsShown();
        checkStartCastButtonIsNotShown();

        // Set |route| to a different route.
        details.route = fakeRouteTwo;
        assertEquals(fakeRouteTwo, details.route);
        checkElementText(fakeRouteTwo.description, 'route-description');
        checkDefaultViewIsShown();
        checkStartCastButtonIsShown();
      });
    });
  }

  return {
    registerTests: registerTests,
  };
});
