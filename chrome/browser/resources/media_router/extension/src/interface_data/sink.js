// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

goog.provide('mr.Sink');
goog.provide('mr.SinkIconType');


/**
 * Sink icon types defined in Chrome.
 * To define a new icon, add it to Chrome first and then here.
 * @enum {string}
 */
mr.SinkIconType = {
  CAST: 'cast',
  CAST_AUDIO_GROUP: 'cast_audio_group',
  CAST_AUDIO: 'cast_audio',
  MEETING: 'meeting',
  HANGOUT: 'hangout',
  EDUCATION: 'education',
  GENERIC: 'generic',
};



/**
 * Represents a device which can be a destination for a media route.
 */
mr.Sink = class {
  /**
   * @param {string} id The ID of the sink.
   * @param {string} friendlyName The human readable name of the sink.
   * @param {mr.SinkIconType=} iconType
   * @param {?string=} description The human readable description of
   *     the sink.  The description is displayed below the sink name,
   *     and may contain more detailed information about the sink
   *     (e.g., meeting description).
   * @param {?string=} domain The Dasher domain associated with the
   *     sink.
   */
  constructor(
      id, friendlyName, iconType = undefined, description = null,
      domain = null) {
    // For some reason the current public release of jscompiler gets upset if
    // mr.SinkIconType.GENERIC is specified as a default argument.
    iconType = iconType || mr.SinkIconType.GENERIC;

    /**
     * @type {string}
     * @export
     */
    this.id = id;

    /**
     * @type {string}
     * @export
     */
    this.friendlyName = friendlyName;

    /**
     * @type {mr.SinkIconType}
     * @export
     */
    this.iconType = iconType;

    /**
     * @type {?string}
     * @export
     */
    this.description = description;

    /**
     * A non-null domain signifies the sink is tied to a Dasher domain.
     * @type {?string}
     * @export
     */
    this.domain = domain;
  }
};
