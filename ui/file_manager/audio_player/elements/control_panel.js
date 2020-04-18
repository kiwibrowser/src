// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * @typedef {?{
 *   mute: string,
 *   next: string,
 *   pause: string,
 *   play: string,
 *   playList: string,
 *   previous: string,
 *   repeat: string,
 *   seekSlider: string,
 *   shuffle: string,
 *   unmute: string,
 *   volume: string,
 *   volumeSlider: string,
 * }}
 */
var AriaLabels;

(function() {
  'use strict';

  /**
   * Moves |target| element above |anchor| element, in order to match the
   * bottom lines.
   * @param {HTMLElement} target Target element.
   * @param {HTMLElement} anchor Anchor element.
   */
  function matchBottomLine(target, anchor) {
    var targetRect = target.getBoundingClientRect();
    var anchorRect = anchor.getBoundingClientRect();

    var pos = {
      left: anchorRect.left + anchorRect.width / 2 - targetRect.width / 2,
      bottom: window.innerHeight - anchorRect.bottom,
    };

    target.style.position = 'fixed';
    target.style.left = pos.left + 'px';
    target.style.bottom = pos.bottom + 'px';
  }

  Polymer({
    is: 'control-panel',

    properties: {
      /**
       * Flag whether the audio is playing or paused. True if playing, or false
       * paused.
       */
      playing: {
        type: Boolean,
        value: false,
        notify: true,
        reflectToAttribute: true,
        observer: 'playingChanged_'
      },

      /**
       * Current elapsed time in the current music in millisecond.
       */
      time: {
        type: Number,
        value: 0,
        notify: true
      },

      /**
       * Current seeking position on the time slider in millisecond.
       */
      seekingTime: {
        type: Number,
        value: 0,
        readOnly: true
      },

      /**
       * Total length of the current music in millisecond.
       */
      duration: {
        type: Number,
        value: 0
      },

      /**
       * Whether the shuffle button is ON.
       */
      shuffle: {
        type: Boolean,
        value: false,
        notify: true
      },

      /**
       * What mode the repeat button idicates.
       * repeat-modes can be "no-repeat", "repeat-all", "repeat-one".
       */
      repeatMode: {
        type: String,
        value: "no-repeat",
        notify: true
      },

      /**
       * The audio volume. 0 is silent, and 100 is maximum loud.
       */
      volume: {
        type: Number,
        value: 50,
        notify: true,
        reflectToAttribute: true,
        observer: 'volumeChanged_'
      },

      /**
       * Whether the playlist is expanded or not.
       */
      playlistExpanded: {
        type: Boolean,
        value: false,
        notify: true
      },

      /**
       * Whether the knob of time slider is being dragged.
       */
      dragging: {
        type: Boolean,
        value: false,
        notify: true
      },

      /**
       * Dictionary which contains aria-labels for each controls.
       * @type {AriaLabels}
       */
      ariaLabels: {
        type: Object,
        observer: 'ariaLabelsChanged_'
      }
    },

    /**
     * Initializes an element. This method is called automatically when the
     * element is ready.
     */
    ready: function() {
      var timeSlider = /** @type {PaperSliderElement} */ (this.$.timeSlider);
      timeSlider.addEventListener('change', function() {
        if (this.dragging)
          this.dragging = false;
        this._setSeekingTime(0);
      }.bind(this));
      timeSlider.addEventListener('immediate-value-change', function() {
        this._setSeekingTime(timeSlider.immediateValue);
        if (!this.dragging)
          this.dragging = true;
      }.bind(this));

      // Update volume on user inputs for volume slider.
      // During a drag operation, the volume should be updated immediately.
      var volumeSlider =
          /** @type {PaperSliderElement} */ (this.$.volumeSlider);
      volumeSlider.addEventListener('change', function() {
        this.volume = volumeSlider.value;
      }.bind(this));
      volumeSlider.addEventListener('immediate-value-change', function() {
        this.volume = volumeSlider.immediateValue;
      }.bind(this));
    },

    /**
     * Invoked when the next button is clicked.
     */
    nextClick: function() {
      this.fire('next-clicked');
    },

    /**
     * Invoked when the play button is clicked.
     */
    playClick: function() {
      this.playing = !this.playing;
    },

    /**
     * Invoked when the previous button is clicked.
     */
    previousClick: function() {
      this.fire('previous-clicked');
    },

    /**
     * Invoked when the volume button is clicked.
     */
    volumeClick: function() {
      if (this.volume !== 0) {
        this.savedVolume_ = this.volume;
        this.volume = 0;
      } else {
        this.volume = this.savedVolume_ || 50;
      }
    },

    /**
     * Skips min(5 seconds, 10% of duration).
     * @param {boolean} forward Whether to skip forward/backword.
     */
    smallSkip: function(forward) {
      var millisecondsToSkip = Math.min(5000, this.duration / 10);
      if (!forward) {
        millisecondsToSkip *= -1;
      }
      this.skip_(millisecondsToSkip);
    },

    /**
     * Skips min(10 seconds, 20% of duration).
     * @param {boolean} forward Whether to skip forward/backword.
     */
    bigSkip: function(forward) {
      var millisecondsToSkip = Math.min(10000, this.duration / 5);
      if (!forward) {
        millisecondsToSkip *= -1;
      }
      this.skip_(millisecondsToSkip);
    },

    /**
     * Skips forward/backword.
     * @param {number} millis Milliseconds to skip. Set negative value to skip
     *     backword.
     * @private
     */
    skip_: function(millis) {
      if (this.duration > 0)
        this.time = Math.max(Math.min(this.time + millis, this.duration), 0);
    },

    /**
     * Converts the time into human friendly string.
     * @param {number} time Time to be converted.
     * @return {string} String representation of the given time
     */
    time2string_: function(time) {
      return ~~(time / 60000) + ':' + ('0' + ~~(time / 1000 % 60)).slice(-2);
    },

    /**
     * Converts the time and duration into human friendly string.
     * @param {number} time Time to be converted.
     * @param {number} duration Duration to be converted.
     * @return {string} String representation of the given time
     */
    computeTimeString_: function(time, duration) {
      return this.time2string_(time) + ' / ' + this.time2string_(duration);
    },

    /**
     * Computes string representation of displayed time. If a user is dragging
     * the knob of seek bar, seeking position should be shown. Otherwise,
     * playing position should be shown.
     * @param {boolean} dragging Whether the know of seek bar is being dragged.
     * @param {number} time Time corresponding to the playing position.
     * @param {number} seekingTime Time corresponding to the seeking position.
     * @param {number} duration Duration of the audio file.
     * @return {string} String representation to be displayed as current time.
     */
    computeDisplayTimeString_: function(dragging, time, seekingTime, duration) {
      if (dragging)
        return this.computeTimeString_(seekingTime, duration);
      else
        return this.computeTimeString_(time, duration);
    },

    /**
     * Invoked when the playing property is changed.
     * @param {boolean} playing
     * @private
     */
    playingChanged_: function(playing) {
      if (this.ariaLabels) {
        this.$.play.setAttribute('aria-label',
            playing ? this.ariaLabels.pause : this.ariaLabels.play);
      }
    },

    /**
     * Invoked when the volume property is changed.
     * @param {number} volume
     * @private
     */
    volumeChanged_: function(volume) {
      if (!this.$.volumeSlider.dragging)
        this.$.volumeSlider.value = volume;

      if (this.ariaLabels) {
        this.$.volumeButton.setAttribute('aria-label',
            volume !== 0 ? this.ariaLabels.mute : this.ariaLabels.unmute);
      }
    },

    /**
     * Invoked when the ariaLabels property is changed.
     * @param {Object} ariaLabels
     * @private
     */
    ariaLabelsChanged_: function(ariaLabels) {
      assert(ariaLabels);
      // TODO(fukino): Use data bindings.
      this.$.volumeSlider.setAttribute('aria-label', ariaLabels.volumeSlider);
      this.$.shuffle.setAttribute('aria-label', ariaLabels.shuffle);
      this.$.repeat.setAttribute('aria-label', ariaLabels.repeat);
      this.$.previous.setAttribute('aria-label', ariaLabels.previous);
      this.$.play.setAttribute('aria-label',
          this.playing ? ariaLabels.pause : ariaLabels.play);
      this.$.next.setAttribute('aria-label', ariaLabels.next);
      this.$.volumeButton.setAttribute('aria-label', ariaLabels.volume);
      this.$.playList.setAttribute('aria-label', ariaLabels.playList);
      this.$.timeSlider.setAttribute('aria-label', ariaLabels.seekSlider);
      this.$.volumeButton.setAttribute('aria-label',
          this.volume !== 0 ? ariaLabels.mute : ariaLabels.unmute);
      this.$.volumeSlider.setAttribute('aria-label', ariaLabels.volumeSlider);
    },
  });
})();  // Anonymous closure
