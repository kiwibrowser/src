// Copyright 2015 The ChromeOS IME Authors. All Rights Reserved.
// limitations under the License.
// See the License for the specific language governing permissions and
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// distributed under the License is distributed on an "AS-IS" BASIS,
// Unless required by applicable law or agreed to in writing, software
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// You may obtain a copy of the License at
// you may not use this file except in compliance with the License.
// Licensed under the Apache License, Version 2.0 (the "License");
//
goog.provide('i18n.input.chrome.sounds.SoundController');

goog.require('goog.Disposable');
goog.require('goog.dom');
goog.require('i18n.input.chrome.ElementType');
goog.require('i18n.input.chrome.sounds.Sounds');

goog.scope(function() {
var Sounds = i18n.input.chrome.sounds.Sounds;
var ElementType = i18n.input.chrome.ElementType;
var keyToSoundIdOnKeyUp = {};
var keyToSoundIdOnKeyRepeat = {};



/**
 * Sound controller for the keyboard.
 *
 * @param {!boolean} enabled Whether sounds is enabled by default.
 * @param {?number=} opt_volume The default volume for sound tracks.
 * @constructor
 * @extends {goog.Disposable}
 */
i18n.input.chrome.sounds.SoundController = function(enabled, opt_volume) {

  /**
   * Collection of all the sound pools.
   *
   * @private {!Object.<string, !Object>}
   */
  this.sounds_ = {};

  /** @private {boolean} */
  this.enabled_ = enabled;

  /**
   * The default volume for all audio tracks. Tracks with volume 0 will be
   * skipped.
   *
   * @private {number}
   */
  this.volume_ = opt_volume || this.DEFAULT_VOLUME;

  if (enabled) {
    this.initialize();
  }
};
goog.inherits(i18n.input.chrome.sounds.SoundController, goog.Disposable);


var Controller = i18n.input.chrome.sounds.SoundController;


/**
 * @define {number}  The size of the pool to use for playing audio sounds.
 */
Controller.prototype.POOL_SIZE = 10;


/**
 * @define {number}  The default audio track volume.
 */
Controller.prototype.DEFAULT_VOLUME = 0.6;


/** @private {boolean} */
Controller.prototype.initialized_ = false;


/**
 * Initializes the sound controller.
 */
Controller.prototype.initialize = function() {
  if (!this.initialized_) {
    for (var sound in Sounds) {
      this.addSound_(Sounds[sound]);
    }
    keyToSoundIdOnKeyUp[ElementType.BACKSPACE_KEY] = Sounds.NONE;
    keyToSoundIdOnKeyUp[ElementType.ENTER_KEY] = Sounds.RETURN;
    keyToSoundIdOnKeyUp[ElementType.SPACE_KEY] = Sounds.SPACEBAR;
    keyToSoundIdOnKeyRepeat[ElementType.BACKSPACE_KEY] = Sounds.DELETE;
    this.initialized_ = true;
  }
};


/**
 * Caches the specified sound on the keyboard.
 *
 * @param {string} soundId The name of the .wav file in the "sounds"
     directory.
 * @private
 */
Controller.prototype.addSound_ = function(soundId) {
  if (soundId == Sounds.NONE || this.sounds_[soundId])
    return;
  var pool = [];
  // Create sound pool.
  for (var i = 0; i < this.POOL_SIZE; i++) {
    var audio = goog.dom.createDom('audio', {
      preload: 'auto',
      id: soundId,
      src: 'sounds/' + soundId + '.wav',
      volume: this.volume_
    });
    pool.push(audio);
  }
  this.sounds_[soundId] = pool;
};


/**
 * Sets the volume for the specified sound.
 *
 * @param {string} soundId The id of the sound.
 * @param {number} volume The volume to set.
 */
Controller.prototype.setVolume = function(soundId, volume) {
  var pool = this.sounds_[soundId];
  if (!pool) {
    console.error('Cannot find sound: ' + soundId);
    return;
  }
  // Change volume for all sounds in the pool.
  for (var i = 0; i < pool.length; i++) {
    pool[i].volume = volume;
  }
};


/**
 * Enables or disable playing sounds on keypress.
 * @param {!boolean} enabled
 */
Controller.prototype.setEnabled = function(enabled) {
  this.enabled_ = enabled;
  if (this.enabled_) {
    this.initialize();
  }
};


/**
 * Gets the flag whether sound controller is enabled or not.
 *
 * @return {!boolean}
 */
Controller.prototype.getEnabled = function() {
  return this.enabled_;
};


/**
 * Sets the volume for all sounds on the keyboard.
 *
 * @param {number} volume The volume of the sounds.
 */
Controller.prototype.setMasterVolume = function(volume) {
  this.volume_ = volume;
  for (var id in this.sounds_) {
    this.setVolume(id, volume);
  }
};


/**
 * Plays the specified sound.
 *
 * @param {string} soundId The id of the audio tag.
 * @param {boolean=} opt_force Force to play sound whatever the enabled flags is
 *     turned on.
 */
Controller.prototype.playSound = function(soundId, opt_force) {
  if (opt_force) {
    this.initialize();
  }
  // If master volume is zero, ignore the request.
  if (!opt_force && !this.enabled_ || this.volume_ == 0 ||
      soundId == Sounds.NONE) {
    return;
  }
  var pool = this.sounds_[soundId];
  if (!pool) {
    console.error('Cannot find sound: ' + soundId);
    return;
  }
  // Search the sound pool for a free resource.
  for (var i = 0; i < pool.length; i++) {
    if (pool[i].paused) {
      pool[i].play();
      return;
    }
  }
};


/**
 * On key up.
 *
 * @param {ElementType} key The key released.
 */
Controller.prototype.onKeyUp = function(key) {
  var sound = keyToSoundIdOnKeyUp[key] || Sounds.STANDARD;
  this.playSound(sound);
};


/**
 * On key repeat.
 *
 * @param {ElementType} key The key that is being repeated.
 */
Controller.prototype.onKeyRepeat = function(key) {
  var sound = keyToSoundIdOnKeyRepeat[key] || Sounds.NONE;
  this.playSound(sound);
};


/** @override */
Controller.prototype.disposeInternal = function() {
  for (var soundId in this.sounds_) {
    var pool = this.sounds_[soundId];
    for (var i = 0; i < pool.length; i++) {
      var tag = pool[i];
      if (tag && tag.loaded) {
        tag.pause();
        tag.autoplay = false;
        tag.loop = false;
        tag.currentTime = 0;
      }
    }
    delete this.sounds_[soundId];
  }
  this.sounds_ = {};
  keyToSoundIdOnKeyUp = {};
  keyToSoundIdOnKeyRepeat = {};
  goog.base(this, 'disposeInternal');
};

});  // goog.scope
