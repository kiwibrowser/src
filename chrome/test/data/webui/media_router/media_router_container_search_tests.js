// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/** @fileoverview Suite of tests for media-router-container that focus on
 * the MRPM search feature.
 */
cr.define('media_router_container_search', function() {
  /**
   * Wrapper that lets a function |f| run after the container animation promise
   * completes but also lets any UI logic run before setting up the call. This
   * is important because |container.animationPromise_| may not exist until the
   * UI logic runs or it may be updated to a new Promise.  This wrapper also
   * carries assertion errors (and any other exceptions) outside of the promise
   * back into the test since throwing in a then() or catch() doesn't stop the
   * test.
   *
   * @param {function()} f
   */
  var chainOnAnimationPromise = function(f) {
    setTimeout(function() {
      container.animationPromise_.then(f).catch(function(err) {
        setTimeout(function() {
          throw err;
        });
      });
    });
  };

  /**
   * Checks whether |view| matches the current view of |container|.
   *
   * @param {!media_router.MediaRouterView} view Expected view type.
   */
  var checkCurrentView;

  /**
   * Checks whether an element is visible. An element is visible if it exists,
   * does not have its |hidden| property set, and its |display| property is not
   * 'none'.
   *
   * @param {Element} element The element to test.
   * @param {boolean} visible Whether the element should be visible.
   */
  var checkElementVisible = function(element, visible) {
    assertEquals(
        visible,
        !!element && !element.hidden && element.style.display != 'none');
  };

  /**
   * Checks that |targetSink| is in the search result list and it is the only
   * sink with a spinner.
   *
   * @param {media_router.Sink} targetSink
   */
  var checkSpinningSinkInFilter = function(targetSink) {
    var searchResults =
        container.$$('#search-results').querySelectorAll('paper-item');
    var targets = 0;
    searchResults.forEach(function(sink) {
      var item = container.$$('#searchResults').itemForElement(sink).sinkItem;
      var spinner = sink.querySelector('paper-spinner-lite');
      var isTargetSink = item.id == targetSink.id;
      checkElementVisible(spinner, isTargetSink);
      if (isTargetSink) {
        ++targets;
      }
    });
    assertEquals(1, targets);
  };

  /**
   * Checks that |targetSink| is in the sink list and it is the only sink with a
   * spinner. Also checks that the sink list has length |length|.
   *
   * @param {media_router.Sink} targetSink
   * @param {number} length
   */
  var checkSpinningSinkInSinkList = function(targetSink, length) {
    var sinkList = container.$$('#sink-list').querySelectorAll('paper-item');
    assertEquals(length, sinkList.length);
    var targets = 0;
    sinkList.forEach(function(sink) {
      var item = container.$$('#sinkList').itemForElement(sink);
      var spinner = sink.querySelector('paper-spinner-lite');
      var isTargetSink = item.id == targetSink.id;
      checkElementVisible(spinner, isTargetSink);
      if (isTargetSink) {
        ++targets;
      }
    });
    assertEquals(1, targets);
  };

  /**
   * Media Router Container created before each test.
   * @type {?MediaRouterContainer}
   */
  var container;

  /**
   * The list of available sinks.
   * @type {!Array<!media_router.Sink>}
   */
  var fakeSinkList = [];

  /**
   * The list of available sinks plus the pseudo sink.
   * @type {!Array<!media_router.Sink>}
   */
  var fakeSinkListWithPseudoSink = [];

  /**
   * Sink returned by search.
   * @type {media_router.Sink}
   */
  var foundSink = null;

  /**
   * Example pseudo sink.
   * @type {media_router.Sink}
   */
  var pseudoSink = null;

  /**
   * Import media_router_container.html before running suite.
   */
  var doSuiteSetup = function() {
    return PolymerTest.importHtml(
        'chrome://media-router/elements/media_router_container/' +
        'media_router_container.html');
  };

  /**
   * Performs test setup before each test.
   *
   * @param {function()} done Function for async test completion.
   */
  var doSetup = function(done) {
    PolymerTest.clearBody();
    // Initialize a media-router-container before each test.
    container = document.createElement('media-router-container');
    document.body.appendChild(container);

    // Get common functions and variables.
    var test_base = media_router_container_test_base.init(container);

    checkCurrentView = test_base.checkCurrentView;
    fakeSinkList = test_base.fakeSinkList;

    pseudoSink = new media_router.Sink(
        'pseudo:test', '', null, 'domain.com', media_router.SinkIconType.CAST,
        undefined, test_base.castModeBitset);
    pseudoSink.isPseudoSink = true;
    foundSink = new media_router.Sink(
        'found sink id', 'no existing sink', null, pseudoSink.domain,
        pseudoSink.iconType, undefined, pseudoSink.castModes);
    fakeSinkListWithPseudoSink = fakeSinkList.concat([pseudoSink]);

    container.allSinks = fakeSinkListWithPseudoSink;

    // Allow for the media router container to be created, attached, and
    // listeners registered in an afterNextRender() call.
    Polymer.RenderStatus.afterNextRender(this, done);
  };

  function registerTestsPart1() {
    suite('MediaRouterContainerSearchPart1', function() {
      suiteSetup(doSuiteSetup);
      setup(doSetup);

      test('pseudo sink hidden without filter input', function(done) {
        setTimeout(function() {
          var sinkList =
              container.$$('#sink-list').querySelectorAll('paper-item');
          assertEquals(fakeSinkList.length, sinkList.length);
          MockInteractions.tap(container.$$('#sink-search-icon'));
          chainOnAnimationPromise(function() {
            var searchResults =
                container.$$('#search-results').querySelectorAll('paper-item');
            assertEquals(fakeSinkList.length, searchResults.length);
            done();
          });
        });
      });

      test('filter input adds pseudo sink', function(done) {
        var searchInput = container.$$('#sink-search-input');
        searchInput.value = 'no existing sink';
        chainOnAnimationPromise(function() {
          var searchResults =
              container.$$('#search-results').querySelectorAll('paper-item');
          assertEquals(1, searchResults.length);
          var item =
              container.$$('#searchResults').itemForElement(searchResults[0]);
          assertEquals(pseudoSink.id, item.sinkItem.id);
          done();
        });
      });

      test('filter exact match real sink hides pseudo sink', function(done) {
        var searchInput = container.$$('#sink-search-input');
        searchInput.value = fakeSinkList[0].name;
        chainOnAnimationPromise(function() {
          var searchResults =
              container.$$('#search-results').querySelectorAll('paper-item');
          assertEquals(1, searchResults.length);
          var item =
              container.$$('#searchResults').itemForElement(searchResults[0]);
          assertEquals(fakeSinkList[0].id, item.sinkItem.id);
          done();
        });
      });

      test('clicking pseudo sink starts search', function(done) {
        var searchInput = container.$$('#sink-search-input');
        searchInput.value = 'no existing sink';
        chainOnAnimationPromise(function() {
          var searchResults =
              container.$$('#search-results').querySelectorAll('paper-item');
          container.addEventListener(
              'search-sinks-and-create-route', function(data) {
                assertEquals(pseudoSink.id, data.detail.id);
                assertEquals(pseudoSink.name, data.detail.name);
                assertEquals(pseudoSink.domain, data.detail.domain);
                done();
              });
          MockInteractions.tap(searchResults[0]);
        });
      });

      test('spinner starts on pseudo sink', function(done) {
        var searchInput = container.$$('#sink-search-input');
        searchInput.value = foundSink.name;
        chainOnAnimationPromise(function() {
          var searchResults =
              container.$$('#search-results').querySelectorAll('paper-item');
          MockInteractions.tap(searchResults[0]);
          setTimeout(function() {
            searchResults =
                container.$$('#search-results').querySelectorAll('paper-item');
            assertEquals(1, searchResults.length);
            checkSpinningSinkInFilter(pseudoSink);

            searchInput.value = foundSink.name[0];
            setTimeout(function() {
              checkSpinningSinkInFilter(pseudoSink);

              searchInput.value = '';
              setTimeout(function() {
                checkSpinningSinkInFilter(pseudoSink);
                done();
              });
            });
          });
        });
      });

      test('pseudo sink shown in sink list before real sink', function(done) {
        var searchInput = container.$$('#sink-search-input');
        searchInput.value = foundSink.name;
        chainOnAnimationPromise(function() {
          var searchResults =
              container.$$('#search-results').querySelectorAll('paper-item');
          MockInteractions.tap(searchResults[0]);
          MockInteractions.tap(
              container.$['container-header'].$$('#back-button'));
          chainOnAnimationPromise(function() {
            checkCurrentView(media_router.MediaRouterView.SINK_LIST);
            checkSpinningSinkInSinkList(
                pseudoSink, fakeSinkListWithPseudoSink.length);
            done();
          });
        });
      });
    });
  }

  function registerTestsPart2() {
    suite('MediaRouterContainerSearchPart2', function() {
      suiteSetup(doSuiteSetup);
      setup(doSetup);

      test('onReceiveSearchResult updates spinner', function(done) {
        var searchInput = container.$$('#sink-search-input');
        searchInput.value = foundSink.name;
        chainOnAnimationPromise(function() {
          var searchResults =
              container.$$('#search-results').querySelectorAll('paper-item');
          MockInteractions.tap(searchResults[0]);
          container.allSinks = fakeSinkListWithPseudoSink.concat([foundSink]);
          container.onReceiveSearchResult(foundSink.id);
          setTimeout(function() {
            searchResults =
                container.$$('#search-results').querySelectorAll('paper-item');
            assertEquals(1, searchResults.length);
            checkSpinningSinkInFilter(foundSink);
            done();
          });
        });
      });

      test('sink list updates spinner', function(done) {
        var searchInput = container.$$('#sink-search-input');
        searchInput.value = foundSink.name;
        chainOnAnimationPromise(function() {
          var searchResults =
              container.$$('#search-results').querySelectorAll('paper-item');
          MockInteractions.tap(searchResults[0]);
          setTimeout(function() {
            container.onReceiveSearchResult(foundSink.id);
            container.allSinks = fakeSinkListWithPseudoSink.concat([foundSink]);
            setTimeout(function() {
              searchResults = container.$$('#search-results')
                                  .querySelectorAll('paper-item');
              assertEquals(1, searchResults.length);
              checkSpinningSinkInFilter(foundSink);
              done();
            });
          });
        });
      });

      test('route received clears spinner and search state', function(done) {
        var route = new media_router.Route(
            'id 1', foundSink.id, 'Title 1', 0, true, false);

        var searchInput = container.$$('#sink-search-input');
        searchInput.value = foundSink.name;
        chainOnAnimationPromise(function() {
          var searchResults =
              container.$$('#search-results').querySelectorAll('paper-item');
          MockInteractions.tap(searchResults[0]);
          container.allSinks = fakeSinkListWithPseudoSink.concat([foundSink]);
          container.onReceiveSearchResult(foundSink.id);
          container.onCreateRouteResponseReceived(foundSink.id, route, true);
          assertEquals(null, container.pseudoSinkSearchState_);
          setTimeout(function() {
            checkCurrentView(media_router.MediaRouterView.ROUTE_DETAILS);
            MockInteractions.tap(
                container.$['container-header'].$$('#back-button'));
            chainOnAnimationPromise(function() {
              checkCurrentView(media_router.MediaRouterView.SINK_LIST);
              sinkList =
                  container.$$('#sink-list').querySelectorAll('paper-item');
              sinkList.forEach(function(sink) {
                var spinner = sink.querySelector('paper-spinner-lite');
                checkElementVisible(spinner, false);
              });
              done();
            });
          });
        });
      });

      test('cannot create another route during search', function(done) {
        var checkCreateRoute = function() {
          done();
        };
        var checkNoCreateRoute = function() {
          assertTrue(false);
        };
        var route = new media_router.Route(
            'id 1', foundSink.id, 'Title 1', 0, true, false);
        container.addEventListener('create-route', checkNoCreateRoute);

        var searchInput = container.$$('#sink-search-input');
        searchInput.value = foundSink.name;
        chainOnAnimationPromise(function() {
          var searchResults =
              container.$$('#search-results').querySelectorAll('paper-item');
          MockInteractions.tap(searchResults[0]);
          MockInteractions.tap(
              container.$['container-header'].$$('#back-button'));
          chainOnAnimationPromise(function() {
            var sinkList =
                container.$$('#sink-list').querySelectorAll('paper-item');
            sinkList = [...sinkList];
            var sink = sinkList.find(function(sink) {
              var item = container.$$('#sinkList').itemForElement(sink);
              return fakeSinkList[0].id == item.id;
            });
            MockInteractions.tap(sink);
            container.allSinks = fakeSinkListWithPseudoSink.concat([foundSink]);
            chainOnAnimationPromise(function() {
              container.onReceiveSearchResult(foundSink.id);
              MockInteractions.tap(sink);
              container.onCreateRouteResponseReceived(
                  foundSink.id, route, true);
              chainOnAnimationPromise(function() {
                checkCurrentView(media_router.MediaRouterView.ROUTE_DETAILS);
                MockInteractions.tap(
                    container.$['container-header'].$$('#back-button'));
                container.removeEventListener(
                    'create-route', checkNoCreateRoute);
                container.addEventListener('create-route', checkCreateRoute);
                chainOnAnimationPromise(function() {
                  checkCurrentView(media_router.MediaRouterView.SINK_LIST);
                  MockInteractions.tap(sink);
                });
              });
            });
          });
        });
      });

      test('route creation failure clears spinner and search', function(done) {
        var searchInput = container.$$('#sink-search-input');
        searchInput.value = foundSink.name;
        chainOnAnimationPromise(function() {
          var searchResults =
              container.$$('#search-results').querySelectorAll('paper-item');
          MockInteractions.tap(searchResults[0]);
          container.allSinks = fakeSinkListWithPseudoSink.concat([foundSink]);
          container.onReceiveSearchResult(foundSink.id);
          container.onCreateRouteResponseReceived(pseudoSink.id, null, true);
          assertEquals(null, container.pseudoSinkSearchState_);
          setTimeout(function() {
            checkCurrentView(media_router.MediaRouterView.FILTER);
            searchResults =
                container.$$('#search-results').querySelectorAll('paper-item');
            searchResults.forEach(function(sink) {
              var spinner = sink.querySelector('paper-spinner-lite');
              checkElementVisible(spinner, false);
            });
            done();
          });
        });
      });

      test('route creation failure resets search', function(done) {
        var searchInput = container.$$('#sink-search-input');
        searchInput.value = foundSink.name;
        chainOnAnimationPromise(function() {
          var searchResults =
              container.$$('#search-results').querySelectorAll('paper-item');
          MockInteractions.tap(searchResults[0]);

          // A found sink is added as part of the search but is removed right
          // before the route failure is reported.  The filter should revert to
          // showing the pseudo sink when this is done.
          container.allSinks = fakeSinkListWithPseudoSink.concat([foundSink]);
          container.onReceiveSearchResult(foundSink.id);
          container.allSinks = fakeSinkListWithPseudoSink;
          container.onCreateRouteResponseReceived(pseudoSink.id, null, true);
          assertEquals(null, container.pseudoSinkSearchState_);
          setTimeout(function() {
            checkCurrentView(media_router.MediaRouterView.FILTER);
            searchResults =
                container.$$('#search-results').querySelectorAll('paper-item');
            assertTrue(container.searchResultsToShow_.some(function(sink) {
              return sink.sinkItem.id == pseudoSink.id;
            }));
            done();
          });
        });
      });

      test('pseudo sink with empty domain is not shown', function(done) {
        pseudoSink.domain = '';
        // Trigger |allSinks| observer to be called again with new pseudo sink
        // domain.
        container.allSinks = [];
        container.allSinks = fakeSinkListWithPseudoSink;

        var searchInput = container.$$('#sink-search-input');
        searchInput.value = foundSink.name;
        chainOnAnimationPromise(function() {
          var noMatches = container.$$('#no-search-matches');
          var searchResults = container.$$('#search-results');
          checkElementVisible(noMatches, true);
          checkElementVisible(searchResults, false);
          done();
        });
      });

      test('pseudo sink search state launching sink id', function() {
        var searchState = new PseudoSinkSearchState(pseudoSink);

        assertEquals(pseudoSink.id, searchState.checkForRealSink(fakeSinkList));
        assertEquals(
            pseudoSink.id,
            searchState.checkForRealSink(fakeSinkList.concat([foundSink])));
        assertEquals(pseudoSink.id, searchState.checkForRealSink(fakeSinkList));

        searchState.receiveSinkResponse(foundSink.id);
        assertEquals(pseudoSink.id, searchState.checkForRealSink(fakeSinkList));
        assertEquals(
            foundSink.id,
            searchState.checkForRealSink(fakeSinkList.concat([foundSink])));
        assertEquals(foundSink.id, searchState.checkForRealSink(fakeSinkList));
      });
    });
  }

  return {
    registerTestsPart1: registerTestsPart1,
    registerTestsPart2: registerTestsPart2,
  };
});
