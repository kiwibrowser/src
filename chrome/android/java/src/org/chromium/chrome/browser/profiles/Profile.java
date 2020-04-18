// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.profiles;

import org.chromium.base.VisibleForTesting;
import org.chromium.base.annotations.CalledByNative;
import org.chromium.base.library_loader.LibraryProcessType;
import org.chromium.chrome.browser.cookies.CookiesFetcher;
import org.chromium.content.browser.BrowserStartupController;

/**
 * Wrapper that allows passing a Profile reference around in the Java layer.
 */
public class Profile {

    /** Whether this wrapper corresponds to an off the record Profile. */
    private final boolean mIsOffTheRecord;

    /** Pointer to the Native-side ProfileAndroid. */
    private long mNativeProfileAndroid;

    private Profile(long nativeProfileAndroid) {
        mNativeProfileAndroid = nativeProfileAndroid;
        mIsOffTheRecord = nativeIsOffTheRecord(mNativeProfileAndroid);
    }

    public static Profile getLastUsedProfile() {
        // TODO(crbug.com/704025): turn this into an assert once the bug is fixed
        if (!BrowserStartupController.get(LibraryProcessType.PROCESS_BROWSER)
                        .isStartupSuccessfullyCompleted()) {
            throw new IllegalStateException("Browser hasn't finished initialization yet!");
        }
        return (Profile) nativeGetLastUsedProfile();
    }

    /**
     * Destroys the Profile.  Destruction is delayed until all associated
     * renderers have been killed, so the profile might not be destroyed upon returning from
     * this call.
     */
    public void destroyWhenAppropriate() {
        nativeDestroyWhenAppropriate(mNativeProfileAndroid);
    }

    public Profile getOriginalProfile() {
        return (Profile) nativeGetOriginalProfile(mNativeProfileAndroid);
    }

    public Profile getOffTheRecordProfile() {
        return (Profile) nativeGetOffTheRecordProfile(mNativeProfileAndroid);
    }

    public boolean hasOffTheRecordProfile() {
        return nativeHasOffTheRecordProfile(mNativeProfileAndroid);
    }

    public boolean isOffTheRecord() {
        return mIsOffTheRecord;
    }

    /**
     * @return Whether the profile is signed in to a child account.
     */
    public boolean isChild() {
        return nativeIsChild(mNativeProfileAndroid);
    }

    /**
     * @return Whether or not the native side profile exists.
     */
    @VisibleForTesting
    public boolean isNativeInitialized() {
        return mNativeProfileAndroid != 0;
    }

    @CalledByNative
    private static Profile create(long nativeProfileAndroid) {
        return new Profile(nativeProfileAndroid);
    }

    @CalledByNative
    private void onNativeDestroyed() {
        mNativeProfileAndroid = 0;

        if (mIsOffTheRecord) {
            CookiesFetcher.deleteCookiesIfNecessary();
        }
    }

    @CalledByNative
    private long getNativePointer() {
        return mNativeProfileAndroid;
    }

    private static native Object nativeGetLastUsedProfile();
    private native void nativeDestroyWhenAppropriate(long nativeProfileAndroid);
    private native Object nativeGetOriginalProfile(long nativeProfileAndroid);
    private native Object nativeGetOffTheRecordProfile(long nativeProfileAndroid);
    private native boolean nativeHasOffTheRecordProfile(long nativeProfileAndroid);
    private native boolean nativeIsOffTheRecord(long nativeProfileAndroid);
    private native boolean nativeIsChild(long nativeProfileAndroid);
}
