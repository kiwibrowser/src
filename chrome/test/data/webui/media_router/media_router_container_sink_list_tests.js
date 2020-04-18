// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/** @fileoverview Suite of tests for media-router-container that focus on the
 * sink list.
 */
cr.define('media_router_container_sink_list', function() {
  function registerTests() {
    suite('MediaRouterContainerSinkList', function() {
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
        var test_base = media_router_container_test_base.init(container);

        checkCurrentView = test_base.checkCurrentView;
        checkElementsVisibleWithId = test_base.checkElementsVisibleWithId;
        checkElementText = test_base.checkElementText;
        fakeBlockingIssue = test_base.fakeBlockingIssue;
        fakeCastModeList = test_base.fakeCastModeList;
        fakeNonBlockingIssue = test_base.fakeNonBlockingIssue;
        fakeSinkList = test_base.fakeSinkList;

        container.castModeList = test_base.fakeCastModeList;

        // Allow for the media router container to be created, attached, and
        // listeners registered in an afterNextRender() call.
        Polymer.RenderStatus.afterNextRender(this, done);
      });

      // Tests that text shown for each sink matches their names.
      test('sink list text', function(done) {
        container.allSinks = fakeSinkList;

        setTimeout(function() {
          var sinkList = container.shadowRoot.getElementById('sink-list')
                             .querySelectorAll('paper-item');
          assertEquals(fakeSinkList.length, sinkList.length);
          for (var i = 0; i < fakeSinkList.length; i++) {
            checkElementText(fakeSinkList[i].name, sinkList[i]);
          }
          done();
        });
      });

      // Tests that text shown for each sink matches their names.
      test('updated sink list', function(done) {
        var sinkOne = new media_router.Sink(
            'sink id 1', 'Sink 1', null, null,
            media_router.SinkIconType.GENERIC, media_router.SinkStatus.IDLE,
            [1, 2, 3]);
        var sinkTwo = new media_router.Sink(
            'sink id 2', 'Sink 2', null, 'example.com',
            media_router.SinkIconType.GENERIC, media_router.SinkStatus.IDLE,
            [1, 2, 3]);
        var sinkThree = new media_router.Sink(
            'sink id 3', 'Sink 3', null, 'example.com',
            media_router.SinkIconType.GENERIC, media_router.SinkStatus.IDLE,
            [1, 2, 3]);
        var sinkFour = new media_router.Sink(
            'sink id 4', 'Sink 4', null, 'example.com',
            media_router.SinkIconType.GENERIC, media_router.SinkStatus.IDLE,
            [1, 2, 3]);

        // Set the initial sink list and check that the order corresponds.
        var listOne = [sinkOne, sinkTwo];
        var listOneExpected = [sinkOne, sinkTwo];
        container.allSinks = listOne;
        setTimeout(function() {
          var sinkList = container.shadowRoot.getElementById('sink-list')
                             .querySelectorAll('paper-item');
          assertEquals(listOne.length, sinkList.length);
          for (var i = 0; i < listOneExpected.length; i++) {
            checkElementText(listOneExpected[i].name, sinkList[i]);
          }

          // Update the sink list with a new sink, but not at the end of the
          // array. The existing sinks should appear first, then the new
          // sink.
          var listTwo = [sinkOne, sinkThree, sinkTwo];
          var listTwoExpected = [sinkOne, sinkTwo, sinkThree];
          container.allSinks = listTwo;
          setTimeout(function() {
            sinkList = container.shadowRoot.getElementById('sink-list')
                           .querySelectorAll('paper-item');
            assertEquals(listTwo.length, sinkList.length);
            for (var i = 0; i < listTwoExpected.length; i++) {
              checkElementText(listTwoExpected[i].name, sinkList[i]);
            }

            // If any sinks are not included in a sink list update, remove
            // them from the sink list.
            var listThree = [sinkFour, sinkOne];
            var listThreeExpected = [sinkOne, sinkFour];
            container.allSinks = listThree;
            setTimeout(function() {
              sinkList = container.shadowRoot.getElementById('sink-list')
                             .querySelectorAll('paper-item');
              assertEquals(listThree.length, sinkList.length);
              for (var i = 0; i < listThreeExpected.length; i++) {
                checkElementText(listThreeExpected[i].name, sinkList[i]);
              }
              done();
            });
          });
        });
      });

      // Tests that text shown for sink with domain matches the name and domain.
      test('sink with domain text', function(done) {
        // Sink 1 - sink, no domain -> text = name
        // Sink 2 - sink, domain -> text = sink + domain
        container.allSinks = [
          new media_router.Sink(
              'sink id 1', 'Sink 1', null, null,
              media_router.SinkIconType.HANGOUT, media_router.SinkStatus.ACTIVE,
              [1, 2, 3]),
          new media_router.Sink(
              'sink id 2', 'Sink 2', null, 'example.com',
              media_router.SinkIconType.HANGOUT, media_router.SinkStatus.ACTIVE,
              [1, 2, 3]),
        ];

        container.showDomain = true;

        setTimeout(function() {
          var sinkList = container.shadowRoot.getElementById('sink-list')
                             .querySelectorAll('paper-item');
          assertEquals(2, sinkList.length);

          // |sinkList[0]| has sink name only.
          checkElementText(container.allSinks[0].name, sinkList[0]);
          // |sinkList[1]| contains sink name and domain.
          assertTrue(sinkList[1].textContent.trim().startsWith(
              container.allSinks[1].name.trim()));
          assertTrue(
              sinkList[1].textContent.trim().indexOf(
                  container.allSinks[1].domain.trim()) != -1);
          done();
        });
      });

      // Tests that domain text is not shown when |showDomain| is false.
      test('sink with domain text', function(done) {
        // Sink 1 - sink, no domain -> text = name
        // Sink 2 - sink, domain -> text = sink + domain
        container.allSinks = [
          new media_router.Sink(
              'sink id 1', 'Sink 1', null, null,
              media_router.SinkIconType.HANGOUT, media_router.SinkStatus.ACTIVE,
              [1, 2, 3]),
          new media_router.Sink(
              'sink id 2', 'Sink 2', null, 'example.com',
              media_router.SinkIconType.HANGOUT, media_router.SinkStatus.ACTIVE,
              [1, 2, 3]),
        ];

        container.showDomain = false;

        setTimeout(function() {
          var sinkList = container.shadowRoot.getElementById('sink-list')
                             .querySelectorAll('paper-item');
          assertEquals(2, sinkList.length);

          // |sinkList[0]| has sink name only.
          checkElementText(container.allSinks[0].name, sinkList[0]);
          // |sinkList[1]| has sink name but domain should be hidden.
          checkElementText(container.allSinks[1].name, sinkList[1]);
          assertTrue(
              sinkList[1].textContent.trim().indexOf(
                  container.allSinks[1].domain.trim()) == -1);
          done();
        });
      });

      // Tests for expected visible UI when the view is SINK_LIST.
      test('sink list state visibility', function() {
        container.showSinkList_();
        checkElementsVisibleWithId(
            ['container-header', 'device-missing', 'sink-list-view']);

        // Set an non-empty sink list.
        container.allSinks = fakeSinkList;
        setTimeout(function() {
          checkElementsVisibleWithId(
              ['container-header', 'sink-list', 'sink-list-view']);
        });
      });

      // Tests for expected visible UI when the view is SINK_LIST, and there is
      // a non blocking issue. Also tests for expected visible UI when the
      // issue is cleared.
      test('sink list visibility non blocking issue', function(done) {
        container.showSinkList_();
        checkCurrentView(media_router.MediaRouterView.SINK_LIST);

        // Set an non-empty sink list.
        container.allSinks = fakeSinkList;

        // Set a non-blocking issue. The issue should be shown.
        container.issue = fakeNonBlockingIssue;
        setTimeout(function() {
          checkCurrentView(media_router.MediaRouterView.SINK_LIST);
          checkElementsVisibleWithId([
            'container-header', 'issue-banner', 'sink-list', 'sink-list-view'
          ]);
          // Replace issue with null.
          container.issue = null;
          setTimeout(function() {
            checkCurrentView(media_router.MediaRouterView.SINK_LIST);
            checkElementsVisibleWithId(
                ['container-header', 'sink-list', 'sink-list-view']);
            done();
          });
        });
      });

      // Tests for expected visible UI when the view is SINK_LIST, and there is
      // a blocking issue. Also tests for expected visible UI when the issue is
      // cleared.
      test('sink list visibility blocking issue', function(done) {
        container.showSinkList_();
        checkCurrentView(media_router.MediaRouterView.SINK_LIST);

        // Set an non-empty sink list.
        container.allSinks = fakeSinkList;

        // Set a blocking issue. The issue should be shown, and everything
        // else, hidden.
        container.issue = fakeBlockingIssue;
        setTimeout(function() {
          checkCurrentView(media_router.MediaRouterView.ISSUE);
          checkElementsVisibleWithId(
              ['container-header', 'issue-banner', 'sink-list']);
          // Replace issue with null.
          container.issue = null;
          setTimeout(function() {
            checkCurrentView(media_router.MediaRouterView.SINK_LIST);
            checkElementsVisibleWithId(
                ['container-header', 'sink-list', 'sink-list-view']);
            done();
          });
        });
      });

      // Tests for expected visible UI when the view is SINK_LIST, and there is
      // a blocking issue. Also tests for expected visible UI when the issue is
      // cleared.
      test(
          'sink list visibility non-blocking replaced with blocking issue',
          function(done) {
            container.showSinkList_();
            checkCurrentView(media_router.MediaRouterView.SINK_LIST);

            // Set an non-empty sink list.
            container.allSinks = fakeSinkList;

            // Set a non-blocking issue. The issue should be shown.
            container.issue = fakeNonBlockingIssue;
            setTimeout(function() {
              checkCurrentView(media_router.MediaRouterView.SINK_LIST);
              checkElementsVisibleWithId([
                'container-header', 'issue-banner', 'sink-list',
                'sink-list-view'
              ]);

              // Set a blocking issue. The issue should be shown, and everything
              // else, hidden.
              container.issue = fakeBlockingIssue;
              setTimeout(function() {
                checkCurrentView(media_router.MediaRouterView.ISSUE);
                checkElementsVisibleWithId(
                    ['container-header', 'issue-banner', 'sink-list']);
                done();
              });
            });
          });

      // Tests all sinks are always shown in auto mode, and that the mode will
      // switch if the sinks support only 1 cast mode.
      test('sink list in auto mode', function(done) {
        container.allSinks = fakeSinkList;
        setTimeout(function() {
          // Container is initially in auto mode since a cast mode has not been
          // selected.
          assertEquals(
              media_router.AUTO_CAST_MODE.description, container.headerText);
          assertEquals(
              media_router.CastModeType.AUTO, container.shownCastModeValue_);
          assertFalse(container.userHasSelectedCastMode_);
          var sinkList = container.shadowRoot.getElementById('sink-list')
                             .querySelectorAll('paper-item');

          // All sinks are shown in auto mode.
          assertEquals(3, sinkList.length);

          // When sink list changes to only 1 compatible cast mode, the mode is
          // switched, and all sinks are shown.
          container.allSinks = [
            new media_router.Sink(
                'sink id 10', 'Sink 10', null, null,
                media_router.SinkIconType.CAST, media_router.SinkStatus.ACTIVE,
                0x4),
            new media_router.Sink(
                'sink id 20', 'Sink 20', null, null,
                media_router.SinkIconType.CAST, media_router.SinkStatus.ACTIVE,
                0x4),
            new media_router.Sink(
                'sink id 30', 'Sink 30', null, null,
                media_router.SinkIconType.CAST, media_router.SinkStatus.PENDING,
                0x4),
          ];

          setTimeout(function() {
            assertEquals(fakeCastModeList[2].description, container.headerText);
            assertEquals(
                fakeCastModeList[2].type, container.shownCastModeValue_);
            assertFalse(container.userHasSelectedCastMode_);

            var sinkList = container.shadowRoot.getElementById('sink-list')
                               .querySelectorAll('paper-item');
            assertEquals(3, sinkList.length);

            // When compatible cast modes size is no longer exactly 1, switch
            // back to auto mode, and all sinks are shown.
            container.allSinks = fakeSinkList;
            setTimeout(function() {
              assertEquals(
                  media_router.AUTO_CAST_MODE.description,
                  container.headerText);
              assertEquals(
                  media_router.CastModeType.AUTO,
                  container.shownCastModeValue_);
              assertFalse(container.userHasSelectedCastMode_);
              var sinkList = container.shadowRoot.getElementById('sink-list')
                                 .querySelectorAll('paper-item');

              // All sinks are shown in auto mode.
              assertEquals(3, sinkList.length);

              done();
            });
          });
        });
      });
    });
  }

  return {
    registerTests: registerTests,
  };
});
