// Copyright 2016 The ChromeOS IME Authors. All Rights Reserved.
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
goog.provide('i18n.input.chrome.FeatureTracker');

goog.require('i18n.input.chrome.FeatureName');

goog.scope(function() {
var FeatureName = i18n.input.chrome.FeatureName;



/**
 * Controller for experimental features.
 *
 * @constructor
 */
i18n.input.chrome.FeatureTracker = function() {

  /**
   * Whether experimental flags is enabled.
   *
   * @private {Object.<string, boolean>}
   */
  this.features_ = {};

  /**
   * Features that are enabled by default. Any feature not on this list is
   * disabled unless the runtime --enable-inputview-{feature} flag is present.
   *
   * @const
   * @private {!Array<FeatureName>}
   */
  this.ENABLED_BY_DEFAULT_ = [];

  /**
   * Whether the features list is ready.
   *
   * @private {boolean}
   */
  this.ready_ = false;
};

var FeatureTracker = i18n.input.chrome.FeatureTracker;


/**
 * Whether the feature is enabled.
 *
 * @param {!FeatureName} feature .
 * @return {boolean}
 */
FeatureTracker.prototype.isEnabled = function(feature) {
  if (!this.ready_) {
    console.error('Features not present in config or not ready yet.');
  }
  if (feature in this.features_) {
    return this.features_[feature];
  }
  return (this.ENABLED_BY_DEFAULT_.indexOf(feature) >= 0) ||
      !!this.features_[FeatureName.EXPERIMENTAL];
};


/**
 * Inits the feature tracker.
 *
 * @param {!Object} config The keyboard config.
 */
FeatureTracker.prototype.initialize = function(config) {
  if (config.features) {
    var features = config.features;
    // Parse the run time flags.
    for (var i = 0; i < features.length; i++) {
      var pieces = features[i].split('-');
      var state = pieces.pop();
      if (state == 'enabled') {
        this.features_[pieces.join('-')] = true;
      } else if (state == 'disabled') {
        this.features_[pieces.join('-')] = false;
      } else {
        console.error('Unrecognized flag: ' + features[i]);
      }
    }
    this.ready_ = true;
  } else {
    console.error('API Error. Features not present in config.');
    return;
  }
};

});  // goog.scope

