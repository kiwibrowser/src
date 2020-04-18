// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.webapk.test;

import android.content.pm.ApplicationInfo;
import android.content.pm.PackageInfo;
import android.content.res.Resources;
import android.os.Bundle;

import org.mockito.Mockito;
import org.robolectric.RuntimeEnvironment;
import org.robolectric.Shadows;
import org.robolectric.shadows.ShadowPackageManager;

/**
 * Helper class for WebAPK JUnit tests.
 */
public class WebApkTestHelper {

    /**
     * Registers WebAPK. This function also creates an empty resource for the WebAPK.
     * @param packageName The package to register
     * @param metaData Bundle with meta data from WebAPK's Android Manifest.
     */
    public static void registerWebApkWithMetaData(String packageName, Bundle metaData) {
        ShadowPackageManager packageManager =
                Shadows.shadowOf(RuntimeEnvironment.application.getPackageManager());
        Resources res = Mockito.mock(Resources.class);
        packageManager.resources.put(packageName, res);
        packageManager.addPackage(newPackageInfo(packageName, metaData));
    }

    /** Sets the resource for the given package name. */
    public static void setResource(String packageName, Resources res) {
        ShadowPackageManager packageManager =
                Shadows.shadowOf(RuntimeEnvironment.application.getPackageManager());
        packageManager.resources.put(packageName, res);
    }

    private static PackageInfo newPackageInfo(String packageName, Bundle metaData) {
        ApplicationInfo applicationInfo = new ApplicationInfo();
        applicationInfo.metaData = metaData;
        PackageInfo packageInfo = new PackageInfo();
        packageInfo.packageName = packageName;
        packageInfo.applicationInfo = applicationInfo;
        return packageInfo;
    }
}
