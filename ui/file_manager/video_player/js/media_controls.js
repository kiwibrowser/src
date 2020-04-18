// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * @fileoverview MediaControls class implements media playback controls
 * that exist outside of the audio/video HTML element.
 */

/**
 * Model of a volume slider and a mute switch and its user interaction.
 * @constructor
 * @struct
 */
function VolumeModel() {
  /**
   * @type {boolean}
   */
  this.isMuted_ = false;

  /**
   * The volume level in [0..1].
   * @type {number}
   */
  this.volume_ = 0.5;
}

/**
 * After unmuting, the volume should be non-zero value to avoid that the mute
 * button gives no response to user.
 */
VolumeModel.MIN_VOLUME_AFTER_UNMUTE = 0.01;

/**
 * @return {number} the value to be set as the volume level of a media element.
 */
VolumeModel.prototype.getMediaVolume = function() {
  return this.isMuted_ ? 0 : this.volume_;
};

/**
 * Handles operation to the volume level slider.
 * @param {number} value new position of the slider in [0..1].
 */
VolumeModel.prototype.onVolumeChanged = function(value) {
  if (value == 0) {
    this.isMuted_ = true;
  } else {
    this.isMuted_ = false;
    this.volume_ = value;
  }
};

/**
 * Toggles the mute state.
 */
VolumeModel.prototype.toggleMute = function() {
  this.isMuted_ = !this.isMuted_;
  if (!this.isMuted_) {
    this.volume_ = Math.max(VolumeModel.MIN_VOLUME_AFTER_UNMUTE, this.volume_);
  }
};

/**
 * Sets the status of the model.
 * @param {number} volume the volume level in [0..1].
 * @param {boolean} mute whether to mute the sound.
 */
VolumeModel.prototype.set = function(volume, mute) {
  this.volume_ = volume;
  this.isMuted_ = mute;
};

/**
 * @param {!HTMLElement} containerElement The container for the controls.
 * @param {function(Event)} onMediaError Function to display an error message.
 * @constructor
 * @struct
 */
function MediaControls(containerElement, onMediaError) {
  this.container_ = containerElement;
  this.document_ = this.container_.ownerDocument;
  this.media_ = null;

  this.onMediaPlayBound_ = this.onMediaPlay_.bind(this, true);
  this.onMediaPauseBound_ = this.onMediaPlay_.bind(this, false);
  this.onMediaDurationBound_ = this.onMediaDuration_.bind(this);
  this.onMediaProgressBound_ = this.onMediaProgress_.bind(this);
  this.onMediaError_ = onMediaError || function() {};

  /**
   * @type {VolumeModel}
   * @private
   */
  this.volumeModel_ = new VolumeModel();

  /**
   * @type {HTMLElement}
   * @private
   */
  this.playButton_ = null;

  /**
   * @type {PaperSliderElement}
   * @private
   */
  this.progressSlider_ = null;

  /**
   * @type {PaperSliderElement}
   * @private
   */
  this.volume_ = null;

  /**
   * @type {HTMLElement}
   * @private
   */
  this.textBanner_ = null;

  /**
   * @type {HTMLElement}
   * @private
   */
  this.soundButton_ = null;

  /**
   * @type {HTMLElement}
   * @private
   */
  this.subtitlesButton_ = null;

  /**
   * @private {TextTrack}
   */
  this.subtitlesTrack_ = null;

  /**
   * @type {boolean}
   * @private
   */
  this.resumeAfterDrag_ = false;

  /**
   * @type {HTMLElement}
   * @private
   */
  this.currentTime_ = null;

  /**
   * @type {HTMLElement}
   * @private
   */
  this.currentTimeSpacer_ = null;

  /**
   * @private {boolean}
   */
  this.seeking_ = false;

  /**
   * @private {boolean}
   */
  this.showRemainingTime_ = false;
}

/**
 * Button's state types. Values are used as CSS class names.
 * @enum {string}
 */
MediaControls.ButtonStateType = {
  DEFAULT: 'default',
  PLAYING: 'playing',
  ENDED: 'ended'
};

/**
 * @return {HTMLAudioElement|HTMLVideoElement} The media element.
 */
MediaControls.prototype.getMedia = function() {
  return this.media_;
};

/**
 * Format the time in hh:mm:ss format (omitting redundant leading zeros)
 * adding '-' sign if given value is negative.
 * @param {number} timeInSec Time in seconds.
 * @return {string} Formatted time string.
 * @private
 */
