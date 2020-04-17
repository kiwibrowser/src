// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.customtabs.dynamicmodule;

import android.os.Bundle;

import org.chromium.chrome.browser.customtabs.dynamicmodule.IObjectWrapper;

/** Runtime providing additional features to a Chrome activity. */
interface IActivityDelegate {

  void onCreate(in Bundle savedInstanceState) = 0;

  void onPostCreate(in Bundle savedInstanceState) = 1;

  void onStart() = 2;

  void onStop() = 3;

  void onWindowFocusChanged(boolean hasFocus) = 4;

  void onSaveInstanceState(in Bundle outState) = 5;

  void onRestoreInstanceState(in Bundle savedInstanceState) = 6;

  void onResume() = 7;

  void onPause() = 8;

  void onDestroy() = 9;

  boolean onBackPressed() = 10;

  /**
   * Offers an opportunity to handle the back press event. If it is not handled,
   * the Runnable must be run.
   *
   * Introduced in API version 2.
   */
  void onBackPressedAsync(in IObjectWrapper /* Runnable */ notHandledRunnable) = 11;

  /**
   * Notify module about navigation events.
   * {@see android.support.customtabs.CustomTabsCallback#onNavigationEvent}
   *
   * Introduced in API version 4.
   */
  void onNavigationEvent(int navigationEvent, in Bundle extras) = 12;

  /**
   * Notifies the module that a postMessage channel has been created and is
   * ready for sending and receiving messages.
   *
   * Experimental API.
   */
  void onMessageChannelReady() = 13;

  /**
   * Notifies the module about messages posted to it by the web page.
   * @param message The message sent. No guarantees are made about the format.
   *
   * Experimental API.
   */
  void onPostMessage(String message) = 14;

  /**
   * Notifies the module of page load metrics, for example time until first
   * contentful paint.
   * @param metricName Name of the page load metric.
   * @param navigationStart Navigation start time in ms as SystemClock.uptimeMillis() value.
   * @param offset offset in ms from navigationStart for the page load metric.
   * @param navigationId the unique id of a navigation this metrics is related to.
   * This parameter is guaranteed to be positive value or zero in case of "no id".
   *
   * Experimental API.
   */
  void onPageMetricEvent(in String metricName, long navigationStart, long offset, long navigationId) = 15;
}
