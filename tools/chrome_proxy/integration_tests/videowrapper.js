// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// This script finds the first video element on a page and collect metrics
// for that element. This is based on src/tools/perf/metrics/media.js.

(function() {
  // VideoWrapper attaches event listeners to collect metrics.
  // The constructor starts playing the video.
  function VideoWrapper(element) {
    if (!(element instanceof HTMLVideoElement))
      throw new Error('Unrecognized video element type ' + element);
    metrics['ready'] = false;
    this.element = element;
    element.loop = false;
    // Set the basic event handlers for this HTML5 video element.
    this.element.addEventListener('loadedmetadata', this.onLoaded.bind(this));
    this.element.addEventListener('canplay', this.onCanplay.bind(this));
    this.element.addEventListener('ended', this.onEnded.bind(this));
    this.playbackTimer = new Timer();
    element.play()
  }

  VideoWrapper.prototype.onLoaded = function(e) {
    if (this.element.readyState == HTMLMediaElement.HAVE_NOTHING) {
      return
    }
    metrics['ready'] = true;
    metrics['video_height'] = this.element.videoHeight;
    metrics['video_width'] = this.element.videoWidth;
    metrics['video_duration'] = this.element.duration;
    window.__chromeProxyVideoLoaded = true;
  };

  VideoWrapper.prototype.onCanplay = function(event) {
    metrics['time_to_play_ms'] = this.playbackTimer.stop();
  };

  VideoWrapper.prototype.onEnded = function(event) {
    var time_to_end = this.playbackTimer.stop() - metrics['time_to_play_ms'];
    metrics['buffering_time_ms'] = time_to_end - this.element.duration * 1000;
    metrics['decoded_audio_bytes'] = this.element.webkitAudioDecodedByteCount;
    metrics['decoded_video_bytes'] = this.element.webkitVideoDecodedByteCount;
    metrics['decoded_frames'] = this.element.webkitDecodedFrameCount;
    metrics['dropped_frames'] = this.element.webkitDroppedFrameCount;
    window.__chromeProxyVideoEnded = true;
  };

  function MediaMetric(element) {
    if (element instanceof HTMLMediaElement)
      return new VideoWrapper(element);
    throw new Error('Unrecognized media element type.');
  }

  function Timer() {
    this.start();
  }

  Timer.prototype = {
    start: function() {
      this.start_ = getCurrentTime();
    },

    stop: function() {
      // Return delta time since start in millisecs.
      return Math.round((getCurrentTime() - this.start_) * 1000) / 1000;
    }
  };

  function getCurrentTime() {
    if (window.performance)
      return (performance.now ||
              performance.mozNow ||
              performance.msNow ||
              performance.oNow ||
              performance.webkitNow).call(window.performance);
    else
      return Date.now();
  }

  function createVideoWrappersForDocument() {
    var videos = document.querySelectorAll('video');
    switch (videos.length) {
    case 0:
      throw new Error('Page has no videos.');
    case 1:
      break;
    default:
      throw new Error('Page too many videos: ' + videos.length.toString());
    }
    new VideoWrapper(videos[0])
  }

  metrics = {};
  window.__chromeProxyCreateVideoWrappers = createVideoWrappersForDocument;
  window.__chromeProxyVideoMetrics = metrics;
  window.__chromeProxyVideoLoaded = false;
  window.__chromeProxyVideoEnded = false;
})();