MediaControls.formatTime_ = function(timeInSec) {
  var result = '';
  if (timeInSec < 0) {
    timeInSec *= -1;
    result += '-';
  }
  var seconds = Math.floor(timeInSec % 60);
  var minutes = Math.floor((timeInSec / 60) % 60);
  var hours = Math.floor(timeInSec / 60 / 60);
  if (hours) result += hours + ':';
  if (hours && (minutes < 10)) result += '0';
  result += minutes + ':';
  if (seconds < 10) result += '0';
  result += seconds;
  return result;
};

/**
 * Create a custom control.
 *
 * @param {string} className Class name.
 * @param {HTMLElement=} opt_parent Parent element or container if undefined.
 * @param {string=} opt_tagName Tag name of the control. 'div' if undefined.
 * @return {!HTMLElement} The new control element.
 */
MediaControls.prototype.createControl =
    function(className, opt_parent, opt_tagName) {
  var parent = opt_parent || this.container_;
  var control = /** @type {!HTMLElement} */
      (this.document_.createElement(opt_tagName || 'div'));
  control.className = className;
  parent.appendChild(control);
  return control;
};

/**
 * Create a custom button.
 *
 * @param {string} className Class name.
 * @param {function(Event)=} opt_handler Click handler.
 * @param {HTMLElement=} opt_parent Parent element or container if undefined.
 * @param {number=} opt_numStates Number of states, default: 1.
 * @return {!HTMLElement} The new button element.
 */
MediaControls.prototype.createButton = function(
    className, opt_handler, opt_parent, opt_numStates) {
  opt_numStates = opt_numStates || 1;

  var button = this.createControl(className, opt_parent, 'files-icon-button');
  button.classList.add('media-button');

  button.setAttribute('state', MediaControls.ButtonStateType.DEFAULT);

  if (opt_handler)
    button.addEventListener('click', opt_handler);

  return button;
};

/**
 * Enable/disable controls.
 *
 * @param {boolean} on True if enable, false if disable.
 * @private
 */
MediaControls.prototype.enableControls_ = function(on) {
  var controls = this.container_.querySelectorAll('.media-control');
  for (var i = 0; i != controls.length; i++) {
    var classList = controls[i].classList;
    if (on)
      classList.remove('disabled');
    else
      classList.add('disabled');
  }
  this.progressSlider_.disabled = !on;
  this.volume_.disabled = !on;
};

/*
 * Playback control.
 */

/**
 * Play the media.
 */
MediaControls.prototype.play = function() {
  if (!this.media_)
    return;  // Media is detached.

  this.media_.play();
};

/**
 * Pause the media.
 */
MediaControls.prototype.pause = function() {
  if (!this.media_)
    return;  // Media is detached.

  this.media_.pause();
};

/**
 * @return {boolean} True if the media is currently playing.
 */
MediaControls.prototype.isPlaying = function() {
  return !!this.media_ && !this.media_.paused && !this.media_.ended;
};

/**
 * Toggle play/pause.
 */
MediaControls.prototype.togglePlayState = function() {
  if (this.isPlaying())
    this.pause();
  else
    this.play();
};

/**
 * Toggles play/pause state on a mouse click on the play/pause button.
 *
 * @param {Event} event Mouse click event.
 */
MediaControls.prototype.onPlayButtonClicked = function(event) {
  this.togglePlayState();
};

/**
 * @param {HTMLElement=} opt_parent Parent container.
 */
MediaControls.prototype.initPlayButton = function(opt_parent) {
  this.playButton_ = this.createButton('play media-control',
      this.onPlayButtonClicked.bind(this), opt_parent, 3 /* States. */);
  this.playButton_.setAttribute('aria-label',
      str('MEDIA_PLAYER_PLAY_BUTTON_LABEL'));
};

/*
 * Time controls
 */

/**
 * The default range of 100 is too coarse for the media progress slider.
 */
MediaControls.PROGRESS_RANGE = 5000;

/**
 * 5 seconds should be skipped when left/right key is pressed.
 */
MediaControls.PROGRESS_MAX_SECONDS_TO_SMALL_SKIP = 5;

/**
 * 10 seconds should be skipped when J/L key is pressed.
 */
MediaControls.PROGRESS_MAX_SECONDS_TO_BIG_SKIP = 10;

/**
 * 10% of duration should be skipped when the video is too short to skip 5
 * seconds.
 */
MediaControls.PROGRESS_MAX_RATIO_TO_SMALL_SKIP = 0.1;

/**
 * 20% of duration should be skipped when the video is too short to skip 10
 * seconds.
 */
MediaControls.PROGRESS_MAX_RATIO_TO_BIG_SKIP = 0.2;

/**
 * @param {HTMLElement=} opt_parent Parent container.
 */
