// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * @fileoverview
 * 'cr-picture-pane' is a Polymer element used to show either a profile
 * picture or a camera image preview.
 */

Polymer({
  is: 'cr-picture-pane',

  behaviors: [CrPngBehavior],

  properties: {

    /** Whether the camera is present / available */
    cameraPresent: Boolean,

    /** Image source to show when imageType != CAMERA. */
    imageSrc: {
      type: String,
      observer: 'imageSrcChanged_',
    },

    /** Image URL to use when imageType != CAMERA. */
    imageUrl: {
      type: String,
      value: '',
    },

    /**
     * The type of image to display in the preview.
     * @type {CrPicture.SelectionTypes}
     */
    imageType: {
      type: String,
      value: CrPicture.SelectionTypes.NONE,
    },

    /** Strings provided by host */
    discardImageLabel: String,
    previewAltText: String,
    takePhotoLabel: String,
    captureVideoLabel: String,
    switchModeToCameraLabel: String,
    switchModeToVideoLabel: String,

    /** Whether camera video mode is enabled */
    cameraVideoModeEnabled: Boolean,

    /** Whether the camera should be shown and active (started). */
    cameraActive_: {
      type: Boolean,
      computed: 'getCameraActive_(cameraPresent, imageType)',
      observer: 'cameraActiveChanged_',
    },
  },

  /**
   * Tells the camera to take a photo; the camera will fire a 'photo-taken'
   * event when the photo is completed.
   */
  takePhoto: function() {
    var camera = /** @type {?CrCameraElement} */ (this.$$('#camera'));
    if (camera)
      camera.takePhoto();
  },

  /** Tells the pane to focus the main action button. */
  focusActionButton: function() {
    if (this.showDiscard_())
      this.$.discardImage.focus();
    else if (this.cameraActive_)
      this.$$('#camera').focusTakePhotoButton();
  },

  /**
   * @return {boolean}
   * @private
   */
  getCameraActive_: function() {
    return this.cameraPresent &&
        this.imageType == CrPicture.SelectionTypes.CAMERA;
  },

  /** @private */
  cameraActiveChanged_: function() {
    var camera = /** @type {?CrCameraElement} */ (this.$$('#camera'));
    if (!camera)
      return;  // Camera will be started when attached.
    if (this.cameraActive_)
      camera.startCamera();
    else
      camera.stopCamera();
  },

  /** @private */
  imageSrcChanged_: function() {
    /**
     * If current image URL is an object URL created below then revoke it to
     * prevent this code from using more than one object URL per document.
     */
    if (this.imageUrl.startsWith('blob:'))
      URL.revokeObjectURL(this.imageUrl);

    /**
     * Data URLs for PNG images can be large. Create an object URL to avoid
     * URL length limits.
     */
    var image = /** @type {!HTMLImageElement} */ (this.$$('#image'));
    if (this.imageSrc.startsWith('data:image/png')) {
      var byteString = atob(this.imageSrc.split(',')[1]);
      var bytes = new Uint8Array(byteString.length);
      for (var i = 0; i < byteString.length; i++)
        bytes[i] = byteString.charCodeAt(i);
      var blob = new Blob([bytes], {'type': 'image/png'});
      // Use first frame as placeholder while rest of image loads.
      image.style.backgroundImage = 'url(' +
          CrPngBehavior.convertImageSequenceToPng([this.imageSrc]) + ')';
      this.imageUrl = URL.createObjectURL(blob);
    } else {
      image.style.backgroundImage = 'none';
      this.imageUrl = this.imageSrc;
    }
  },

  /**
   * @return {boolean}
   * @private
   */
  showImagePreview_: function() {
    return !this.cameraActive_ && !!this.imageSrc;
  },

  /**
   * @return {boolean}
   * @private
   */
  showDiscard_: function() {
    return this.imageType == CrPicture.SelectionTypes.OLD;
  },

  /** @private */
  onTapDiscardImage_: function() {
    this.fire('discard-image');
  },

  /**
   * Returns the image to use for 'src'.
   * @param {string} url
   * @return {string}
   * @private
   */
  getImgSrc_: function(url) {
    // Always use 2x user image for preview.
    if (url.startsWith('chrome://theme'))
      return url + '@2x';

    return url;
  },
});
