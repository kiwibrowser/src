// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.customtabs.dynamicmodule;

import android.net.Uri;
import org.chromium.chrome.browser.customtabs.dynamicmodule.IObjectWrapper;

/** API to customize the Chrome activity. */
interface IActivityHost {

  /** Returns the Context of the Chrome activity. */
  IObjectWrapper /* Context */ getActivityContext() = 0;

  /**
   * Sets the given view as the bottom bar.
   *
   * Compared to the overlay view, the bottom bar is automatically hidden when
   * the user scrolls down on the web content.
   */
  void setBottomBarView(in IObjectWrapper /* View */ bottomBarView) = 1;

  /*
   * Sets the overlay view.
   *
   * This view is always on top of the web content.
   */
  void setOverlayView(in IObjectWrapper /* View */ overlayView) = 2;

  /**
   * Sets the height of the bottom bar.
   *
   * Chrome uses this to calculate the bottom padding of the web content.
   */
  void setBottomBarHeight(int heightInPx) = 3;

  /**
   * Loads a URI in the existing CCT activity.
   *
   * Introduced in API version 3.
   */
  void loadUri(in Uri uri) = 4;

  /**
   * Sets the top bar view in CCT. This will not attempt to hide or remove
   * the CCT header. It should only be called once in the lifecycle of an
   * activity.

   * Introduced in API version 5.
   */
  void setTopBarView(in IObjectWrapper /* View */ topBarView) = 5;

  /**
   * Sets the height of the top bar for module-managed URLs only. This is needed
   * for CCT to calculate the web content area. It is not applicable to
   * non-module-managed URLs, i.e. landing pages, where the top bar is not
   * shown.
   *
   * @param heightInPx The desired height. Chrome will decide if it can honor
   *        the desired height at runtime.
   *
   * Introduced in API version 7.
   */
  void setTopBarHeight(int heightInPx) = 6;

  /**
   * Sends a request to create a two-way postMessage channel between the web
   * page in the host activity and the dynamic module.
   *
   * @param postMessageOrigin Identifies the client which is attempting to
   *        establish the connection. For instance, this could be a URI
   *        representation of the package name.
   * @return whether the request was accepted.
   *
   * Introduced in API version 9.
   */
  boolean requestPostMessageChannel(in Uri postMessageOrigin) = 7;

  /**
   * Posts a message to the host activity.
   *
   * @param message The message to send. No constraints are enforced upon the
   *        format; the string will be sent directly to the web contents.
   * @return An integer constant about the postMessage request result. See the
   *         docs on CustomTabsService#postMessage for more details:
   *         https://developer.android.com/reference/android/support/customtabs/CustomTabsService#postmessage.
   *
   * Introduced in API version 9.
   */
  int postMessage(in String message) = 8;

  /**
   * Sets the min height of the top bar for module-managed URLs only. This is
   * needed for sticky top bar elements. It is not applicable to
   * non-module-managed URLs, i.e. landing pages, where the top bar is not
   * shown.
   *
   * @param heightInPx The desired height. Chrome will decide if it can honor
   *        the desired height at runtime.
   *
   * Introduced in API version 8.
   */
  void setTopBarMinHeight(int heightInPx) = 9;
}