MediaControls.prototype.initTimeControls = function(opt_parent) {
  var timeControls = this.createControl('time-controls', opt_parent);

  var timeBox = this.createControl('time media-control', timeControls);

  this.currentTimeSpacer_ = this.createControl('spacer', timeBox);
  this.currentTime_ = this.createControl('current', timeBox);
  this.currentTime_.addEventListener('click',
      this.onTimeLabelClick_.bind(this));
  // Set the initial width to the minimum to reduce the flicker.
  this.updateTimeLabel_(0, 0);

  this.progressSlider_ = /** @type {!PaperSliderElement} */ (
      document.createElement('paper-slider'));
  this.progressSlider_.classList.add('progress', 'media-control');
  this.progressSlider_.max = MediaControls.PROGRESS_RANGE;
  this.progressSlider_.setAttribute('aria-label',
      str('MEDIA_PLAYER_SEEK_SLIDER_LABEL'));
  this.progressSlider_.addEventListener('change', function(event) {
    this.onProgressChange_(this.progressSlider_.ratio / 100);
  }.bind(this));
  this.progressSlider_.addEventListener(
      'immediate-value-change',
      function(event) {
        this.onProgressDrag_();
      }.bind(this));
  timeControls.appendChild(this.progressSlider_);
};

/**
 * @param {number} current Current time is seconds.
 * @param {number} duration Duration in seconds.
 * @private
 */
MediaControls.prototype.displayProgress_ = function(current, duration) {
  var ratio = current / duration;
  this.progressSlider_.value = ratio * this.progressSlider_.max;
  this.updateTimeLabel_(current);
};

/**
 * @param {number} value Progress [0..1].
 * @private
 */
MediaControls.prototype.onProgressChange_ = function(value) {
  if (!this.media_)
    return;  // Media is detached.

  if (!this.media_.seekable || !this.media_.duration) {
    console.error('Inconsistent media state');
    return;
  }

  this.setSeeking_(false);

  // Re-start playing the video when the seek bar is moved from ending point.
  if (this.media_.ended)
    this.play();

  var current = this.media_.duration * value;
  this.media_.currentTime = current;
  this.updateTimeLabel_(current);
};

/**
 * @private
 */
MediaControls.prototype.onProgressDrag_ = function() {
  if (!this.media_)
    return;  // Media is detached.

  this.setSeeking_(true);

  // Show seeking position instead of playing position while dragging.
  if (this.media_.duration && this.progressSlider_.max > 0) {
    var immediateRatio =
        this.progressSlider_.immediateValue / this.progressSlider_.max;
    var current = this.media_.duration * immediateRatio;
    this.updateTimeLabel_(current);
  }
};

/**
 * Skips forward/backword.
 * @param {number} sec Seconds to skip. Set negative value to skip backword.
 * @private
 */
MediaControls.prototype.skip_ = function(sec) {
  if (this.media_ && this.media_.duration > 0) {
    var stepsToSkip = MediaControls.PROGRESS_RANGE *
        (sec / this.media_.duration);
    this.progressSlider_.value = Math.max(Math.min(
        this.progressSlider_.value + stepsToSkip,
        this.progressSlider_.max), 0);
    this.onProgressChange_(this.progressSlider_.ratio / 100);
  }
};

/**
 * Invokes small skip.
 * @param {boolean} forward Whether to skip forward or backword.
 */
MediaControls.prototype.smallSkip = function(forward) {
  var secondsToSkip = Math.min(
      MediaControls.PROGRESS_MAX_SECONDS_TO_SMALL_SKIP,
      this.media_.duration * MediaControls.PROGRESS_MAX_RATIO_TO_SMALL_SKIP);
  if (!forward)
    secondsToSkip *= -1;
  this.skip_(secondsToSkip);
};

/**
 * Invokes big skip.
 * @param {boolean} forward Whether to skip forward or backword.
 */
MediaControls.prototype.bigSkip = function(forward) {
  var secondsToSkip = Math.min(
      MediaControls.PROGRESS_MAX_SECONDS_TO_BIG_SKIP,
      this.media_.duration * MediaControls.PROGRESS_MAX_RATIO_TO_BIG_SKIP);
  if (!forward)
    secondsToSkip *= -1;
  this.skip_(secondsToSkip);
};

/**
 * Handles 'seeking' state, which starts by dragging slider knob and finishes by
 * releasing it. While seeking, we pause the video when seeking starts and
 * resume the last play state when seeking ends.
 * @private
 */
