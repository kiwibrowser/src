// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// API invoked by this UI to communicate with the browser WebUI message handler.
cr.define('media_router.browserApi', function() {
  'use strict';

  /**
   * Indicates that the user has acknowledged the first run flow.
   *
   * @param {boolean} optedIntoCloudServices Whether or not the user opted into
   *                  cloud services.
   */
  function acknowledgeFirstRunFlow(optedIntoCloudServices) {
    chrome.send('acknowledgeFirstRunFlow', [optedIntoCloudServices]);
  }

  /**
   * Acts on the given issue.
   *
   * @param {number} issueId
   * @param {number} actionType Type of action that the user clicked.
   * @param {?number} helpPageId The numeric help center ID.
   */
  function actOnIssue(issueId, actionType, helpPageId) {
    chrome.send(
        'actOnIssue',
        [{issueId: issueId, actionType: actionType, helpPageId: helpPageId}]);
  }

  /**
   * Modifies |route| by changing its source to the one identified by
   * |selectedCastMode|.
   *
   * @param {!media_router.Route} route The route being modified.
   * @param {number} selectedCastMode The value of the cast mode the user
   *   selected.
   */
  function changeRouteSource(route, selectedCastMode) {
    chrome.send(
        'requestRoute',
        [{sinkId: route.sinkId, selectedCastMode: selectedCastMode}]);
  }

  /**
   * Closes the dialog.
   *
   * @param {boolean} pressEscToClose Whether the user pressed ESC to close the
   *                  dialog.
   */
  function closeDialog(pressEscToClose) {
    chrome.send('closeDialog', [pressEscToClose]);
  }

  /**
   * Closes the given route.
   *
   * @param {!media_router.Route} route
   */
  function closeRoute(route) {
    chrome.send('closeRoute', [{routeId: route.id, isLocal: route.isLocal}]);
  }

  /**
   * Joins the given route.
   *
   * @param {!media_router.Route} route
   */
  function joinRoute(route) {
    chrome.send('joinRoute', [{sinkId: route.sinkId, routeId: route.id}]);
  }

  /**
   * Indicates that the initial data has been received.
   */
  function onInitialDataReceived() {
    chrome.send('onInitialDataReceived');
  }

  /**
   * Reports that the route details view was closed.
   */
  function onMediaControllerClosed() {
    chrome.send('onMediaControllerClosed');
  }

  /**
   * Reports that the route details view was opened for |routeId|.
   *
   * @param {string} routeId
   */
  function onMediaControllerAvailable(routeId) {
    chrome.send('onMediaControllerAvailable', [{routeId: routeId}]);
  }

  /**
   * Sends a command to pause the route shown in the route details view.
   */
  function pauseCurrentMedia() {
    chrome.send('pauseCurrentMedia');
  }

  /**
   * Sends a command to play the route shown in the route details view.
   */
  function playCurrentMedia() {
    chrome.send('playCurrentMedia');
  }

  /**
   * Reports when the user clicks outside the dialog.
   */
  function reportBlur() {
    chrome.send('reportBlur');
  }

  /**
   * Reports the index of the selected sink.
   *
   * @param {number} sinkIndex
   */
  function reportClickedSinkIndex(sinkIndex) {
    chrome.send('reportClickedSinkIndex', [sinkIndex]);
  }

  /**
   * Reports that the user used the filter input.
   */
  function reportFilter() {
    chrome.send('reportFilter');
  }

  /**
   * Reports the initial dialog view.
   *
   * @param {string} view
   */
  function reportInitialState(view) {
    chrome.send('reportInitialState', [view]);
  }

  /**
   * Reports the initial action the user took.
   *
   * @param {number} action
   */
  function reportInitialAction(action) {
    chrome.send('reportInitialAction', [action]);
  }

  /**
   * Reports the navigation to the specified view.
   *
   * @param {string} view
   */
  function reportNavigateToView(view) {
    chrome.send('reportNavigateToView', [view]);
  }

  /**
   * Reports whether or not a route was created successfully.
   *
   * @param {boolean} success
   */
  function reportRouteCreation(success) {
    chrome.send('reportRouteCreation', [success]);
  }

  /**
   * Reports the outcome of a create route response.
   *
   * @param {number} outcome
   */
  function reportRouteCreationOutcome(outcome) {
    chrome.send('reportRouteCreationOutcome', [outcome]);
  }

  /**
   * Reports the cast mode that the user selected.
   *
   * @param {number} castModeType
   */
  function reportSelectedCastMode(castModeType) {
    chrome.send('reportSelectedCastMode', [castModeType]);
  }

  /**
   * Reports the current number of sinks.
   *
   * @param {number} sinkCount
   */
  function reportSinkCount(sinkCount) {
    chrome.send('reportSinkCount', [sinkCount]);
  }

  /**
   * Reports the time it took for the user to select a sink after the sink list
   * is populated and shown.
   *
   * @param {number} timeMs
   */
  function reportTimeToClickSink(timeMs) {
    chrome.send('reportTimeToClickSink', [timeMs]);
  }

  /**
   * Reports the time, in ms, it took for the user to close the dialog without
   * taking any other action.
   *
   * @param {number} timeMs
   */
  function reportTimeToInitialActionClose(timeMs) {
    chrome.send('reportTimeToInitialActionClose', [timeMs]);
  }

  /**
   * Reports the time, in ms, it took the WebUI route controller to load media
   * status info.
   *
   * @param {number} timeMs
   */
  function reportWebUIRouteControllerLoaded(timeMs) {
    chrome.send('reportWebUIRouteControllerLoaded', [timeMs]);
  }

  /**
   * Requests data to initialize the WebUI with.
   * The data will be returned via media_router.ui.setInitialData.
   */
  function requestInitialData() {
    chrome.send('requestInitialData');
  }

  /**
   * Requests that a media route be started with the given sink.
   *
   * @param {string} sinkId The sink ID.
   * @param {number} selectedCastMode The value of the cast mode the user
   *   selected.
   */
  function requestRoute(sinkId, selectedCastMode) {
    chrome.send(
        'requestRoute', [{sinkId: sinkId, selectedCastMode: selectedCastMode}]);
  }

  /**
   * Requests that the media router search all providers for a sink matching
   * |searchCriteria| that can be used with the media source associated with the
   * cast mode |selectedCastMode|. If such a sink is found, a route is also
   * created between the sink and the media source.
   *
   * @param {string} sinkId Sink ID of the pseudo sink generating the request.
   * @param {string} searchCriteria Search criteria for the route providers.
   * @param {string} domain User's current hosted domain.
   * @param {number} selectedCastMode The value of the cast mode to be used with
   *   the sink.
   */
  function searchSinksAndCreateRoute(
      sinkId, searchCriteria, domain, selectedCastMode) {
    chrome.send('searchSinksAndCreateRoute', [{
                  sinkId: sinkId,
                  searchCriteria: searchCriteria,
                  domain: domain,
                  selectedCastMode: selectedCastMode
                }]);
  }

  /**
   * Sends a command to seek the route shown in the route details view.
   *
   * @param {number} time The new current time in seconds.
   */
  function seekCurrentMedia(time) {
    chrome.send('seekCurrentMedia', [{time: time}]);
  }

  /**
   * Sends a command to open a file dialog and allow the user to choose a local
   * media file.
   */
  function selectLocalMediaFile() {
    chrome.send('selectLocalMediaFile');
  }

  /**
   * Sends a command to mute or unmute the route shown in the route details
   * view.
   *
   * @param {boolean} mute Mute the route if true, unmute it if false.
   */
  function setCurrentMediaMute(mute) {
    chrome.send('setCurrentMediaMute', [{mute: mute}]);
  }

  /**
   * Sends a command to change the volume of the route shown in the route
   * details view.
   *
   * @param {number} volume The volume between 0 and 1.
   */
  function setCurrentMediaVolume(volume) {
    chrome.send('setCurrentMediaVolume', [{volume: volume}]);
  }

  /**
   * Sets the local present mode of the Hangouts associated with the current
   * route.
   * @param {boolean} localPresent
   */
  function setHangoutsLocalPresent(localPresent) {
    chrome.send('hangouts.setLocalPresent', [localPresent]);
  }

  /**
   * Sends a command to change the Media Remoting enabled value associated with
   * current route.
   * @param {boolean} enabled
   */
  function setMediaRemotingEnabled(enabled) {
    chrome.send('setMediaRemotingEnabled', [enabled]);
  }

  return {
    acknowledgeFirstRunFlow: acknowledgeFirstRunFlow,
    actOnIssue: actOnIssue,
    changeRouteSource: changeRouteSource,
    closeDialog: closeDialog,
    closeRoute: closeRoute,
    joinRoute: joinRoute,
    onInitialDataReceived: onInitialDataReceived,
    onMediaControllerClosed: onMediaControllerClosed,
    onMediaControllerAvailable: onMediaControllerAvailable,
    pauseCurrentMedia: pauseCurrentMedia,
    playCurrentMedia: playCurrentMedia,
    reportBlur: reportBlur,
    reportClickedSinkIndex: reportClickedSinkIndex,
    reportFilter: reportFilter,
    reportInitialAction: reportInitialAction,
    reportInitialState: reportInitialState,
    reportNavigateToView: reportNavigateToView,
    reportRouteCreation: reportRouteCreation,
    reportRouteCreationOutcome: reportRouteCreationOutcome,
    reportSelectedCastMode: reportSelectedCastMode,
    reportSinkCount: reportSinkCount,
    reportTimeToClickSink: reportTimeToClickSink,
    reportTimeToInitialActionClose: reportTimeToInitialActionClose,
    reportWebUIRouteControllerLoaded: reportWebUIRouteControllerLoaded,
    requestInitialData: requestInitialData,
    requestRoute: requestRoute,
    searchSinksAndCreateRoute: searchSinksAndCreateRoute,
    seekCurrentMedia: seekCurrentMedia,
    selectLocalMediaFile: selectLocalMediaFile,
    setCurrentMediaMute: setCurrentMediaMute,
    setCurrentMediaVolume: setCurrentMediaVolume,
    setHangoutsLocalPresent: setHangoutsLocalPresent,
    setMediaRemotingEnabled: setMediaRemotingEnabled
  };
});
