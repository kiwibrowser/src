// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/** @fileoverview Suite of tests for media-router-header. */
cr.define('media_router_header', function() {
  function registerTests() {
    suite('MediaRouterHeader', function() {
      /**
       * Media Router Container created before each test.
       * @type {?MediaRouterContainer}
       */
      var container;

      /**
       * Media Router Header created before each test.
       * @type {MediaRouterHeader}
       */
      var header;

      /**
       * The list of elements to check for visibility.
       * @const {!Array<string>}
       */
      var hiddenCheckElementIdList = [
        'arrow-drop-icon',
        'back-button-container',
        'close-button',
        'header-text',
        'user-email-container',
      ];

      // Checks whether the current icon matches the icon used for the view.
      var checkArrowDropIcon = function(view) {
        assertEquals(
            header.computeArrowDropIcon_(view),
            header.$['arrow-drop-icon'].icon);
      };

      // Checks whether |element| is hidden.
      // An element is considered hidden if it does not exist (e.g. unstamped)
      // or its |hidden| property is |false|.
      var checkElementHidden = function(hidden, elementId) {
        var element =
            header.$[elementId] || header.shadowRoot.getElementById(elementId);
        assertEquals(
            hidden,
            !element || element.hidden ||
                window.getComputedStyle(element, null)
                        .getPropertyValue('display') == 'none');
      };

      // Checks whether the elements specified in |elementIdList| are visible.
      // Checks whether all other elements are hidden.
      var checkElementsVisibleWithId = function(elementIdList) {
        for (var i = 0; i < elementIdList.length; i++)
          checkElementHidden(false, elementIdList[i]);

        for (var j = 0; j < hiddenCheckElementIdList.length; j++) {
          if (elementIdList.indexOf(hiddenCheckElementIdList[j]) == -1)
            checkElementHidden(true, hiddenCheckElementIdList[j]);
        }
      };

      // Checks whether |expected| and the text in the |element| are equal.
      var checkElementText = function(expected, element) {
        assertEquals(expected.trim(), element.textContent.trim());
      };

      // Import media_router_header.html before running suite.
      suiteSetup(function() {
        return PolymerTest.importHtml(
            'chrome://media-router/elements/media_router_container/' +
            'media_router_container.html');
      });

      // Initialize an media-router-header before each test.
      setup(function(done) {
        PolymerTest.clearBody();
        container = document.createElement('media-router-container');
        document.body.appendChild(container);
        header = container.$['container-header'];

        // Allow for the media router container to be created, attached, and
        // listeners registered in an afterNextRender() call.
        Polymer.RenderStatus.afterNextRender(this, done);
      });

      // Tests for 'close-dialog' event firing when the close button is
      // clicked.
      test('close button click', function(done) {
        header.addEventListener('close-dialog', function(data) {
          assertFalse(data.detail.pressEscToClose);
          done();
        });
        MockInteractions.tap(header.$['close-button']);
      });

      // Tests for 'back-click' event firing when the back button
      // is clicked.
      test('back button click', function(done) {
        header.view = media_router.MediaRouterView.ROUTE_DETAILS;
        setTimeout(function() {
          header.addEventListener('back-click', function() {
            done();
          });
          MockInteractions.tap(header.shadowRoot.getElementById('back-button'));
        });
      });

      // Tests for 'header-or-arrow-click' event firing when the arrow drop
      // button is clicked on the CAST_MODE_LIST view.
      test('arrow drop icon click', function(done) {
        header.view = media_router.MediaRouterView.CAST_MODE_LIST;
        header.addEventListener('header-or-arrow-click', function() {
          done();
        });
        MockInteractions.tap(header.$['arrow-drop-icon']);
      });

      // Tests for 'header-or-arrow-click' event firing when the arrow drop
      // button is clicked on the SINK_LIST view.
      test('arrow drop icon click', function(done) {
        header.view = media_router.MediaRouterView.SINK_LIST;
        header.addEventListener('header-or-arrow-click', function() {
          done();
        });
        MockInteractions.tap(header.$['arrow-drop-icon']);
      });

      // Tests for 'header-or-arrow-click' event firing when the header text is
      // clicked on the CAST_MODE_LIST view.
      test('header text click on cast mode list view', function(done) {
        header.view = media_router.MediaRouterView.CAST_MODE_LIST;
        header.addEventListener('header-or-arrow-click', function() {
          done();
        });
        MockInteractions.tap(header.$['header-text']);
      });

      // Tests for 'header-or-arrow-click' event firing when the header text is
      // clicked on the SINK_LIST view.
      test('header text click on sink list view', function(done) {
        header.view = media_router.MediaRouterView.SINK_LIST;
        header.addEventListener('header-or-arrow-click', function() {
          done();
        });
        MockInteractions.tap(header.$['header-text']);
      });

      // Tests for no event firing when the header text is clicked on certain
      // views.
      test('header text click without event firing', function(done) {
        header.addEventListener('header-or-arrow-click', function() {
          assertNotReached();
        });

        header.view = media_router.MediaRouterView.FILTER;
        MockInteractions.tap(header.$['header-text']);
        header.view = media_router.MediaRouterView.ISSUE;
        MockInteractions.tap(header.$['header-text']);
        header.view = media_router.MediaRouterView.ROUTE_DETAILS;
        MockInteractions.tap(header.$['header-text']);
        done();
      });

      // Tests for 'header-height-changed' event firing when the header changes
      // and the email is shown.
      test('header height changed with email shown', function(done) {
        header.addEventListener('header-height-changed', function() {
          assertEquals(header.headerWithEmailHeight_, header.offsetHeight);
          done();
        });
        header.userEmail = 'user@example.com';
        header.showEmail = true;
      });

      // Test for 'header-height-changed' event firing when the header changes
      // and the email is not shown.
      test('header height changed without email shown', function(done) {
        header.userEmail = 'user@example.com';
        header.showEmail = true;
        setTimeout(function() {
          header.addEventListener('header-height-changed', function() {
            assertEquals(header.headerWithoutEmailHeight_, header.offsetHeight);
            done();
          });
          header.showEmail = false;
        });
      });

      // Tests the |computeArrowDropIcon_| function.
      test('compute arrow drop icon', function() {
        assertEquals(
            'media-router:arrow-drop-up',
            header.computeArrowDropIcon_(
                media_router.MediaRouterView.CAST_MODE_LIST));
        assertEquals(
            'media-router:arrow-drop-down',
            header.computeArrowDropIcon_(media_router.MediaRouterView.FILTER));
        assertEquals(
            'media-router:arrow-drop-down',
            header.computeArrowDropIcon_(media_router.MediaRouterView.ISSUE));
        assertEquals(
            'media-router:arrow-drop-down',
            header.computeArrowDropIcon_(
                media_router.MediaRouterView.ROUTE_DETAILS));
        assertEquals(
            'media-router:arrow-drop-down',
            header.computeArrowDropIcon_(
                media_router.MediaRouterView.SINK_LIST));
      });

      test('visibility of UI depending on view', function(done) {
        header.view = media_router.MediaRouterView.CAST_MODE_LIST;
        checkElementsVisibleWithId(
            ['arrow-drop-icon', 'close-button', 'header-text']);

        header.view = media_router.MediaRouterView.FILTER;
        setTimeout(function() {
          checkElementsVisibleWithId(
              ['back-button-container', 'close-button', 'header-text']);

          header.view = media_router.MediaRouterView.ISSUE;
          setTimeout(function() {
            checkElementsVisibleWithId(['close-button', 'header-text']);

            header.view = media_router.MediaRouterView.ROUTE_DETAILS;
            setTimeout(function() {
              checkElementsVisibleWithId(
                  ['back-button-container', 'close-button', 'header-text']);

              header.view = media_router.MediaRouterView.SINK_LIST;
              setTimeout(function() {
                checkElementsVisibleWithId(
                    ['arrow-drop-icon', 'close-button', 'header-text']);
                done();
              });
            });
          });
        });
      });

      // Verify email is shown and header updated if showEmail is true.
      test('visibility and style of UI depending on email', function(done) {
        header.userEmail = 'user@example.com';
        header.showEmail = true;
        setTimeout(function() {
          assertEquals(header.headerWithEmailHeight_, header.offsetHeight);

          assertFalse(header.$$('#user-email-container').hidden);
          checkElementText(
              header.userEmail, header.$$('#user-email-container'));
          done();
        });
      });

      // Verify no email is shown and header is not modified if email is empty.
      test('visibility and style of UI for empty email', function(done) {
        header.userEmail = undefined;
        header.showEmail = true;
        setTimeout(function() {
          assertNotEquals(header.headerWithEmailHeight_, header.offsetHeight);
          checkElementText('', header.$$('#user-email-container'));
          done();
        });
      });
    });
  }

  return {
    registerTests: registerTests,
  };
});