MediaControls.prototype.setSeeking_ = function(seeking) {
  if (seeking === this.seeking_)
    return;

  this.seeking_ = seeking;

  if (seeking) {
    this.resumeAfterDrag_ = this.isPlaying();
    this.media_.pause(true /* seeking */);
  } else {
    if (this.resumeAfterDrag_) {
      if (this.media_.ended)
        this.onMediaPlay_(false);
      else
        this.media_.play(true /* seeking */);
    }
    this.resumeAfterDrag_ = false;
  }
  this.updatePlayButtonState_(this.isPlaying());
};

/**
 * Click handler for the time label.
 * @private
 */
MediaControls.prototype.onTimeLabelClick_ = function(event) {
  this.showRemainingTime_ = !this.showRemainingTime_;
  this.updateTimeLabel_(this.media_.currentTime, this.media_.duration);
};

/**
 * Update the label for current playing position and video duration.
 * The label should be like "0:06 / 0:32" or "-0:26 / 0:32".
 * @param {number} current Current playing position.
 * @param {number=} opt_duration Video's duration.
 * @private
 */
MediaControls.prototype.updateTimeLabel_ = function(current, opt_duration) {
  var duration = opt_duration;
  if (duration === undefined)
    duration = this.media_ ? this.media_.duration : 0;
  // media's duration and currentTime can be NaN. Default to 0.
  if (isNaN(duration))
    duration = 0;
  if (isNaN(current))
    current = 0;

  if (isFinite(duration)) {
    this.currentTime_.textContent =
        (this.showRemainingTime_ ? MediaControls.formatTime_(current - duration)
          : MediaControls.formatTime_(current)) + ' / ' +
          MediaControls.formatTime_(duration);
    // Keep the maximum space to prevent time label from moving while playing.
    this.currentTimeSpacer_.textContent =
        (this.showRemainingTime_ ? '-' : '') +
        MediaControls.formatTime_(duration) + ' / ' +
        MediaControls.formatTime_(duration);
  } else {
    // Media's duration can be positive infinity value when the media source is
    // not known to be bounded yet. In such cases, we should hide duration.
    this.currentTime_.textContent = MediaControls.formatTime_(current);
    this.currentTimeSpacer_.textContent = MediaControls.formatTime_(current);
  }
};

/*
 * Volume controls
 */

MediaControls.STORAGE_PREFIX = 'videoplayer-';

MediaControls.KEY_NORMALIZED_VOLUME =
    MediaControls.STORAGE_PREFIX + 'normalized-volume';
MediaControls.KEY_MUTED =
    MediaControls.STORAGE_PREFIX + 'muted';

/**
 * @param {HTMLElement=} opt_parent Parent element for the controls.
 */
MediaControls.prototype.initVolumeControls = function(opt_parent) {
  var volumeControls = this.createControl('volume-controls', opt_parent);

  this.soundButton_ = this.createButton('sound media-control',
      this.onSoundButtonClick_.bind(this), volumeControls);
  this.soundButton_.setAttribute('level', 3);  // max level.
  this.soundButton_.setAttribute('aria-label',
      str('MEDIA_PLAYER_MUTE_BUTTON_LABEL'));

  this.volume_ = /** @type {!PaperSliderElement} */ (
      document.createElement('paper-slider'));
  this.volume_.classList.add('volume', 'media-control');
  this.volume_.setAttribute('aria-label',
      str('MEDIA_PLAYER_VOLUME_SLIDER_LABEL'));
  this.volume_.addEventListener('change', function(event) {
    this.onVolumeChange_(this.volume_.ratio / 100);
  }.bind(this));
  this.volume_.addEventListener('immediate-value-change', function(event) {
    this.onVolumeDrag_();
  }.bind(this));
  this.loadVolumeControlState();
  volumeControls.appendChild(this.volume_);
};

MediaControls.prototype.loadVolumeControlState = function() {
  chrome.storage.local.get([MediaControls.KEY_NORMALIZED_VOLUME,
                            MediaControls.KEY_MUTED],
      function(retrieved) {
        var normalizedVolume = (MediaControls.KEY_NORMALIZED_VOLUME
                                 in retrieved)
            ? retrieved[MediaControls.KEY_NORMALIZED_VOLUME] : 1;
        var isMuted = (MediaControls.KEY_MUTED in retrieved)
            ? retrieved[MediaControls.KEY_MUTED] : false;
        this.volumeModel_.set(normalizedVolume, isMuted);
        this.reflectVolumeToUi_();
      }.bind(this));
};

MediaControls.prototype.saveVolumeControlState = function() {
  var valuesToStore = {};
  valuesToStore[MediaControls.KEY_NORMALIZED_VOLUME] =
      this.volumeModel_.volume_;
  valuesToStore[MediaControls.KEY_MUTED] = this.volumeModel_.isMuted_;
  chrome.storage.local.set(valuesToStore);
};

