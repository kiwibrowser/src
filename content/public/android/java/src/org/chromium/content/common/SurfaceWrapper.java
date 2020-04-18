// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.content.common;

import android.os.Parcel;
import android.os.Parcelable;
import android.view.Surface;

import org.chromium.base.annotations.MainDex;

/**
 * A wrapper for marshalling a Surface without self-destruction.
 */
@MainDex
public class SurfaceWrapper implements Parcelable {
    private final Surface mSurface;

    public SurfaceWrapper(Surface surface) {
        mSurface = surface;
    }

    public Surface getSurface() {
        return mSurface;
    }

    @Override
    public int describeContents() {
        return 0;
    }

    @Override
    public void writeToParcel(Parcel out, int flags) {
        // Ignore flags so that the Surface won't call release()
        mSurface.writeToParcel(out, 0);
    }

    public static final Parcelable.Creator<SurfaceWrapper> CREATOR =
            new Parcelable.Creator<SurfaceWrapper>() {
                @Override
                public SurfaceWrapper createFromParcel(Parcel in) {
                    Surface surface = Surface.CREATOR.createFromParcel(in);
                    return new SurfaceWrapper(surface);
                }

                @Override
                public SurfaceWrapper[] newArray(int size) {
                    return new SurfaceWrapper[size];
                }
            };
}
