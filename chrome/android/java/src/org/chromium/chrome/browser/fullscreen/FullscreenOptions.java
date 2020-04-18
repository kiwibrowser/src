// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.fullscreen;

import android.os.Parcel;
import android.os.Parcelable;

/**
 * Options to control a fullscreen request.
 */
public class FullscreenOptions implements Parcelable {
    private boolean mShowNavigationBar;

    /**
     * Constructs FullscreenOptions.
     *
     * @param showNavigationBar Whether the navigation bar should be shown.
     */
    public FullscreenOptions(boolean showNavigationBar) {
        mShowNavigationBar = showNavigationBar;
    }

    /**
     * @return Whether the navigation bar should be shown.
     */
    public boolean showNavigationBar() {
        return mShowNavigationBar;
    }

    @Override
    public boolean equals(Object obj) {
        if (!(obj instanceof FullscreenOptions)) {
            return false;
        }
        FullscreenOptions options = (FullscreenOptions) obj;
        return mShowNavigationBar == options.mShowNavigationBar;
    }

    /**
     * The Parcelable interface.
     * */

    public static final Parcelable.Creator<FullscreenOptions> CREATOR =
            new Parcelable.Creator<FullscreenOptions>() {
                @Override
                public FullscreenOptions createFromParcel(Parcel in) {
                    return new FullscreenOptions(in);
                }

                @Override
                public FullscreenOptions[] newArray(int size) {
                    return new FullscreenOptions[size];
                }
            };

    /**
     * {@inheritDoc}
     */
    @Override
    public int describeContents() {
        return 0;
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void writeToParcel(Parcel out, int flags) {
        out.writeByte(mShowNavigationBar ? (byte) 1 : (byte) 0);
    }

    private FullscreenOptions(Parcel in) {
        mShowNavigationBar = in.readByte() != 0;
    }
}