/**
 * Click handler for the sound level button.
 * @private
 */
MediaControls.prototype.onSoundButtonClick_ = function() {
  this.volumeModel_.toggleMute();
  this.saveVolumeControlState();
  this.reflectVolumeToUi_();
};

/**
 * @param {number} value Volume [0..1].
 * @return {number} The rough level [0..3] used to pick an icon.
 * @private
 */
MediaControls.getVolumeLevel_ = function(value) {
  if (value == 0) return 0;
  if (value <= 1 / 3) return 1;
  if (value <= 2 / 3) return 2;
  return 3;
};

/**
 * Reflects volume model to the UI elements.
 * @private
 */
MediaControls.prototype.reflectVolumeToUi_ = function() {
  this.soundButton_.setAttribute('level',
      MediaControls.getVolumeLevel_(this.volumeModel_.getMediaVolume()));
  this.soundButton_.setAttribute('aria-label', this.volumeModel_.isMuted_
                                 ? str('MEDIA_PLAYER_UNMUTE_BUTTON_LABEL')
                                 : str('MEDIA_PLAYER_MUTE_BUTTON_LABEL'));
  this.volume_.value = this.volumeModel_.getMediaVolume() * this.volume_.max;
  if (this.media_) {
    this.media_.volume = this.volumeModel_.getMediaVolume();
  }
};

/**
 * Handles change event of the volume slider.
 * @param {number} value Volume [0..1].
 * @private
 */
MediaControls.prototype.onVolumeChange_ = function(value) {
  if (!this.media_)
    return;  // Media is detached.

  this.volumeModel_.onVolumeChanged(value);
  this.saveVolumeControlState();
  this.reflectVolumeToUi_();
};

/**
 * @private
 */
MediaControls.prototype.onVolumeDrag_ = function() {
  if (this.media_.volume !== 0) {
    this.volumeModel_.onVolumeChanged(this.media_.volume);
  }
};

/**
 * Initializes subtitles button.
 */
MediaControls.prototype.initSubtitlesButton = function() {
  this.subtitlesTrack_ = null;
  this.subtitlesButton_ =
      this.createButton('subtitles', this.onSubtitlesButtonClicked_.bind(this));
};

/**
 * @param {Event} event Mouse click event.
 * @private
 */
MediaControls.prototype.onSubtitlesButtonClicked_ = function(event) {
  if (!this.subtitlesTrack_) {
    return;
  }
  this.toggleSubtitlesMode_(this.subtitlesTrack_.mode === 'hidden');
};

/**
 * @param {boolean} on Whether enabled or not.
 * @private
 */
MediaControls.prototype.toggleSubtitlesMode_ = function(on) {
  if (!this.subtitlesTrack_) {
    return;
  }
  if (on) {
    this.subtitlesTrack_.mode = 'showing';
    this.subtitlesButton_.setAttribute('showing', '');
    this.subtitlesButton_.setAttribute('aria-label',
        str('VIDEO_PLAYER_DISABLE_SUBTITLES_BUTTON_LABEL'));
  } else {
    this.subtitlesTrack_.mode = 'hidden';
    this.subtitlesButton_.removeAttribute('showing');
    this.subtitlesButton_.setAttribute('aria-label',
        str('VIDEO_PLAYER_ENABLE_SUBTITLES_BUTTON_LABEL'));
  }
};

/**
 * @param {TextTrack} track Subtitles track
 * @private
 */
MediaControls.prototype.attachTextTrack_ = function(track) {
  this.subtitlesTrack_ = track;
  if (this.subtitlesTrack_) {
    this.toggleSubtitlesMode_(true);
    this.subtitlesButton_.removeAttribute('unavailable');
  } else {
    this.subtitlesButton_.setAttribute('unavailable', '');
  }
};

/**
 * @private
 */
MediaControls.prototype.detachTextTrack_ = function() {
  this.subtitlesTrack_ = null;
};

/*
 * Media event handlers.
 */

/**
 * Attach a media element.
 *
 * @param {!HTMLMediaElement} mediaElement The media element to control.
 */
