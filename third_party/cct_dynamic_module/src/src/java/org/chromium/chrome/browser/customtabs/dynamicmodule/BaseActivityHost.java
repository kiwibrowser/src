// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.customtabs.dynamicmodule;

import android.net.Uri;

/**
 * Base class extracted from {@link IActivityHost.Stub}.
 *
 */
public class BaseActivityHost extends IActivityHost.Stub {

  protected BaseActivityHost() {}

  @Override
  public IObjectWrapper /* Context */ getActivityContext() {
    throw new UnsupportedOperationException("Not implemented");
  }

  @Override
  public void setBottomBarView(IObjectWrapper /* View */ bottomBarView) {
    throw new UnsupportedOperationException("Not implemented");
  }

  @Override
  public void setOverlayView(IObjectWrapper /* View */ overlayView) {
    throw new UnsupportedOperationException("Not implemented");
  }

  @Override
  public void setBottomBarHeight(int heightInPx) {
    throw new UnsupportedOperationException("Not implemented");
  }

  @Override
  public void loadUri(Uri uri) {
    throw new UnsupportedOperationException("Not implemented");
  }

  @Override
  public void setTopBarView(IObjectWrapper /* View */ topBarView) {
    throw new UnsupportedOperationException("Not implemented");
  }

  @Override
  public void setTopBarHeight(int heightInPx) {
    throw new UnsupportedOperationException("Not implemented");
  }

  @Override
  public boolean requestPostMessageChannel(Uri postMessageOrigin) {
    throw new UnsupportedOperationException("Not implemented");
  }

  @Override
  public int postMessage(String message) {
    throw new UnsupportedOperationException("Not implemented");
  }

  @Override
  public void setTopBarMinHeight(int heightInPx) {
    throw new UnsupportedOperationException("Not implemented");
  }
}
