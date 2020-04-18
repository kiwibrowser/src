// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// API invoked by the browser MediaRouterWebUIMessageHandler to communicate
// with this UI.
cr.define('media_router.ui', function() {
  'use strict';

  // The media-router-container element.
  var container = null;

  // The media-router-header element.
  var header = null;

  // The route-controls element. Is null if the route details view isn't open.
  var routeControls = null;

  // The initial height for |container|.
  var initialMaxHeight = 0;

  /**
   * Handles response of previous create route attempt.
   *
   * @param {string} sinkId The ID of the sink to which the Media Route was
   *     creating a route.
   * @param {?media_router.Route} route The newly created route that
   *     corresponds to the sink if route creation succeeded; null otherwise.
   * @param {boolean} isForDisplay Whether or not |route| is for display.
   */
  function onCreateRouteResponseReceived(sinkId, route, isForDisplay) {
    container.onCreateRouteResponseReceived(sinkId, route, isForDisplay);
  }

  /**
   * Called when the route controller for the route that is currently selected
   * is invalidated.
   */
  function onRouteControllerInvalidated() {
    container.onRouteControllerInvalidated();
  }

  /**
   * Handles the search response by forwarding |sinkId| to the container.
   *
   * @param {string} sinkId The ID of the sink found by search.
   */
  function receiveSearchResult(sinkId) {
    container.onReceiveSearchResult(sinkId);
  }

  /**
   * Sets the cast mode list.
   *
   * @param {!Array<!media_router.CastMode>} castModeList
   */
  function setCastModeList(castModeList) {
    container.castModeList = castModeList;
  }

  /**
   * Sets |container| and |header|.
   *
   * @param {!MediaRouterContainerInterface} mediaRouterContainer
   * @param {!MediaRouterHeaderElement} mediaRouterHeader
   */
  function setElements(mediaRouterContainer, mediaRouterHeader) {
    container = mediaRouterContainer;
    header = mediaRouterHeader;

    if (initialMaxHeight) {
      container.updateMaxDialogHeight(initialMaxHeight);
      initialMaxHeight = 0;
    }
  }

  /**
   * Populates the WebUI with data obtained about the first run flow.
   *
   * @param {{firstRunFlowCloudPrefLearnMoreUrl: string,
   *          firstRunFlowLearnMoreUrl: string,
   *          wasFirstRunFlowAcknowledged: boolean,
   *          showFirstRunFlowCloudPref: boolean}} data
   * Parameters in data:
   *   firstRunFlowCloudPrefLearnMoreUrl - url to open when the cloud services
   *       pref learn more link is clicked.
   *   firstRunFlowLearnMoreUrl - url to open when the first run flow learn
   *       more link is clicked.
   *   wasFirstRunFlowAcknowledged - true if first run flow was previously
   *       acknowledged by user.
   *   showFirstRunFlowCloudPref - true if the cloud pref option should be
   *       shown.
   */
  function setFirstRunFlowData(data) {
    container.firstRunFlowCloudPrefLearnMoreUrl =
        data['firstRunFlowCloudPrefLearnMoreUrl'];
    container.firstRunFlowLearnMoreUrl = data['firstRunFlowLearnMoreUrl'];
    container.showFirstRunFlowCloudPref = data['showFirstRunFlowCloudPref'];
    // Some users acknowledged the first run flow before the cloud prefs
    // setting was implemented. These users will see the first run flow
    // again.
    container.showFirstRunFlow = !data['wasFirstRunFlowAcknowledged'] ||
        container.showFirstRunFlowCloudPref;
  }

  /**
   * Populates the WebUI with data obtained from Media Router.
   *
   * @param {{deviceMissingUrl: string,
   *          sinksAndIdentity: {
   *            sinks: !Array<!media_router.Sink>,
   *            showEmail: boolean,
   *            userEmail: string,
   *            showDomain: boolean
   *          },
   *          routes: !Array<!media_router.Route>,
   *          castModes: !Array<!media_router.CastMode>,
   *          useTabMirroring: boolean}} data
   * Parameters in data:
   *   deviceMissingUrl - url to be opened on "Device missing?" clicked.
   *   sinksAndIdentity - list of sinks to be displayed and user identity.
   *   useWebUiRouteControls - whether new WebUI route controls should be used.
   *   routes - list of routes that are associated with the sinks.
   *   castModes - list of available cast modes.
   *   useTabMirroring - whether the cast mode should be set to TAB_MIRROR.
   */
  function setInitialData(data) {
    container.deviceMissingUrl = data['deviceMissingUrl'];
    container.castModeList = data['castModes'];
    this.setSinkListAndIdentity(data['sinksAndIdentity']);
    container.routeList = data['routes'];
    container.maybeShowRouteDetailsOnOpen();
    if (data['useTabMirroring'])
      container.selectCastMode(media_router.CastModeType.TAB_MIRROR);
    media_router.browserApi.onInitialDataReceived();
  }

  /**
   * Sets current issue to |issue|, or clears the current issue if |issue| is
   * null.
   *
   * @param {?media_router.Issue} issue
   */
  function setIssue(issue) {
    container.issue = issue;
  }

  /**
   * Sets |routeControls|. The argument may be null if the route details view is
   * getting closed.
   *
   * @param {?RouteControlsInterface} mediaRouterRouteControls
   */
  function setRouteControls(mediaRouterRouteControls) {
    routeControls = mediaRouterRouteControls;
  }

  /**
   * Sets the list of currently active routes.
   *
   * @param {!Array<!media_router.Route>} routeList
   */
  function setRouteList(routeList) {
    container.routeList = routeList;
  }

  /**
   * Sets the list of discovered sinks along with properties of whether to hide
   * identity of the user email and domain.
   *
   * @param {{sinks: !Array<!media_router.Sink>,
   *          showEmail: boolean,
   *          userEmail: string,
   *          showDomain: boolean}} data
   * Parameters in data:
   *   sinks - list of sinks to be displayed.
   *   showEmail - true if the user email should be shown.
   *   userEmail - email of the user if the user is signed in.
   *   showDomain - true if the user domain should be shown.
   */
  function setSinkListAndIdentity(data) {
    container.showDomain = data['showDomain'];
    container.allSinks = data['sinks'];
    header.userEmail = data['userEmail'];
    header.showEmail = data['showEmail'];
  }

  /**
   * Updates the max height of the dialog
   *
   * @param {number} height
   */
  function updateMaxHeight(height) {
    if (container) {
      container.updateMaxDialogHeight(height);
    } else {
      // Update the max height once |container| gets set.
      initialMaxHeight = height;
    }
  }

  /**
   * Updates the route status shown in the route controls.
   *
   * @param {!media_router.RouteStatus} status
   */
  function updateRouteStatus(status) {
    if (routeControls) {
      routeControls.routeStatus = status;
    }
  }

  function userSelectedLocalMediaFile(fileName) {
    container.onFileDialogSuccess(fileName);
  }

  return {
    onCreateRouteResponseReceived: onCreateRouteResponseReceived,
    onRouteControllerInvalidated: onRouteControllerInvalidated,
    receiveSearchResult: receiveSearchResult,
    setCastModeList: setCastModeList,
    setElements: setElements,
    setFirstRunFlowData: setFirstRunFlowData,
    setInitialData: setInitialData,
    setIssue: setIssue,
    setRouteControls: setRouteControls,
    setRouteList: setRouteList,
    setSinkListAndIdentity: setSinkListAndIdentity,
    updateMaxHeight: updateMaxHeight,
    updateRouteStatus: updateRouteStatus,
    userSelectedLocalMediaFile: userSelectedLocalMediaFile,
  };
});
