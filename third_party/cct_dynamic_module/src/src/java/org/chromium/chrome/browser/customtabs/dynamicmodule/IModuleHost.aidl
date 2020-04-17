// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.customtabs.dynamicmodule;

import org.chromium.chrome.browser.customtabs.dynamicmodule.IObjectWrapper;

/** Chrome host that runs custom modules. */
interface IModuleHost {

  /** Returns the context of this host. */
  IObjectWrapper /* Context */ getHostApplicationContext() = 0;

  /** Returns the context of the custom module. */
  IObjectWrapper /* Context */ getModuleContext() = 1;

  /** Returns the API version of the host. */
  int getHostVersion() = 2;

  /**
   * Returns the minimum API version that needs to be supported by the module.
   *
   * The module will not be loaded if its API version is less than this value.
   */
  int getMinimumModuleVersion() = 3;
}