MediaControls.prototype.attachMedia = function(mediaElement) {
  this.media_ = mediaElement;

  this.media_.addEventListener('play', this.onMediaPlayBound_);
  this.media_.addEventListener('pause', this.onMediaPauseBound_);
  this.media_.addEventListener('durationchange', this.onMediaDurationBound_);
  this.media_.addEventListener('timeupdate', this.onMediaProgressBound_);
  this.media_.addEventListener('error', this.onMediaError_);

  // If the text banner is being displayed, hide it immediately, since it is
  // related to the previous media.
  this.textBanner_.removeAttribute('visible');

  // Reflect the media state in the UI.
  this.onMediaDuration_();
  this.onMediaPlay_(this.isPlaying());
  this.onMediaProgress_();

  // Reflect the user specified volume to the media.
  this.media_.volume = this.volumeModel_.getMediaVolume();

  if (this.media_.textTracks && this.media_.textTracks.length > 0) {
    this.attachTextTrack_(this.media_.textTracks[0]);
  } else {
    this.attachTextTrack_(null);
  }
};

/**
 * Detach media event handlers.
 */
MediaControls.prototype.detachMedia = function() {
  if (!this.media_)
    return;

  this.media_.removeEventListener('play', this.onMediaPlayBound_);
  this.media_.removeEventListener('pause', this.onMediaPauseBound_);
  this.media_.removeEventListener('durationchange', this.onMediaDurationBound_);
  this.media_.removeEventListener('timeupdate', this.onMediaProgressBound_);
  this.media_.removeEventListener('error', this.onMediaError_);

  this.media_ = null;
  this.detachTextTrack_();
};

/**
 * Force-empty the media pipeline. This is a workaround for crbug.com/149957.
 * The document is not going to be GC-ed until the last Files app window closes,
 * but we want the media pipeline to deinitialize ASAP to minimize leakage.
 */
MediaControls.prototype.cleanup = function() {
  if (!this.media_)
    return;

  this.media_.src = '';
  this.media_.load();
  this.detachMedia();
};

/**
 * 'play' and 'pause' event handler.
 * @param {boolean} playing True if playing.
 * @private
 */
MediaControls.prototype.onMediaPlay_ = function(playing) {
  if (this.progressSlider_.dragging)
    return;

  this.updatePlayButtonState_(playing);
  this.onPlayStateChanged();
};

/**
 * 'durationchange' event handler.
 * @private
 */
MediaControls.prototype.onMediaDuration_ = function() {
  if (!this.media_ || !this.media_.duration) {
    this.enableControls_(false);
    return;
  }

  this.enableControls_(true);

  if (this.media_.seekable)
    this.progressSlider_.classList.remove('readonly');
  else
    this.progressSlider_.classList.add('readonly');

  this.updateTimeLabel_(this.media_.currentTime, this.media_.duration);

  if (this.media_.seekable)
    this.restorePlayState();
};

/**
 * 'timeupdate' event handler.
 * @private
 */
MediaControls.prototype.onMediaProgress_ = function() {
  if (!this.media_ || !this.media_.duration) {
    this.displayProgress_(0, 1);
    return;
  }

  var current = this.media_.currentTime;
  var duration = this.media_.duration;

  if (this.progressSlider_.dragging)
    return;

  this.displayProgress_(current, duration);

  if (current == duration) {
    this.onMediaComplete();
  }
  this.onPlayStateChanged();
};

/**
 * Called when the media playback is complete.
 */
MediaControls.prototype.onMediaComplete = function() {};

/**
 * Called when play/pause state is changed or on playback progress.
 * This is the right moment to save the play state.
 */
MediaControls.prototype.onPlayStateChanged = function() {};

/**
 * Updates the play button state.
 * @param {boolean} playing If the video is playing.
 * @private
 */
MediaControls.prototype.updatePlayButtonState_ = function(playing) {
  if (this.media_.ended &&
      this.progressSlider_.value === this.progressSlider_.max) {
    this.playButton_.setAttribute('state',
                                  MediaControls.ButtonStateType.ENDED);
    this.playButton_.setAttribute('aria-label',
        str('MEDIA_PLAYER_PLAY_BUTTON_LABEL'));
  } else if (playing) {
    this.playButton_.setAttribute('state',
                                  MediaControls.ButtonStateType.PLAYING);
    this.playButton_.setAttribute('aria-label',
        str('MEDIA_PLAYER_PAUSE_BUTTON_LABEL'));
  } else {
    this.playButton_.setAttribute('state',
                                  MediaControls.ButtonStateType.DEFAULT);
    this.playButton_.setAttribute('aria-label',
        str('MEDIA_PLAYER_PLAY_BUTTON_LABEL'));
  }
};

/**
 * Restore play state. Base implementation is empty.
 */
MediaControls.prototype.restorePlayState = function() {};

/**
 * Encode current state into the page URL or the app state.
 */
MediaControls.prototype.encodeState = function() {
  if (!this.media_ || !this.media_.duration)
    return;

  if (window.appState) {
    window.appState.time = this.media_.currentTime;
    util.saveAppState();
  }
  return;
};

