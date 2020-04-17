// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.customtabs.dynamicmodule;

import android.os.Bundle;

/**
 * Base class extracted from {@link IActivityDelegate.Stub}.
 *
 */
public class BaseActivityDelegate extends IActivityDelegate.Stub {
  protected BaseActivityDelegate() {}

  @Override
  public void onCreate(Bundle savedInstanceState) {
    throw new UnsupportedOperationException("Not implemented");
  }

  @Override
  public void onPostCreate(Bundle savedInstanceState) {
    throw new UnsupportedOperationException("Not implemented");
  }

  @Override
  public void onStart() {
    throw new UnsupportedOperationException("Not implemented");
  }

  @Override
  public void onStop() {
    throw new UnsupportedOperationException("Not implemented");
  }

  @Override
  public void onWindowFocusChanged(boolean hasFocus) {
    throw new UnsupportedOperationException("Not implemented");
  }

  @Override
  public void onSaveInstanceState(Bundle outState) {
    throw new UnsupportedOperationException("Not implemented");
  }

  @Override
  public void onRestoreInstanceState(Bundle savedInstanceState) {
    throw new UnsupportedOperationException("Not implemented");
  }

  @Override
  public void onResume() {
    throw new UnsupportedOperationException("Not implemented");
  }

  @Override
  public void onPause() {
    throw new UnsupportedOperationException("Not implemented");
  }

  @Override
  public void onDestroy() {
    throw new UnsupportedOperationException("Not implemented");
  }

  @Override
  public boolean onBackPressed() {
    throw new UnsupportedOperationException("Not implemented");
  }

  @Override
  public void onBackPressedAsync(IObjectWrapper /* Runnable */ notHandledRunnable) {
    throw new UnsupportedOperationException("Not implemented");
  }

  @Override
  public void onNavigationEvent(int navigationEvent, Bundle extras) {
    throw new UnsupportedOperationException("Not implemented");
  }

  @Override
  public void onMessageChannelReady() {
    throw new UnsupportedOperationException("Not implemented");
  }

  @Override
  public void onPostMessage(String message) {
    throw new UnsupportedOperationException("Not implemented");
  }

  @Override
  public void onPageMetricEvent(String metricName, long navigationStart,
      long offset, long navigationId) {
    throw new UnsupportedOperationException("Not implemented");
  }
}
