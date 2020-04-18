// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.toolbar;

import org.chromium.chrome.browser.modelutil.PropertyObservable;

/**
 * All of the state for the bottom toolbar, updated by the {@link BottomToolbarController}.
 */
public class BottomToolbarModel extends PropertyObservable<BottomToolbarModel.PropertyKey> {
    /** The different properties that can change on the bottom toolbar. */
    public static class PropertyKey {
        public static final PropertyKey Y_OFFSET = new PropertyKey();
        public static final PropertyKey ANDROID_VIEW_VISIBILITY = new PropertyKey();

        private PropertyKey() {}
    }

    /** The Y offset of the view in px. */
    private int mYOffsetPx;

    /** The visibility of the Android view version of the toolbar. */
    private int mAndroidViewVisibility;

    /** Default constructor. */
    public BottomToolbarModel() {}

    /**
     * @param offsetPx The current Y offset in px.
     */
    public void setYOffset(int offsetPx) {
        mYOffsetPx = offsetPx;
        notifyPropertyChanged(PropertyKey.Y_OFFSET);
    }

    /**
     * @return The current Y offset in px.
     */
    public int getYOffset() {
        return mYOffsetPx;
    }

    /**
     * @param visibility The visibility of the Android view version of the toolbar.
     */
    public void setAndroidViewVisibility(int visibility) {
        mAndroidViewVisibility = visibility;
        notifyPropertyChanged(PropertyKey.ANDROID_VIEW_VISIBILITY);
    }

    /**
     * @return The visibility of the Android view version of the toolbar.
     */
    public int getAndroidViewVisibility() {
        return mAndroidViewVisibility;
    }
}
