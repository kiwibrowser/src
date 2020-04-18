// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.services;

import org.chromium.base.Callback;
import org.chromium.chrome.browser.AppHooks;
import org.chromium.chrome.browser.childaccounts.ChildAccountService;
import org.chromium.components.signin.ChildAccountStatus;

/**
 * A helper for Android EDU and child account checks.
 * Usage:
 * new AndroidEduAndChildAccountHelper() { override onParametersReady() }.start(appContext).
 */
public abstract class AndroidEduAndChildAccountHelper
        implements Callback<Integer>, AndroidEduOwnerCheckCallback {
    private Boolean mIsAndroidEduDevice;
    private @ChildAccountStatus.Status Integer mChildAccountStatus;
    // Abbreviated to < 20 chars.
    private static final String TAG = "EduChildHelper";

    /** The callback called when Android EDU and child account parameters are known. */
    public abstract void onParametersReady();

    /** @return Whether the device is Android EDU device. */
    public boolean isAndroidEduDevice() {
        return mIsAndroidEduDevice;
    }

    /** @return The status of the device regarding child accounts. */
    public @ChildAccountStatus.Status int getChildAccountStatus() {
        return mChildAccountStatus;
    }

    /**
     * Starts fetching the Android EDU and child accounts information.
     * Calls onParametersReady() once the information is fetched.
     */
    public void start() {
        ChildAccountService.checkChildAccountStatus(this);
        AppHooks.get().checkIsAndroidEduDevice(this);
        // TODO(aruslan): Should we start a watchdog to kill if Child/Edu stuff takes too long?
    }

    private void checkDone() {
        if (mIsAndroidEduDevice == null || mChildAccountStatus == null) return;
        onParametersReady();
    }

    // AndroidEdu.OwnerCheckCallback:
    @Override
    public void onSchoolCheckDone(boolean isAndroidEduDevice) {
        mIsAndroidEduDevice = isAndroidEduDevice;
        checkDone();
    }

    // Callback<Integer>:
    @Override
    public void onResult(@ChildAccountStatus.Status Integer status) {
        mChildAccountStatus = status;
        checkDone();
    }
}
