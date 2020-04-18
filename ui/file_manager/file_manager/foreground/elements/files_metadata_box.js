// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

var FilesMetadataBox = Polymer({
  is: 'files-metadata-box',

  properties: {
    // File media type, e.g. image, video.
    type: String,
    size: String,
    modificationTime: String,
    filePath: String,
    mediaMimeType: String,

    // File type specific metadata below.
    imageWidth: {
      type: Number,
      observer: 'metadataUpdated_',
    },
    imageHeight: {
      type: Number,
      observer: 'metadataUpdated_',
    },
    mediaAlbum: {
      type: String,
      observer: 'metadataUpdated_',
    },
    mediaArtist: {
      type: String,
      observer: 'metadataUpdated_',
    },
    mediaDuration: {
      type: Number,
      observer: 'metadataUpdated_',
    },
    mediaGenre: {
      type: String,
      observer: 'metadataUpdated_',
    },
    mediaTitle: {
      type: String,
      observer: 'metadataUpdated_',
    },
    mediaTrack: {
      type: String,
      observer: 'metadataUpdated_',
    },
    mediaYearRecorded: {
      type: String,
      observer: 'metadataUpdated_',
    },
    /**
     * Exif information parsed by exif_parser.js or null if there is no
     * information.
     * @type {?Object}
     */
    ifd: {
      type: Object,
      observer: 'metadataUpdated_',
    },

    // Whether the size is the middle of loading.
    isSizeLoading: Boolean,

    /**
     * @private
     */
    hasFileSpecificInfo_: Boolean,
  },

  /**
   * Clears fields.
   * @param {boolean} keepSizeFields do not clear size and isSizeLoading fields.
   */
  clear: function(keepSizeFields) {
    this.type = '';
    this.modificationTime = '';
    this.mediaMimeType = '';
    this.filePath = '';

    this.imageWidth = 0;
    this.imageHeight = 0;
    this.mediaTitle = '';
    this.mediaArtist = '';
    this.mediaAlbum = '';
    this.mediaDuration = 0;
    this.mediaGenre = '';
    this.mediaTrack = '';
    this.mediaYearRecorded = '';
    this.ifd = null;

    if (!keepSizeFields) {
      this.size = '';
      this.isSizeLoading = false;
    }
  },

  /**
   * @param {string} type
   * @return {boolean}
   *
   * @private
   */
  isImage_: function(type) {
    return type === 'image';
  },

  /**
   * @param {string} type
   * @return {boolean}
   *
   * @private
   */
  isVideo_: function(type) {
    return type === 'video';
  },

  /**
   * @param {string} type
   * @return {boolean}
   *
   * @private
   */
  isAudio_: function(type) {
    return type === 'audio';
  },

  /**
   * Update private properties computed from metadata.
   * @private
   */
  metadataUpdated_: function() {
    this.hasFileSpecificInfo_ =
        !!(this.imageWidth && this.imageHeight || this.mediaTitle ||
           this.mediaArtist || this.mediaAlbum || this.mediaDuration ||
           this.mediaGenre || this.mediaTrack || this.mediaYearRecorded ||
           this.ifd);
  },

  /**
   * Converts the duration into human friendly string.
   * @param {number} time the duration in seconds.
   * @return {string} String representation of the given duration.
   */
  time2string_: function(time) {
    if (!time)
      return '';

    time = parseInt(time, 10);
    var seconds = time % 60;
    var minutes = Math.floor(time / 60) % 60;
    var hours = Math.floor(time / 60 / 60);

    if (hours === 0) {
      return minutes + ':' + ('0' + seconds).slice(-2);
    }
    return hours + ':' + ('0' + minutes).slice(-2) + ('0' + seconds).slice(-2);
  },

  /**
   * @param {number} imageWidth
   * @param {number} imageHeight
   * @return {string}
   *
   * @private
   */
  dimension_: function(imageWidth, imageHeight) {
    if (imageWidth && imageHeight)
      return imageWidth + " x " + imageHeight;
    return '';
  },

  /**
   * @param {?Object} ifd
   * @return {string}
   *
   * @private
   */
  deviceModel_: function(ifd) {
    var id = 272;
    return (ifd && ifd.image && ifd.image[id] && ifd.image[id].value) || '';
  },

  /**
   * @param {Array<String>} r array of two strings representing the numerator
   *     and the denominator.
   * @return {number}
   * @private
   */
  parseRational_: function(r) {
    var num = parseInt(r[0], 10);
    var den = parseInt(r[1], 10);
    return num / den;
  },

  /**
   * Returns geolocation as a string in the form of 'latitude, longitude',
   * where the values have 3 decimal precision. Negative latitude indicates
   * south latitude and negative longitude indicates west longitude.
   * @param {?Object} ifd
   * @return {string}
   *
   * @private
   */
  geography_: function(ifd) {
    var gps = ifd && ifd.gps;
    if (!gps || !gps[1] || !gps[2] || !gps[3] || !gps[4])
      return '';

    var computeCoordinate = function(value) {
      return this.parseRational_(value[0]) +
          this.parseRational_(value[1]) / 60 +
          this.parseRational_(value[2]) / 3600;
    }.bind(this);

    var latitude =
        computeCoordinate(gps[2].value) * (gps[1].value === 'N\0' ? 1 : -1);
    var longitude =
        computeCoordinate(gps[4].value) * (gps[3].value === 'E\0' ? 1 : -1);

    return Number(latitude).toFixed(3) + ', ' + Number(longitude).toFixed(3);
  },

  /**
   * Returns device settings as a string in the form of
   * 'FNumber exposureTime focalLength isoSpeedRating'.
   * Example: 'f/2 1/120 4.67mm ISO108'.
   * @param {?Object} ifd
   * @return {string}
   *
   * @private
   */
  deviceSettings_: function(ifd) {
    var exif = ifd && ifd.exif;
    if (!exif)
      return '';

    var f = exif[33437] ? this.parseRational_(exif[33437].value) : 0;
    var fNumber = '';
    if (f) {
      fNumber = 'f/' + (Number.isInteger(f) ? f : Number(f).toFixed(1));
    }
    var exposureTime =
        exif[33434] ? exif[33434].value[0] + '/' + exif[33434].value[1] : '';
    var focalLength = exif[37386] ?
        Number(this.parseRational_(exif[37386].value)).toFixed(2) + 'mm' :
        '';
    var iso = exif[34855] ? 'ISO' + exif[34855].value : '';

    var values = [fNumber, exposureTime, focalLength, iso];

    var result = '';
    for (var i = 0; i < values.length; i++) {
      if (values[i]) {
        result += (result ? ' ' : '') + values[i];
      }
    }
    return result;
  },
});
