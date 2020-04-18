// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chromecast.shell;

import com.google.android.things.update.UpdateManager;

import org.chromium.base.annotations.CalledByNative;
import org.chromium.base.annotations.JNINamespace;

/**
 * Java implementation of CastSysInfoAndroidThings methods.
 */
@JNINamespace("chromecast")
public final class CastSysInfoAndroidThings {
    @CalledByNative
    private static String getReleaseChannel() {
        return UpdateManager.getInstance().getChannel();
    }
}
