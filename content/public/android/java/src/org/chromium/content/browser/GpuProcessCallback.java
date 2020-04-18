// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.content.browser;

import android.view.Surface;

import org.chromium.base.UnguessableToken;
import org.chromium.base.annotations.JNINamespace;
import org.chromium.content.common.IGpuProcessCallback;
import org.chromium.content.common.SurfaceWrapper;

@JNINamespace("content")
class GpuProcessCallback extends IGpuProcessCallback.Stub {
    GpuProcessCallback() {}

    @Override
    public void forwardSurfaceForSurfaceRequest(UnguessableToken requestToken, Surface surface) {
        nativeCompleteScopedSurfaceRequest(requestToken, surface);
    }

    @Override
    public SurfaceWrapper getViewSurface(int surfaceId) {
        Surface surface = nativeGetViewSurface(surfaceId);
        if (surface == null) {
            return null;
        }
        return new SurfaceWrapper(surface);
    }

    private static native void nativeCompleteScopedSurfaceRequest(
            UnguessableToken requestToken, Surface surface);
    private static native Surface nativeGetViewSurface(int surfaceId);
};
