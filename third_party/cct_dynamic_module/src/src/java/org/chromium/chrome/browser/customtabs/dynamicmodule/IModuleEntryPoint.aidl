// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.customtabs.dynamicmodule;

import android.os.Bundle;
import org.chromium.chrome.browser.customtabs.dynamicmodule.IActivityDelegate;
import org.chromium.chrome.browser.customtabs.dynamicmodule.IActivityHost;
import org.chromium.chrome.browser.customtabs.dynamicmodule.IModuleHost;
import org.chromium.chrome.browser.customtabs.dynamicmodule.IObjectWrapper;

/** Entry point for a dynamic module. */
interface IModuleEntryPoint {

  /** Called by Chrome to perform module initialization. */
  void init(in IModuleHost moduleHost) = 0;

  /** Returns the API version supported by this module. */
  int getModuleVersion() = 1;

  /**
   * Returns the minimum API version that the host must support.
   *
   * The module will not be initialized if the host version is less than this
   * value.
   */
  int getMinimumHostVersion() = 2;

  /**
   * Called when an enhanced activity is started.
   *
   * @throws IllegalStateException if the hosted application is not created.
   */
  IActivityDelegate createActivityDelegate(in IActivityHost activityHost) = 3;

  /** Called by Chrome when the module is destroyed. */
  void onDestroy() = 4;

  /**
   * Called by Chrome when a bundle for the module is received.
   *
   * Introduced in API version 6.
   */
  void onBundleReceived(in IObjectWrapper /* Bundle */ args) = 5;
}
