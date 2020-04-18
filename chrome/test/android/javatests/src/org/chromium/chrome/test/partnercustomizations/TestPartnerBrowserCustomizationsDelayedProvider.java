// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.test.partnercustomizations;

import android.database.Cursor;
import android.net.Uri;
import android.os.Bundle;
import android.text.TextUtils;

import org.chromium.base.annotations.MainDex;

import java.util.List;

/**
 * PartnerBrowserCustomizationsProvider example for testing. This adds one second latency for
 * query function.
 * Note: if you move or rename this class, make sure you have also updated AndroidManifest.xml.
 */
@MainDex
public class TestPartnerBrowserCustomizationsDelayedProvider
        extends TestPartnerBrowserCustomizationsProvider {
    private static String sUriPathToDelay;

    public TestPartnerBrowserCustomizationsDelayedProvider() {
        super();
        mTag = TestPartnerBrowserCustomizationsDelayedProvider.class.getSimpleName();
    }

    private void setUriPathToDelay(String path) {
        sUriPathToDelay = path;
    }

    @Override
    public Bundle call(String method, String arg, Bundle extras) {
        if (TextUtils.equals(method, "setUriPathToDelay")) {
            setUriPathToDelay(arg);
        }
        return super.call(method, arg, extras);
    }

    @Override
    public Cursor query(Uri uri, String[] projection, String selection, String[] selectionArgs,
            String sortOrder) {
        try {
            List<String> pathSegments = uri.getPathSegments();
            if (sUriPathToDelay == null
                    || (pathSegments != null && !pathSegments.isEmpty()
                            && TextUtils.equals(pathSegments.get(0), sUriPathToDelay))) {
                Thread.sleep(1000);
            }
        } catch (InterruptedException e) {
            assert false;
        }
        return super.query(uri, projection, selection, selectionArgs, sortOrder);
    }
}
