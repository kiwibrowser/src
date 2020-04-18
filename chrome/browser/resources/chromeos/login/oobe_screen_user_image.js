// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * @fileoverview Oobe user image screen implementation.
 */

login.createScreen('UserImageScreen', 'user-image', function() {
  var CONTEXT_KEY_IS_CAMERA_PRESENT = 'isCameraPresent';
  var CONTEXT_KEY_SELECTED_IMAGE_INDEX = 'selectedImageIndex';
  var CONTEXT_KEY_SELECTED_IMAGE_URL = 'selectedImageURL';
  var CONTEXT_KEY_PROFILE_PICTURE_DATA_URL = 'profilePictureDataURL';
  var CONTEXT_KEY_IS_PROFILE_PICTURE_AVAILABLE = 'isProfilePictureAvailable';

  return {
    EXTERNAL_API: ['setDefaultImages', 'hideCurtain'],

    /** @override */
    decorate: function(element) {
      var self = this;
      this.context.addObserver(
          CONTEXT_KEY_IS_CAMERA_PRESENT, function(present) {
            $('changePicture').cameraPresent = present;
          });
      this.context.addObserver(
          CONTEXT_KEY_SELECTED_IMAGE_INDEX, this.setSelectedImageIndex_);
      this.context.addObserver(
          CONTEXT_KEY_SELECTED_IMAGE_URL, this.setSelectedImageUrl_);
      this.context.addObserver(
          CONTEXT_KEY_PROFILE_PICTURE_DATA_URL, function(url) {
            self.profileImageLoading = false;
            if (url)
              $('changePicture').setProfileImageUrl(url, false /* selected */);
          });
      this.context.addObserver(
          CONTEXT_KEY_IS_PROFILE_PICTURE_AVAILABLE, function(available) {
            if (!available) {
              self.profileImageLoading = false;
              // Empty url hides profile image selection choice.
              $('changePicture').setProfileImageUrl('', false /* selected */);
            }
          });

      this.profileImageLoading = true;
      chrome.send('getImages');
    },

    /* @type {string} Unused for change picture dialog. */
    get header() {},

    /**
     * Buttons in oobe wizard's button strip.
     * @type {array} Array of Buttons.
     */
    get buttons() {
      var okButton = this.ownerDocument.createElement('oobe-text-button');
      okButton.id = 'ok-button';
      okButton.textContent = loadTimeData.getString('okButtonText');
      okButton.setAttribute('inverse', '');
      okButton.addEventListener('click', this.acceptImage_.bind(this));
      return [okButton];
    },

    /**
     * True if the Profile image is being loaded.
     * @type {boolean}
     */
    get profileImageLoading() {
      return this.profileImageLoading_;
    },
    set profileImageLoading(value) {
      this.profileImageLoading_ = value;
      $('user-image-screen-main')
          .classList.toggle('profile-image-loading', value);
      if (value)
        announceAccessibleMessage(loadTimeData.getString('syncingPreferences'));
    },

    /**
     * Event handler that is invoked just before the screen is shown.
     * @param {object} data Screen init payload.
     */
    onBeforeShow: function(data) {
      Oobe.getInstance().headerHidden = true;
      this.loading = true;
      chrome.send('onUserImageScreenShown');
    },

    /**
     * Event handler that is invoked just before the screen is hidden.
     */
    onBeforeHide: function() {
      this.loading = false;
    },

    /**
     * Accepts currently selected image, if possible.
     * @private
     */
    acceptImage_: function() {
      var okButton = $('ok-button');
      if (!okButton.disabled) {
        okButton.disabled = true;
        chrome.send('onUserImageAccepted');
        this.loading = true;
      }
    },

    /**
     * Appends default images to the image grid. Should only be called once.
     * @param {Array<{url: string, author: string, website: string}>} images
     *   An array of default images data, including URL, author and website.
     */
    setDefaultImages: function(info) {
      $('changePicture').defaultImages = info.images;
      $('changePicture').firstDefaultImageIndex = info.first;
      chrome.send('screenReady');
    },

    /**
     * Selects user image with the given index.
     * @param {number} index Index of the image to select.
     * @private
     */
    setSelectedImageIndex_: function(index) {
      $('changePicture').selectedImageIndex = index;
    },

    /**
     * Selects user image with the given URL.
     * @param {string} url URL of the image to select.
     * @private
     */
    setSelectedImageUrl_: function(url) {
      if (!url)
        return;
      $('changePicture').selectedImageUrl = url;
    },

    get loading() {
      return this.classList.contains('loading');
    },

    set loading(value) {
      this.classList.toggle('loading', value);
      $('oobe').classList.toggle('image-loading', value);
      Oobe.getInstance().updateScreenSize(this);
    },

    /**
     * Hides curtain with spinner.
     */
    hideCurtain: function() {
      this.loading = false;
      $('changePicture').focus();
    },

    /**
     * Updates localized content of the screen that is not updated via template.
     */
    updateLocalizedContent: function() {
      $('changePicture').i18nUpdateLocale();
    },
  };
});
