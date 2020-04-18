// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * This Polymer element shows media controls for a route that is currently cast
 * to a device.
 * @implements {RouteControlsInterface}
 */
Polymer({
  is: 'route-controls',

  properties: {
    /**
     * Set of possible options for playing fullscreen videos when mirroring.
     * @private {!Object}
     */
    FullscreenVideoOption_: {
      type: Object,
      value: {
        // Play on remote screen only.
        REMOTE_SCREEN: 'remote_screen',
        // Play on both remote and local screens.
        BOTH_SCREENS: 'both_screens'
      }
    },

    /**
     * The current time displayed in seconds, before formatting.
     * @private {number}
     */
    displayedCurrentTime_: {
      type: Number,
      value: 0,
    },

    /**
     * The volume shown in the volume control, between 0 and 1.
     * @private {number}
     */
    displayedVolume_: {
      type: Number,
      value: 0,
    },

    /**
     * True if the Hangouts route is currently using local present mode.
     * Valid for Hangouts routes only.
     * @private {boolean}
     */
    hangoutsLocalPresent_: {
      type: Boolean,
      value: false,
    },

    /**
     * The timestamp for when the initial media status was loaded.
     * @private {number}
     */
    initialLoadTime_: {
      type: Number,
      value: 0,
    },

    /**
     * Set to true when the user is dragging the seek bar. Updates for the
     * current time from the browser will be ignored when set to true.
     * @private {boolean}
     */
    isSeeking_: {
      type: Boolean,
      value: false,
    },

    /**
     * Set to true when the user is dragging the volume bar. Volume updates from
     * the browser will be ignored when set to true.
     * @private
     */
    isVolumeChanging_: {
      type: Boolean,
      value: false,
    },

    /**
     * The timestamp for when the controller last submitted a seek request.
     * @private
     */
    lastSeekByUser_: {
      type: Number,
      value: 0,
    },

    /**
     * The timestamp for when |routeStatus| was last updated.
     * @private
     */
    lastStatusUpdate_: {
      type: Number,
      value: 0,
    },

    /**
     * The timestamp for when the controller last submitted a volume change
     * request.
     * @private
     */
    lastVolumeChangeByUser_: {
      type: Number,
      value: 0,
    },

    /**
     * Keep in sync with media remoting individual user setting.
     * @private
     */
    mediaRemotingEnabled_: {
      type: Boolean,
      value: true,
    },

    /**
     * The route currently associated with this controller.
     * @type {?media_router.Route|undefined}
     */
    route: {
      type: Object,
      observer: 'onRouteUpdated_',
    },

    /**
     * The route description to display. Uses the media route description if
     * none is provided by the media route status object.
     * @private {string}
     */
    routeDescription_: {
      type: String,
      value: '',
    },

    /**
     * The timestamp for when the route details view was opened.
     * @type {number}
     */
    routeDetailsOpenTime: {
      type: Number,
      value: 0,
    },

    /**
     * The status of the media route shown.
     * @type {!media_router.RouteStatus}
     */
    routeStatus: {
      type: Object,
      observer: 'onRouteStatusChange_',
      value: new media_router.RouteStatus(),
    },

    /**
     * The ID of the timer currently set to increment the current time of the
     * media, or 0 if the current time is not being incremented.
     * @private {number}
     */
    timeIncrementsTimeoutId_: {
      type: Number,
      value: 0,
    },

    /**
     * Whether the controls should show the media title.
     * @private {boolean}
     */
    shouldShowRouteStatusTitle_: {
      type: Boolean,
      value: false,
    },
  },

  behaviors: [
    I18nBehavior,
  ],

  /**
   * Called by Polymer when the element loads. Registers the element to be
   * notified of route status updates.
   */
  ready: function() {
    media_router.ui.setRouteControls(
        /** @type {RouteControlsInterface} */ (this));
  },

  /**
   * Current time can be incremented if the media is playing, and either the
   * duration is 0 or current time is less than the duration.
   * @return {boolean}
   * @private
   */
  canIncrementCurrentTime_: function() {
    return !this.isSeeking_ &&
        this.routeStatus.playState === media_router.PlayState.PLAYING &&
        (this.routeStatus.duration === 0 ||
         this.displayedCurrentTime_ < this.routeStatus.duration);
  },

  /**
   * Creates an accessibility label for the element showing the media's current
   * time.
   * @param {number} displayedCurrentTime
   * @return {string}
   * @private
   */
  getCurrentTimeLabel_: function(displayedCurrentTime) {
    return `${
              this.i18n('currentTimeLabel')
            } ${this.getFormattedTime_(displayedCurrentTime)}`;
  },

  /**
   * Creates an accessibility label for the element showing the media's
   * duration.
   * @param {number} duration
   * @return {string}
   * @private
   */
  getDurationLabel_: function(duration) {
    return `${this.i18n('durationLabel')} ${this.getFormattedTime_(duration)}`;
  },

  /**
   * Converts a number representing an interval of seconds to a string with
   * HH:MM:SS format.
   * @param {number} timeInSec Must be non-negative. Intervals longer than 100
   *     hours get truncated silently.
   * @return {string}
   * @private
   */
  getFormattedTime_: function(timeInSec) {
    if (timeInSec < 0) {
      return '';
    }
    var hours = Math.floor(timeInSec / 3600);
    var minutes = Math.floor(timeInSec / 60) % 60;
    var seconds = Math.floor(timeInSec) % 60;
    // Show the hours only if it is nonzero.
    return (hours ? ('0' + hours).substr(-2) + ':' : '') +
        ('0' + minutes).substr(-2) + ':' + ('0' + seconds).substr(-2);
  },

  /**
   * @param {!media_router.RouteStatus} routeStatus
   * @return {string} The value for the icon attribute of the mute/unmute
   *     button.
   * @private
   */
  getMuteUnmuteIcon_: function(routeStatus) {
    return routeStatus.isMuted ? 'av:volume-off' : 'av:volume-up';
  },

  /**
   * @param {!media_router.RouteStatus} routeStatus
   * @return {string} Localized title for the mute/unmute button.
   * @private
   */
  getMuteUnmuteTitle_: function(routeStatus) {
    return routeStatus.isMuted ? this.i18n('unmuteTitle') :
                                 this.i18n('muteTitle');
  },

  /**
   * @param {!media_router.RouteStatus} routeStatus
   * @return {string}The value for the icon attribute of the play/pause button.
   * @private
   */
  getPlayPauseIcon_: function(routeStatus) {
    return routeStatus.playState === media_router.PlayState.PAUSED ?
        'av:play-arrow' :
        'av:pause';
  },

  /**
   * @param {!media_router.RouteStatus} routeStatus
   * @return {string} Localized title for the play/pause button.
   * @private
   */
  getPlayPauseTitle_: function(routeStatus) {
    return routeStatus.playState === media_router.PlayState.PAUSED ?
        this.i18n('playTitle') :
        this.i18n('pauseTitle');
  },

  /**
   * @return {string} Text representing the current position on the seek slider.
   * @private
   */
  getTimeSliderValueText_: function(displayedCurrentTime) {
    if (!this.routeStatus) {
      return '';
    }
    return `${
              this.getFormattedTime_(displayedCurrentTime)
            } / ${this.getFormattedTime_(this.routeStatus.duration)}`;
  },

  /**
   * @param {number} volume
   * @return {string} The volume as a percentage.
   * @private
   */
  getVolumeSliderValueText_: function(volume) {
    return String(Math.round(volume * 100)) + '%';
  },

  /**
   * Checks whether the media is still playing, and if so, sends a media status
   * update incrementing the current time and schedules another call for a
   * second later.
   * @private
   */
  maybeIncrementCurrentTime_: function() {
    if (this.canIncrementCurrentTime_()) {
      var updatedCurrentTime = this.routeStatus.currentTime +
          Math.floor((Date.now() - this.lastStatusUpdate_) / 1000);
      this.displayedCurrentTime_ = this.routeStatus.duration === 0 ?
          updatedCurrentTime :
          Math.min(updatedCurrentTime, this.routeStatus.duration);
      if (this.routeStatus.duration === 0 ||
          this.displayedCurrentTime_ < this.routeStatus.duration) {
        this.timeIncrementsTimeoutId_ =
            setTimeout(() => this.maybeIncrementCurrentTime_(), 1000);
      }
    } else {
      this.timeIncrementsTimeoutId_ = 0;
    }
  },

  /**
   * Called when the "smooth motion" box for Hangouts is changed by the user.
   * @param {!{target: !HTMLElement}} e
   * @private
   */
  onHangoutsLocalPresentChange_: function(e) {
    media_router.browserApi.setHangoutsLocalPresent(e.target.checked);
  },

  /**
   * Called when the user toggles the mute status of the media. Sends a mute or
   * unmute command to the browser.
   * @private
   */
  onMuteUnmute_: function() {
    media_router.browserApi.setCurrentMediaMute(!this.routeStatus.isMuted);
  },

  /**
   * Called when the user toggles between playing and pausing the media. Sends a
   * play or pause command to the browser.
   * @private
   */
  onPlayPause_: function() {
    if (this.routeStatus.playState === media_router.PlayState.PAUSED) {
      media_router.browserApi.playCurrentMedia();
    } else {
      media_router.browserApi.pauseCurrentMedia();
    }
  },

  /**
   * Updates seek and volume bars if the user is not currently dragging on
   * them.
   * @param {!media_router.RouteStatus} newRouteStatus
   * @private
   */
  onRouteStatusChange_: function(newRouteStatus) {
    this.lastStatusUpdate_ = Date.now();
    if (this.shouldAcceptCurrentTimeUpdates_()) {
      this.displayedCurrentTime_ = newRouteStatus.currentTime;
    }
    if (this.shouldAcceptVolumeUpdates_()) {
      this.displayedVolume_ = Math.round(newRouteStatus.volume * 100) / 100;
    }
    if (!this.initialLoadTime_) {
      this.initialLoadTime_ = Date.now();
      media_router.browserApi.reportWebUIRouteControllerLoaded(
          this.initialLoadTime_ - this.routeDetailsOpenTime);
    }
    this.stopIncrementingCurrentTime_();
    if (this.canIncrementCurrentTime_()) {
      this.timeIncrementsTimeoutId_ =
          setTimeout(() => this.maybeIncrementCurrentTime_(), 1000);
    }
    this.hangoutsLocalPresent_ = !!newRouteStatus.hangoutsExtraData &&
        newRouteStatus.hangoutsExtraData.localPresent;
    if (newRouteStatus.mirroringExtraData) {
      // Manually update the selected value on the
      // mirroring-fullscreen-video-dropdown dropbox.
      // TODO(imcheng): Avoid doing this by wrapping the dropbox in a Polymer
      // template, or introduce <paper-dropdown-menu> to the Polymer library.
      this.$['mirroring-fullscreen-video-dropdown'].value =
          newRouteStatus.mirroringExtraData.mediaRemotingEnabled ?
          this.FullscreenVideoOption_.REMOTE_SCREEN :
          this.FullscreenVideoOption_.BOTH_SCREENS;
    }
    this.shouldShowRouteStatusTitle_ = !!newRouteStatus.title &&
        newRouteStatus.title != '' &&
        newRouteStatus.title != this.routeDescription_;
  },

  /**
   * Called when the route is updated. Updates the description shown if it has
   * not been provided by status updates.
   * @param {?media_router.Route} route
   * @private
   */
  onRouteUpdated_: function(route) {
    if (!route) {
      this.stopIncrementingCurrentTime_();
    }
    if (route) {
      this.routeDescription_ = route.description;
    }
  },

  /**
   * Called when the user clicks on or stops dragging the seek bar.
   * @param {!Event} e
   * @private
   */
  onSeekComplete_: function(e) {
    this.stopIncrementingCurrentTime_();
    this.displayedCurrentTime_ = e.target.value;
    media_router.browserApi.seekCurrentMedia(this.displayedCurrentTime_);
    this.isSeeking_ = false;
    this.lastSeekByUser_ = Date.now();
  },

  /**
   * Called while the user is dragging the seek bar.
   * @param {!Event} e
   * @private
   */
  onSeekByDragging_: function(e) {
    this.isSeeking_ = true;
    var target = /** @type {{immediateValue: number}} */ (e.target);
    this.displayedCurrentTime_ = target.immediateValue;
  },

  /**
   * Called when the user clicks on or stops dragging the volume bar.
   * @param {!Event} e
   * @private
   */
  onVolumeChangeComplete_: function(e) {
    this.displayedVolume_ = e.target.value;
    media_router.browserApi.setCurrentMediaVolume(this.displayedVolume_);
    this.isVolumeChanging_ = false;
    this.lastVolumeChangeByUser_ = Date.now();
  },

  /**
   * Called while the user is dragging the volume bar.
   * @param {!Event} e
   * @private
   */
  onVolumeChangeByDragging_: function(e) {
    /** @const */ var currentTime = Date.now();
    // We limit the frequency of volume change requests during dragging to
    // limit the number of Mojo calls to the component extension.
    if (currentTime - this.lastVolumeChangeByUser_ < 300) {
      return;
    }
    this.lastVolumeChangeByUser_ = currentTime;
    this.isVolumeChanging_ = true;
    var target = /** @type {{immediateValue: number}} */ (e.target);
    this.displayedVolume_ = target.immediateValue;
    media_router.browserApi.setCurrentMediaVolume(this.displayedVolume_);
  },

  /**
   * Called when the value on the mirroring-fullscreen-video-dropdown dropdown
   * menu changes.
   * @param {!Event} e
   * @private
   */
  onFullscreenVideoDropdownChange_: function(e) {
    /** @const */ var dropdownValue =
        this.$['mirroring-fullscreen-video-dropdown'].value;
    media_router.browserApi.setMediaRemotingEnabled(
        dropdownValue == this.FullscreenVideoOption_.REMOTE_SCREEN);
  },

  /**
   * Resets the route controls. Called when the route details view is closed.
   */
  reset: function() {
    this.routeStatus = new media_router.RouteStatus();
    media_router.ui.setRouteControls(null);
  },

  /**
   * @return {boolean} Whether external current time updates should be reflected
   *     on the seek slider.
   * @private
   */
  shouldAcceptCurrentTimeUpdates_: function() {
    // Ignore external updates immediately after internal updates, because it's
    // likely to just be internal updates coming back from the device, and could
    // make the slider knob jump around.
    return !this.isSeeking_ && Date.now() - this.lastSeekByUser_ > 1000;
  },

  /**
   * @return {boolean} Whether external volume updates should be reflected on
   *     the volume slider.
   * @private
   */
  shouldAcceptVolumeUpdates_: function() {
    // Ignore external updates immediately after internal updates, because it's
    // likely to just be internal updates coming back from the device, and could
    // make the slider knob jump around.
    return !this.isVolumeChanging_ &&
        Date.now() - this.lastVolumeChangeByUser_ > 1000;
  },

  /**
   * If it is currently incrementing the current time shown, then stops doing
   * so.
   * @private
   */
  stopIncrementingCurrentTime_: function() {
    if (this.timeIncrementsTimeoutId_) {
      clearTimeout(this.timeIncrementsTimeoutId_);
      this.timeIncrementsTimeoutId_ = 0;
    }
  }
});
