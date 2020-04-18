// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.compositor.layouts;

import android.content.Context;
import android.content.pm.ApplicationInfo;
import android.content.pm.PackageManager;
import android.content.res.Resources;
import android.os.Looper;
import android.test.mock.MockContext;
import android.test.mock.MockResources;

/**
 * This is the minimal {@link Context} needed by the {@link LayoutManager} to be working properly.
 * It points to a {@link MockResources} for anything that is based on xml configurations. For
 * everything else the standard provided Context should be sufficient.
 */
public class MockContextForLayout extends MockContext {
    private final Context mValidContext;
    private final MockResourcesForLayout mResources;
    private final Resources.Theme mTheme;

    public MockContextForLayout(Context validContext) {
        mValidContext = validContext;
        mResources = new MockResourcesForLayout(validContext.getResources());
        mTheme = mResources.newTheme();
    }

    @Override
    public Resources getResources() {
        return mResources;
    }

    @Override
    public ApplicationInfo getApplicationInfo() {
        return mValidContext.getApplicationInfo();
    }

    @Override
    public Object getSystemService(String name) {
        return mValidContext.getSystemService(name);
    }

    @Override
    public PackageManager getPackageManager() {
        return mValidContext.getPackageManager();
    }

    @Override
    public Context getApplicationContext() {
        return this;
    }

    @Override
    public int checkCallingOrSelfPermission(String permission) {
        return mValidContext.checkCallingOrSelfPermission(permission);
    }

    @Override
    public Looper getMainLooper() {
        return mValidContext.getMainLooper();
    }

    @Override
    public Resources.Theme getTheme() {
        return mTheme;
    }
}