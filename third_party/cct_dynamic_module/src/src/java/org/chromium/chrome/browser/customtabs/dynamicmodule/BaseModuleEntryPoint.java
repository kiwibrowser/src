// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.customtabs.dynamicmodule;

/**
 * Base class extracted from {@link IModuleEntryPoint.Stub}.
 *
 */
public class BaseModuleEntryPoint extends IModuleEntryPoint.Stub {

  protected BaseModuleEntryPoint() {}

  @Override
  public void init(IModuleHost moduleHost) {
    throw new UnsupportedOperationException("Not implemented");
  }

  @Override
  public int getModuleVersion() {
    throw new UnsupportedOperationException("Not implemented");
  }

  @Override
  public int getMinimumHostVersion() {
    throw new UnsupportedOperationException("Not implemented");
  }

  @Override
  public IActivityDelegate createActivityDelegate(IActivityHost activityHost) {
    throw new UnsupportedOperationException("Not implemented");
  }

  @Override
  public void onDestroy() {
    throw new UnsupportedOperationException("Not implemented");
  }

  @Override
  public void onBundleReceived(IObjectWrapper /* Bundle */ bundle) {
    throw new UnsupportedOperationException("Not implemented");
  }
}
