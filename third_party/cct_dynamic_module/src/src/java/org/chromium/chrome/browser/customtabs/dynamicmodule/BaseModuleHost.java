// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.customtabs.dynamicmodule;

/**
 * Base class extracted from {@link IModuleHost.Stub}.
 *
 */
public class BaseModuleHost extends IModuleHost.Stub {

  protected BaseModuleHost() {}

  @Override
  public IObjectWrapper /* Context */ getHostApplicationContext() {
    throw new UnsupportedOperationException("Not implemented");
  }

  @Override
  public IObjectWrapper /* Context */ getModuleContext() {
    throw new UnsupportedOperationException("Not implemented");
  }

  @Override
  public int getHostVersion() {
    throw new UnsupportedOperationException("Not implemented");
  }

  @Override
  public int getMinimumModuleVersion() {
    throw new UnsupportedOperationException("Not implemented");
  }
}