/**
 * Decode current state from the page URL or the app state.
 * @return {boolean} True if decode succeeded.
 */
MediaControls.prototype.decodeState = function() {
  if (!this.media_ || !window.appState || !('time' in window.appState))
    return false;
  // There is no page reload for apps v2, only app restart.
  // Always restart in paused state.
  this.media_.currentTime = window.appState.time;
  this.pause();
  return true;
};

/**
 * Remove current state from the page URL or the app state.
 */
MediaControls.prototype.clearState = function() {
  if (!window.appState)
    return;

  if ('time' in window.appState)
    delete window.appState.time;
  util.saveAppState();
  return;
};

/**
 * Create video controls.
 *
 * @param {!HTMLElement} containerElement The container for the controls.
 * @param {function(Event)} onMediaError Function to display an error message.
 * @param {function(Event)=} opt_fullScreenToggle Function to toggle fullscreen
 *     mode.
 * @param {HTMLElement=} opt_stateIconParent The parent for the icon that
 *     gives visual feedback when the playback state changes.
 * @constructor
 * @struct
 * @extends {MediaControls}
 */
function VideoControls(
    containerElement, onMediaError, opt_fullScreenToggle, opt_stateIconParent) {
  MediaControls.call(this, containerElement, onMediaError);

  this.container_.classList.add('video-controls');
  this.initPlayButton();
  this.initTimeControls();
  this.initVolumeControls();
  this.initSubtitlesButton();

  // Create the cast menu button.
  // We need to use <button> since cr.ui.MenuButton.decorate modifies prototype
  // chain, by which <files-icon-button> will not work correctly.
  // TODO(fukino): Find a way to use files-icon-button consistently.
  this.castButton_ = this.createControl(
      'cast media-button', undefined, 'button');
  this.castButton_.setAttribute('menu', '#cast-menu');
  this.castButton_.setAttribute('aria-label', str('VIDEO_PLAYER_PLAY_ON'));
  this.castButton_.setAttribute('state', MediaControls.ButtonStateType.DEFAULT);
  this.castButton_.appendChild(document.createElement('files-ripple'));
  cr.ui.decorate(this.castButton_, cr.ui.MenuButton);

  // Create the cast button, which is a normal button and is used when we cast
  // videos usign Media Router.
  this.createButton('cast-button');

  if (opt_fullScreenToggle) {
    this.fullscreenButton_ =
        this.createButton('fullscreen', opt_fullScreenToggle);
    this.fullscreenButton_.setAttribute('aria-label',
        str('VIDEO_PLAYER_FULL_SCREEN_BUTTON_LABEL'));
  }

  if (opt_stateIconParent) {
    this.stateIcon_ = this.createControl(
        'playback-state-icon', opt_stateIconParent);
    this.textBanner_ = this.createControl('text-banner', opt_stateIconParent);
  }

  // Disables all controls at first.
  this.enableControls_(false);

  var videoControls = this;
  chrome.mediaPlayerPrivate.onTogglePlayState.addListener(
      function() { videoControls.togglePlayStateWithFeedback(); });
}

/**
 * No resume if we are within this margin from the start or the end.
 */
VideoControls.RESUME_MARGIN = 0.03;

/**
 * No resume for videos shorter than this.
 */
VideoControls.RESUME_THRESHOLD = 5 * 60; // 5 min.

/**
 * When resuming rewind back this much.
 */
VideoControls.RESUME_REWIND = 5;  // seconds.

VideoControls.prototype = { __proto__: MediaControls.prototype };

/**
 * Shows icon feedback for the current state of the video player.
 * @private
 */
VideoControls.prototype.showIconFeedback_ = function() {
  var stateIcon = this.stateIcon_;
  stateIcon.removeAttribute('state');

  setTimeout(function() {
    var newState = this.isPlaying() ? 'play' : 'pause';

    var onAnimationEnd = function(state, event) {
      if (stateIcon.getAttribute('state') === state)
        stateIcon.removeAttribute('state');

      stateIcon.removeEventListener('animationend', onAnimationEnd);
    }.bind(null, newState);
    stateIcon.addEventListener('animationend', onAnimationEnd);

    // Shows the icon with animation.
    stateIcon.setAttribute('state', newState);
  }.bind(this), 0);
};

/**
 * Shows a text banner.
 *
 * @param {string} identifier String identifier.
 * @private
 */
