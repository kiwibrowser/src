// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.ui.base;

import android.view.View;

/**
 * A utility class that has helper methods for Android view.
 */
public final class ViewUtils {
    // Prevent instantiation.
    private ViewUtils() {}

    /**
     * @return {@code true} if the given view has a focus.
     */
    public static boolean hasFocus(View view) {
        // If the container view is not focusable, we consider it always focused from
        // Chromium's point of view.
        return !isFocusable(view) ? true : view.hasFocus();
    }

    /**
     * Requests focus on the given view.
     *
     * @param view A {@link View} to request focus on.
     */
    public static void requestFocus(View view) {
        if (isFocusable(view) && !view.isFocused()) view.requestFocus();
    }

    private static boolean isFocusable(View view) {
        return view.isInTouchMode() ? view.isFocusableInTouchMode() : view.isFocusable();
    }
}
