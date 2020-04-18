// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.components.dom_distiller.core;

import org.chromium.base.ThreadUtils;
import org.chromium.base.annotations.CalledByNative;
import org.chromium.base.annotations.JNINamespace;

/**
 * Wrapper for native dom_distiller::DomDistillerService.
 */
@JNINamespace("dom_distiller::android")
public final class DomDistillerService {

    private final long mDomDistillerServiceAndroid;
    private final DistilledPagePrefs mDistilledPagePrefs;

    private DomDistillerService(long nativeDomDistillerAndroidServicePtr) {
        mDomDistillerServiceAndroid = nativeDomDistillerAndroidServicePtr;
        mDistilledPagePrefs = new DistilledPagePrefs(
                nativeGetDistilledPagePrefsPtr(mDomDistillerServiceAndroid));
    }

    public DistilledPagePrefs getDistilledPagePrefs() {
        return mDistilledPagePrefs;
    }

    public boolean hasEntry(String entryId) {
        return nativeHasEntry(mDomDistillerServiceAndroid, entryId);
    }

    public String getUrlForEntry(String entryId) {
        return nativeGetUrlForEntry(mDomDistillerServiceAndroid, entryId);
    }

    @CalledByNative
    private static DomDistillerService create(long nativeDomDistillerServiceAndroid) {
        ThreadUtils.assertOnUiThread();
        return new DomDistillerService(nativeDomDistillerServiceAndroid);
    }

    private native boolean nativeHasEntry(long nativeDomDistillerServiceAndroid, String entryId);
    private native String nativeGetUrlForEntry(
            long nativeDomDistillerServiceAndroid, String entryId);
    private static native long nativeGetDistilledPagePrefsPtr(
            long nativeDomDistillerServiceAndroid);
}
