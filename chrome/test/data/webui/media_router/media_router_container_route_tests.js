// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/** @fileoverview Suite of tests for media-router-container that focus on
 * routes.
 */
cr.define('media_router_container_route', function() {
  function registerTests() {
    suite('MediaRouterContainerRoute', function() {
      /**
       * Checks whether |view| matches the current view of |container|.
       *
       * @param {!media_router.MediaRouterView} view Expected view type.
       */
      var checkCurrentView;

      /**
       * Checks whether the elements specified in |elementIdList| are visible.
       * Checks whether all other elements are not visible. Throws an assertion
       * error if this is not true.
       *
       * @param {!Array<!string>} elementIdList List of id's of elements that
       *     should be visible.
       */
      var checkElementsVisibleWithId;

      /**
       * Checks the visibility of an element. An element is considered visible
       * if it exists and its |hidden| property is |false|.
       *
       * @param {boolean} visible Whether the element should be visible.
       * @param {?Element} element The element to test.
       * @param {elementId=} elementId Optional element id to display.
       */
      var checkElementVisible;

      /**
       * Checks whether |expected| and the text in the |element| are equal.
       *
       * @param {!string} expected Expected text.
       * @param {!Element} element Element whose text will be checked.
       */
      var checkElementText;

      /**
       * Media Router Container created before each test.
       * @type {?MediaRouterContainer}
       */
      var container;

      /**
       * The blocking issue to show.
       * @type {?media_router.Issue}
       */
      var fakeBlockingIssue;

      /**
       * The list of CastModes to show.
       * @type {!Array<!media_router.CastMode>}
       */
      var fakeCastModeList = [];

      /**
       * The blocking issue to show.
       * @type {?media_router.Issue}
       */
      var fakeNonBlockingIssue;

      /**
       * The list of current routes.
       * @type {!Array<!media_router.Route>}
       */
      var fakeRouteList = [];

      /**
       * The list of available sinks.
       * @type {!Array<!media_router.Sink>}
       */
      var fakeSinkList = [];

      // Import media_router_container.html before running suite.
      suiteSetup(function() {
        return PolymerTest.importHtml(
            'chrome://media-router/elements/media_router_container/' +
            'media_router_container.html');
      });

      setup(function(done) {
        PolymerTest.clearBody();
        // Initialize a media-router-container before each test.
        container = document.createElement('media-router-container');
        document.body.appendChild(container);

        // Get common functions and variables.
        var testBase = media_router_container_test_base.init(container);

        checkCurrentView = testBase.checkCurrentView;
        checkElementsVisibleWithId = testBase.checkElementsVisibleWithId;
        checkElementVisible = testBase.checkElementVisible;
        checkElementText = testBase.checkElementText;
        fakeBlockingIssue = testBase.fakeBlockingIssue;
        fakeCastModeList = testBase.fakeCastModeList;
        fakeNonBlockingIssue = testBase.fakeNonBlockingIssue;
        fakeRouteList = testBase.fakeRouteList;
        fakeRouteListWithLocalRoutesOnly =
            testBase.fakeRouteListWithLocalRoutesOnly;
        fakeSinkList = testBase.fakeSinkList;

        container.castModeList = testBase.fakeCastModeList;

        // Allow for the media router container to be created, attached, and
        // listeners registered in an afterNextRender() call.
        Polymer.RenderStatus.afterNextRender(this, done);
      });

      // Tests for 'create-route' event firing when a sink with no associated
      // route is clicked.
      test('select sink without a route', function(done) {
        container.allSinks = fakeSinkList;

        setTimeout(function() {
          var sinkList = container.shadowRoot.getElementById('sink-list')
                             .querySelectorAll('paper-item');
          container.addEventListener('create-route', function(data) {
            // Container is initially in auto mode since a cast mode has not
            // been selected.
            assertEquals(
                media_router.CastModeType.AUTO, container.shownCastModeValue_);
            assertEquals(fakeSinkList[2].id, data.detail.sinkId);

            // The preferred compatible cast mode on the sink is used, since
            // the we did not choose a cast mode on the container.
            assertEquals(0x2, data.detail.selectedCastModeValue);
            done();
          });
          // Tap on a sink without a route, which should fire a 'create-route'
          // event.
          assertEquals(fakeSinkList.length, sinkList.length);
          MockInteractions.tap(sinkList[2]);
        });
      });

      // Tests that selecting a sink with an associated route will make the
      // |container| switch to ROUTE_DETAILS view.
      test('select sink with a route', function(done) {
        container.allSinks = fakeSinkList;
        container.routeList = fakeRouteList;

        setTimeout(function() {
          var sinkList = container.shadowRoot.getElementById('sink-list')
                             .querySelectorAll('paper-item');

          // Start from the SINK_LIST view.
          container.showSinkList_();
          checkCurrentView(media_router.MediaRouterView.SINK_LIST);
          MockInteractions.tap(sinkList[0]);
          checkCurrentView(media_router.MediaRouterView.ROUTE_DETAILS);
          done();
        });
      });

      // Tests the text shown for the sink list.
      test('initial sink list route text', function(done) {
        // Sink 1 - no sink description, no route -> no subtext
        // Sink 2 - sink description, no route -> subtext = sink description
        // Sink 3 - no sink description, route -> subtext = route description
        // Sink 4 - sink description, route -> subtext = route description
        container.allSinks = [
          new media_router.Sink(
              'sink id 1', 'Sink 1', null, null, media_router.SinkIconType.CAST,
              media_router.SinkStatus.ACTIVE, [1, 2, 3]),
          new media_router.Sink(
              'sink id 2', 'Sink 2', 'Sink 2 description', null,
              media_router.SinkIconType.CAST, media_router.SinkStatus.ACTIVE,
              [1, 2, 3]),
          new media_router.Sink(
              'sink id 3', 'Sink 3', null, null, media_router.SinkIconType.CAST,
              media_router.SinkStatus.PENDING, [1, 2, 3]),
          new media_router.Sink(
              'sink id 4', 'Sink 4', 'Sink 4 description', null,
              media_router.SinkIconType.CAST, media_router.SinkStatus.PENDING,
              [1, 2, 3])
        ];

        container.routeList = [
          new media_router.Route('id 3', 'sink id 3', 'Title 3', 0, true),
          new media_router.Route('id 4', 'sink id 4', 'Title 4', 1, false),
        ];

        setTimeout(function() {
          var sinkSubtextList = container.shadowRoot.getElementById('sink-list')
                                    .querySelectorAll('.sink-subtext');

          // There will only be 3 sink subtext entries, because Sink 1 does not
          // have any subtext.
          assertEquals(3, sinkSubtextList.length);

          checkElementText(
              container.allSinks[1].description, sinkSubtextList[0]);

          // Route description overrides sink description for subtext.
          checkElementText(
              container.routeList[0].description, sinkSubtextList[1]);

          checkElementText(
              container.routeList[1].description, sinkSubtextList[2]);
          done();
        });
      });

      // Tests the expected view when there is only one local active route and
      // media_router_container is created for the first time.
      test('initial view with one local route', function() {
        container.allSinks = fakeSinkList;
        container.routeList = fakeRouteList;
        container.maybeShowRouteDetailsOnOpen();

        checkCurrentView(media_router.MediaRouterView.ROUTE_DETAILS);
      });

      // Tests the expected view when there are multiple local active routes
      // and media_router_container is created for the first time.
      test('initial view with multiple local routes', function() {
        container.allSinks = fakeSinkList;
        container.routeList = fakeRouteListWithLocalRoutesOnly;

        checkCurrentView(media_router.MediaRouterView.SINK_LIST);
      });

      // Tests the expected view when there are no local active routes and
      // media_router_container is created for the first time.
      test('initial view with no local route', function() {
        container.allSinks = fakeSinkList;
        container.routeList = [];

        checkCurrentView(media_router.MediaRouterView.SINK_LIST);
      });

      // Tests the expected view when there are no local active routes and
      // media_router_container is created for the first time.
      test('view after route is closed remotely', function() {
        container.allSinks = fakeSinkList;
        container.routeList = fakeRouteList;
        container.maybeShowRouteDetailsOnOpen();
        checkCurrentView(media_router.MediaRouterView.ROUTE_DETAILS);

        container.routeList = [];
        checkCurrentView(media_router.MediaRouterView.SINK_LIST);
      });

      // Tests for expected visible UI when the view is ROUTE_DETAILS.
      test('route details visibility', function(done) {
        container.showRouteDetails_(
            new media_router.Route('id 3', 'sink id 3', 'Title 3', 0, true));
        setTimeout(function() {
          checkElementsVisibleWithId(
              ['container-header', 'device-missing', 'route-details']);
          done();
        });
      });

      test('updated route in route details', function(done) {
        container.allSinks = fakeSinkList;
        var description = 'Title';
        var route = new media_router.Route(
            'id 1', 'sink id 1', description, 0, true, false);
        container.routeList = [route];
        container.showRouteDetails_(route);
        setTimeout(function() {
          // Note that sink-list-view is hidden.
          checkElementsVisibleWithId(
              ['container-header', 'route-details', 'sink-list']);
          assertTrue(!!container.currentRoute_);
          assertEquals(description, container.currentRoute_.description);

          var newDescription = 'Foo';
          route.description = newDescription;
          container.routeList = [route];
          setTimeout(function() {
            // Note that sink-list-view is hidden.
            checkElementsVisibleWithId(
                ['container-header', 'route-details', 'sink-list']);
            assertTrue(!!container.currentRoute_);
            assertEquals(newDescription, container.currentRoute_.description);
            done();
          });
        });
      });

      // Tests for expected visible UI when the view is ROUTE_DETAILS, and there
      // is a non-blocking issue.
      test('route details visibility non blocking issue', function(done) {
        container.showRouteDetails_(
            new media_router.Route('id 3', 'sink id 3', 'Title 3', 0, true));

        // Set a non-blocking issue. The issue should be shown.
        container.issue = fakeNonBlockingIssue;
        setTimeout(function() {
          checkElementsVisibleWithId(
              ['container-header', 'device-missing', 'route-details']);
          done();
        });
      });

      // Tests for expected visible UI when the view is ROUTE_DETAILS, and there
      // is a blocking issue.
      test('route details visibility with blocking issue', function(done) {
        container.showRouteDetails_(
            new media_router.Route('id 3', 'sink id 3', 'Title 3', 0, true));

        // Set a blocking issue. The issue should be shown, and everything
        // else, hidden.
        container.issue = fakeBlockingIssue;
        setTimeout(function() {
          checkElementsVisibleWithId(
              ['container-header', 'device-missing', 'issue-banner']);
          done();
        });
      });

      test('creating route with selected cast mode', function(done) {
        container.allSinks = fakeSinkList;
        MockInteractions.tap(
            container.$$('#container-header').$['arrow-drop-icon']);
        setTimeout(function() {
          // Select cast mode 2.
          var castModeList =
              container.$$('#cast-mode-list').querySelectorAll('paper-item');
          MockInteractions.tap(castModeList[1]);
          assertEquals(fakeCastModeList[1].description, container.headerText);
          setTimeout(function() {
            var sinkList = container.shadowRoot.getElementById('sink-list')
                               .querySelectorAll('paper-item');
            container.addEventListener('create-route', function(data) {
              assertEquals(fakeSinkList[2].id, data.detail.sinkId);
              // Cast mode 2 is used, since we selected it explicitly.
              assertEquals(
                  fakeCastModeList[1].type, data.detail.selectedCastModeValue);
              done();
            });
            // All sinks are compatible with cast mode 2.
            assertEquals(fakeSinkList.length, sinkList.length);
            // Tap on a sink without a route, which should fire a
            // 'create-route' event.
            MockInteractions.tap(sinkList[2]);
          });
        });
      });

      // Tests that the route details cast button is not shown when another sink
      // is launching.
      test('no cast button when sink launching', function(done) {
        container.allSinks = fakeSinkList;
        container.routeList = fakeRouteList;
        setTimeout(function() {
          var sinkList =
              container.$$('#sink-list').querySelectorAll('paper-item');
          MockInteractions.tap(sinkList[2]);
          setTimeout(function() {
            assertTrue(!!container.currentLaunchingSinkId_);
            MockInteractions.tap(sinkList[0]);
            setTimeout(function() {
              checkElementVisible(
                  false,
                  container.$$('#route-details')
                      .$$('#start-casting-to-route-button'),
                  'start-casting-to-route-button');
              // The other sink stopped launching, due to route failure, so the
              // cast button should now appear.
              container.onCreateRouteResponseReceived(
                  fakeSinkList[0].id, null, true);
              setTimeout(function() {
                checkElementVisible(
                    true,
                    container.$$('#route-details')
                        .$$('#start-casting-to-route-button'),
                    'start-casting-to-route-button');
                done();
              });
            });
          });
        });
      });

      // Tests that the route details cast button is shown when the current
      // route has no |currentCastMode| field defined.
      test('cast button when no current cast mode', function(done) {
        container.allSinks = fakeSinkList;
        container.routeList = fakeRouteList;
        setTimeout(function() {
          assertTrue(!container.currentLaunchingSinkId_);
          MockInteractions.tap(
              container.$$('#sink-list').querySelectorAll('paper-item')[0]);
          setTimeout(function() {
            assertEquals(undefined, container.currentRoute_.currentCastMode);
            checkElementVisible(
                true,
                container.$$('#route-details')
                    .$$('#start-casting-to-route-button'),
                'start-casting-to-route-button');
            done();
          });
        });
      });

      // Tests that the route details cast button is shown when the user has
      // selected a cast mode that is different from the current cast mode of
      // the route.
      test('cast button when selected cast mode differs', function(done) {
        container.allSinks = fakeSinkList;
        container.routeList = fakeRouteList;
        container.castModeList = fakeCastModeList;
        for (var i = 0; i < fakeRouteList.length; ++i) {
          fakeRouteList[i].currentCastMode = 2;
        }
        MockInteractions.tap(
            container.$$('#container-header').$$('#arrow-drop-icon'));
        setTimeout(function() {
          MockInteractions.tap(container.$$('#cast-mode-list')
                                   .querySelectorAll('paper-item')[2]);
          setTimeout(function() {
            assertTrue(container.shownCastModeValue_ != 2);
            MockInteractions.tap(
                container.$$('#sink-list').querySelectorAll('paper-item')[0]);
            setTimeout(function() {
              checkElementVisible(
                  true,
                  container.$$('#route-details')
                      .$$('#start-casting-to-route-button'),
                  'start-casting-to-route-button');
              done();
            });
          });
        });
      });

      // Tests that the route details cast button is not shown when the user has
      // selected a the same cast mode as the route's current cast mode.
      test('no cast button when selected cast mode same', function(done) {
        container.allSinks = fakeSinkList;
        container.routeList = fakeRouteList;
        container.castModeList = fakeCastModeList;
        for (var i = 0; i < fakeRouteList.length; ++i) {
          fakeRouteList[i].currentCastMode = 2;
        }
        MockInteractions.tap(
            container.$$('#container-header').$$('#arrow-drop-icon'));
        setTimeout(function() {
          MockInteractions.tap(container.$$('#cast-mode-list')
                                   .querySelectorAll('paper-item')[1]);
          setTimeout(function() {
            assertEquals(2, container.shownCastModeValue_);
            MockInteractions.tap(
                container.$$('#sink-list').querySelectorAll('paper-item')[0]);
            setTimeout(function() {
              checkElementVisible(
                  false,
                  container.$$('#route-details')
                      .$$('#start-casting-to-route-button'),
                  'start-casting-to-route-button');
              done();
            });
          });
        });
      });

      // Tests that the route details cast button is not shown when AUTO cast
      // mode would pick the same cast mode for the route's sink as the route's
      // current cast mode.
      test('no cast button when auto cast mode same', function(done) {
        container.allSinks = fakeSinkList;
        container.routeList = fakeRouteList;
        container.castModeList = fakeCastModeList;
        for (var i = 0; i < fakeRouteList.length; ++i) {
          // The lowest cast mode value will be used in AUTO mode. We set it on
          // the route here so AUTO mode matches the route's current cast mode.
          fakeRouteList[i].currentCastMode = 2;
        }
        setTimeout(function() {
          assertEquals(-1, container.shownCastModeValue_);
          MockInteractions.tap(
              container.$$('#sink-list').querySelectorAll('paper-item')[0]);
          setTimeout(function() {
            checkElementVisible(
                false,
                container.$$('#route-details')
                    .$$('#start-casting-to-route-button'),
                'start-casting-to-route-button');
            done();
          });
        });
      });

      // Tests that the route details cast button is not shown when there is no
      // sink structure in the container that corresponds to the route whose
      // details are being viewed. This occurs when there is only one local
      // route, so it is shown upon opening the dialog, and its sink isn't
      // compatible with any currently available cast modes.
      test('no cast button when sink missing', function(done) {
        container.routeList = fakeRouteList;
        // Simulate opening dialog with a local route for sink that is
        // unsupported in the current context, and therefore won't be in the
        // container's sink list.
        container.showRouteDetails_(fakeRouteList[0]);
        setTimeout(function() {
          checkElementVisible(
              false,
              container.$$('#route-details')
                  .$$('#start-casting-to-route-button'),
              'start-casting-to-route-button');
          done();
        });
      });
    });
  }

  return {
    registerTests: registerTests,
  };
});
