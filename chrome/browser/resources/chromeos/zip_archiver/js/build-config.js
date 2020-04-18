// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

'use strict';

/**
 * Namespace with build configuration options.
 * @type {Object}
 */
var buildConfig = {
  /**
   * The path to the module's configuration file, which should be a .nmf file.
   * For NaCl use 'newlib/[Release|Debug]/module.nmf' and for PNaCl use
   * 'pnacl/[Release|Debug]/module.nmf'. Defaults to PNaCl.
   * @type {string}
   */
  BUILD_MODULE_PATH: 'pnacl/Debug/module.nmf',

  /**
   * The mime type of the NaCl executable. For NaCl use 'application/x-nacl'
   * and for PNaCl use 'application/x-pnacl'. Defaults to PNaCl.
   * @type {string}
   */
  BUILD_MODULE_TYPE: 'application/x-pnacl'
};
