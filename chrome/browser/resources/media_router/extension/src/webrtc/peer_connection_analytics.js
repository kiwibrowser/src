// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * @fileoverview Defines UMA analytics specific to peer connection.
 */

goog.provide('mr.webrtc.PeerConnectionAnalytics');

goog.require('mr.Timing');


/** @const {*} */
mr.webrtc.PeerConnectionAnalytics = {};


/**
 * Histogram name for time taken to gather ICE candidates, from the start of
 * candidate gethering to the time the last candidate is reported.
 * @private @const {string}
 */
mr.webrtc.PeerConnectionAnalytics.ICE_CANDIDATE_GATHERING_REAL_DURATION_ =
    'MediaRouter.WebRtc.IceCandidateGathering.Duration.Real';


/**
 * Histogram name for time taken to gather ICE candidates, from the start of
 * candidate gethering to the time the the end of collection is reported.
 * @private @const {string}
 */
mr.webrtc.PeerConnectionAnalytics.ICE_CANDIDATE_GATHERING_REPORTED_DURATION_ =
    'MediaRouter.WebRtc.IceCandidateGathering.Duration.Reported';


/**
 * Records the real duration of ICE candidate gathering.
 * @param {number} value
 */
mr.webrtc.PeerConnectionAnalytics.recordIceCandidateGatheringRealDuration =
    function(value) {
  mr.Timing.recordDuration(
      mr.webrtc.PeerConnectionAnalytics.ICE_CANDIDATE_GATHERING_REAL_DURATION_,
      value);
};


/**
 * Records the reported duration of ICE candidate gathering.
 * @param {number} value
 */
mr.webrtc.PeerConnectionAnalytics.recordIceCandidateGatheringReportedDuration =
    function(value) {
  mr.Timing.recordDuration(
      mr.webrtc.PeerConnectionAnalytics
          .ICE_CANDIDATE_GATHERING_REPORTED_DURATION_,
      value);
};
