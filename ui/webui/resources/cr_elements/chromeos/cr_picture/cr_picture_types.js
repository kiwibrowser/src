// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

var CrPicture = {};

/**
 * Contains the possible types for picture list image elements.
 * @enum {string}
 */
CrPicture.SelectionTypes = {
  CAMERA: 'camera',
  FILE: 'file',
  PROFILE: 'profile',
  OLD: 'old',
  DEFAULT: 'default',
  NONE: '',
};

/**
 * An picture list image element.
 * @typedef {{
 *   dataset: {
 *     type: !CrPicture.SelectionTypes,
 *     index: (number|undefined),
 *     imageIndex: (number|undefined),
 *   },
 *   src: string,
 * }}
 */
CrPicture.ImageElement;

CrPicture.kDefaultImageUrl = 'chrome://theme/IDR_LOGIN_DEFAULT_USER';
