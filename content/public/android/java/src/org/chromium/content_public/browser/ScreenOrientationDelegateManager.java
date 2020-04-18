// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.content_public.browser;

import org.chromium.content.browser.ScreenOrientationProvider;

/**
 * An static class used for managing registration of ScreenOrientationDelegates.
 */
public final class ScreenOrientationDelegateManager {
    /**
     * Sets the current ScreenOrientationDelegate.
     */
    public static void setOrientationDelegate(ScreenOrientationDelegate delegate) {
        ScreenOrientationProvider.setOrientationDelegate(delegate);
    }

    private ScreenOrientationDelegateManager() {}
}