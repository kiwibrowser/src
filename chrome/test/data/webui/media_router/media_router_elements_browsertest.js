// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/** @fileoverview Runs the Media Router Polymer elements tests. */

/** @const {string} Path to source root. */
var ROOT_PATH = '../../../../../';

// Polymer BrowserTest fixture.
GEN_INCLUDE(
    [ROOT_PATH + 'chrome/test/data/webui/polymer_browser_test_base.js']);

/**
 * Test fixture for Media Router Polymer elements.
 * @constructor
 * @extends {PolymerTest}
 */
function MediaRouterElementsBrowserTest() {}

MediaRouterElementsBrowserTest.prototype = {
  __proto__: PolymerTest.prototype,

  /** @override */
  browsePreload: 'chrome://media-router/',

  /** @override */
  accessibilityIssuesAreErrors: true,

  commandLineSwitches: [{switchName: 'media-router', switchValue: '1'}],

  // List tests for individual elements. The media-router-container tests are
  // split between several files and use common functionality from
  // media_router_container_test_base.js.
  extraLibraries: PolymerTest.getLibraries(ROOT_PATH).concat([
    'issue_banner_tests.js',
    'media_router_container_cast_mode_list_tests.js',
    'media_router_container_filter_tests.js',
    'media_router_container_first_run_flow_tests.js',
    'media_router_container_route_tests.js',
    'media_router_container_search_tests.js',
    'media_router_container_sink_list_tests.js',
    'media_router_container_test_base.js',
    'media_router_header_tests.js',
    'media_router_search_highlighter_tests.js',
    'route_controls_tests.js',
    'route_details_tests.js',
  ]),

  /**
   * Mocks the browser API methods to make them fire events instead.
   */
  installMockBrowserApi: function() {
    cr.define('media_router.browserApi', function() {
      'use strict';

      function pauseCurrentMedia() {
        document.dispatchEvent(new Event('mock-pause-current-media'));
      }

      function playCurrentMedia() {
        document.dispatchEvent(new Event('mock-play-current-media'));
      }

      function seekCurrentMedia(time) {
        var event =
            new CustomEvent('mock-seek-current-media', {detail: {time: time}});
        document.dispatchEvent(event);
      }

      function setCurrentMediaMute(mute) {
        var event = new CustomEvent(
            'mock-set-current-media-mute', {detail: {mute: mute}});
        document.dispatchEvent(event);
      }

      function setCurrentMediaVolume(volume) {
        var event = new CustomEvent(
            'mock-set-current-media-volume', {detail: {volume: volume}});
        document.dispatchEvent(event);
      }

      function setHangoutsLocalPresent(localPresent) {
        const event = new CustomEvent(
            'mock-set-hangouts-local-present',
            {detail: {localPresent: localPresent}});
        document.dispatchEvent(event);
      }

      function setMediaRemotingEnabled(enabled) {
        const event = new CustomEvent(
            'mock-set-media-remoting-enabled', {detail: {enabled: enabled}});
        document.dispatchEvent(event);
      }

      return {
        pauseCurrentMedia: pauseCurrentMedia,
        playCurrentMedia: playCurrentMedia,
        seekCurrentMedia: seekCurrentMedia,
        setCurrentMediaMute: setCurrentMediaMute,
        setCurrentMediaVolume: setCurrentMediaVolume,
        setHangoutsLocalPresent: setHangoutsLocalPresent,
        setMediaRemotingEnabled: setMediaRemotingEnabled
      };
    });
  },

  /** @override */
  setUp: function() {
    PolymerTest.prototype.setUp.call(this);
    this.installMockBrowserApi();

    // Enable when failure is resolved.
    // AX_ARIA_02: http://crbug.com/591547
    this.accessibilityAuditConfig.ignoreSelectors(
        'nonExistentAriaRelatedElement', '#input');

    // Enable when failure is resolved.
    // AX_ARIA_04: http://crbug.com/591550
    this.accessibilityAuditConfig.ignoreSelectors(
        'badAriaAttributeValue', '#input');

    // This element is used as a focus placeholder on dialog open, then
    // deleted. The user will be unable to tab to it. Remove when there is a
    // long term fix.
    this.accessibilityAuditConfig.ignoreSelectors(
        'focusableElementNotVisibleAndNotAriaHidden', '#focus-placeholder');
  },
};

TEST_F('MediaRouterElementsBrowserTest', 'IssueBanner', function() {
  issue_banner.registerTests();
  mocha.run();
});

// The media-router-container tests are being split into multiple parts due to
// timeout issues on bots.
TEST_F(
    'MediaRouterElementsBrowserTest', 'MediaRouterContainerCastModeList',
    function() {
      media_router_container_cast_mode_list.registerTests();
      mocha.run();
    });

TEST_F(
    'MediaRouterElementsBrowserTest', 'MediaRouterContainerFirstRunFlow',
    function() {
      media_router_container_first_run_flow.registerTests();
      mocha.run();
    });

TEST_F(
    'MediaRouterElementsBrowserTest', 'MediaRouterContainerRoute', function() {
      media_router_container_route.registerTests();
      mocha.run();
    });

TEST_F(
    'MediaRouterElementsBrowserTest', 'MediaRouterContainerSearchPart1',
    function() {
      media_router_container_search.registerTestsPart1();
      mocha.run();
    });

TEST_F(
    'MediaRouterElementsBrowserTest', 'MediaRouterContainerSearchPart2',
    function() {
      media_router_container_search.registerTestsPart2();
      mocha.run();
    });

TEST_F(
    'MediaRouterElementsBrowserTest', 'MediaRouterContainerSinkList',
    function() {
      media_router_container_sink_list.registerTests();
      mocha.run();
    });

// Disabling on Windows Debug due to flaky timeout on Win7 Tests (dbg)(1).
// https://crbug.com/832947
GEN('#if defined(OS_WIN) && !defined(NDEBUG)');
GEN('#define MAYBE_MediaRouterContainerFilterPart1 \\');
GEN('    DISABLED_MediaRouterContainerFilterPart1');
GEN('#else');
GEN('#define MAYBE_MediaRouterContainerFilterPart1 \\');
GEN('    MediaRouterContainerFilterPart1');
GEN('#endif');

TEST_F(
    'MediaRouterElementsBrowserTest', 'MAYBE_MediaRouterContainerFilterPart1',
    function() {
      media_router_container_filter.registerTestsPart1();
      mocha.run();
    });

TEST_F(
    'MediaRouterElementsBrowserTest', 'MediaRouterContainerFilterPart2',
    function() {
      media_router_container_filter.registerTestsPart2();
      mocha.run();
    });

TEST_F('MediaRouterElementsBrowserTest', 'MediaRouterHeader', function() {
  media_router_header.registerTests();
  mocha.run();
});

TEST_F(
    'MediaRouterElementsBrowserTest', 'MediaRouterSearchHighlighter',
    function() {
      media_router_search_highlighter.registerTests();
      mocha.run();
    });

TEST_F(
    'MediaRouterElementsBrowserTest', 'MediaRouterRouteControls', function() {
      route_controls.registerTests();
      mocha.run();
    });

TEST_F('MediaRouterElementsBrowserTest', 'MediaRouterRouteDetails', function() {
  route_details.registerTests();
  mocha.run();
});
