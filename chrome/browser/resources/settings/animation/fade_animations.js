// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * @fileoverview Defines fade animations similar to Polymer's fade-in-animation
 * and fade-out-animation, but with Settings-specific timings.
 */
Polymer({
  is: 'settings-fade-in-animation',

  behaviors: [Polymer.NeonAnimationBehavior],

  configure: function(config) {
    const node = config.node;
    this._effect = new KeyframeEffect(
        node,
        [
          {'opacity': '0'},
          {'opacity': '1'},
        ],
        /** @type {!KeyframeEffectOptions} */ ({
          duration: settings.animation.Timing.DURATION,
          easing: settings.animation.Timing.EASING,
          fill: 'both',
        }));
    return this._effect;
  }
});

Polymer({
  is: 'settings-fade-out-animation',

  behaviors: [Polymer.NeonAnimationBehavior],

  configure: function(config) {
    const node = config.node;
    this._effect = new KeyframeEffect(
        node,
        [
          {'opacity': '1'},
          {'opacity': '0'},
        ],
        /** @type {!KeyframeEffectOptions} */ ({
          duration: settings.animation.Timing.DURATION,
          easing: settings.animation.Timing.EASING,
          fill: 'both',
        }));
    return this._effect;
  }
});