VideoControls.prototype.showTextBanner_ = function(identifier) {
  this.textBanner_.removeAttribute('visible');
  this.textBanner_.textContent = str(identifier);

  setTimeout(function() {
    var onAnimationEnd = function(event) {
      this.textBanner_.removeEventListener(
          'animationend', onAnimationEnd);
      this.textBanner_.removeAttribute('visible');
    }.bind(this);
    this.textBanner_.addEventListener('animationend', onAnimationEnd);

    this.textBanner_.setAttribute('visible', 'true');
  }.bind(this), 0);
};

/**
 * @override
 */
VideoControls.prototype.onPlayButtonClicked = function(event) {
  if (event.ctrlKey) {
    this.toggleLoopedModeWithFeedback(true);
    if (!this.isPlaying())
      this.togglePlayState();
  } else {
    this.togglePlayState();
  }
};

/**
 * Media completion handler.
 */
VideoControls.prototype.onMediaComplete = function() {
  this.onMediaPlay_(false);  // Just update the UI.
  this.savePosition();  // This will effectively forget the position.
};

/**
 * Toggles the looped mode with feedback.
 * @param {boolean} on Whether enabled or not.
 */
VideoControls.prototype.toggleLoopedModeWithFeedback = function(on) {
  if (!this.getMedia().duration)
    return;
  this.toggleLoopedMode(on);
  if (on) {
    // TODO(mtomasz): Simplify, crbug.com/254318.
    this.showTextBanner_('VIDEO_PLAYER_LOOPED_MODE');
  }
};

/**
 * Toggles the looped mode.
 * @param {boolean} on Whether enabled or not.
 */
VideoControls.prototype.toggleLoopedMode = function(on) {
  this.getMedia().loop = on;
};

/**
 * Toggles play/pause state and flash an icon over the video.
 */
VideoControls.prototype.togglePlayStateWithFeedback = function() {
  if (!this.getMedia().duration)
    return;

  this.togglePlayState();
  this.showIconFeedback_();
};

/**
 * Toggles play/pause state.
 */
VideoControls.prototype.togglePlayState = function() {
  if (this.isPlaying()) {
    // User gave the Pause command. Save the state and reset the loop mode.
    this.toggleLoopedMode(false);
    this.savePosition();
  }
  MediaControls.prototype.togglePlayState.apply(this, arguments);
};

/**
 * Saves the playback position to the persistent storage.
 * @param {boolean=} opt_sync True if the position must be saved synchronously
 *     (required when closing app windows).
 */
VideoControls.prototype.savePosition = function(opt_sync) {
  if (!this.media_ ||
      !this.media_.duration ||
      this.media_.duration < VideoControls.RESUME_THRESHOLD) {
    return;
  }

  var ratio = this.media_.currentTime / this.media_.duration;
  var position;
  if (ratio < VideoControls.RESUME_MARGIN ||
      ratio > (1 - VideoControls.RESUME_MARGIN)) {
    // We are too close to the beginning or the end.
    // Remove the resume position so that next time we start from the beginning.
    position = null;
  } else {
    position = Math.floor(
        Math.max(0, this.media_.currentTime - VideoControls.RESUME_REWIND));
  }

  if (opt_sync) {
    // Packaged apps cannot save synchronously.
    // Pass the data to the background page.
    if (!window.saveOnExit)
      window.saveOnExit = [];
    window.saveOnExit.push({ key: this.media_.src, value: position });
  } else {
    util.AppCache.update(this.media_.src, position);
  }
};

/**
 * Resumes the playback position saved in the persistent storage.
 */
VideoControls.prototype.restorePlayState = function() {
  if (this.media_ && this.media_.duration >= VideoControls.RESUME_THRESHOLD) {
    util.AppCache.getValue(this.media_.src, function(position) {
      if (position)
        this.media_.currentTime = position;
    }.bind(this));
  }
};

/**
 * Updates video control when the window is fullscreened or restored.
 */
VideoControls.prototype.onFullScreenChanged = function() {
  var fullscreen = util.isFullScreen(chrome.app.window.current());
  if (fullscreen) {
    this.container_.setAttribute('fullscreen', '');
  } else {
    this.container_.removeAttribute('fullscreen');
  }

  if (this.fullscreenButton_) {
    this.fullscreenButton_.setAttribute(
        'aria-label',
        fullscreen ? str('VIDEO_PLAYER_EXIT_FULL_SCREEN_BUTTON_LABEL') :
                     str('VIDEO_PLAYER_FULL_SCREEN_BUTTON_LABEL'));
    // If the fullscreen button has focus on entering fullscreen mode, reset the
    // focus to make the spacebar toggle play/pause state. This is the
    // consistent behavior with Youtube Web UI.
    if (fullscreen)
      this.fullscreenButton_.blur();
  }
};
