// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.media;

import android.support.v7.media.MediaRouteProvider;
import android.support.v7.media.MediaRouteProviderService;

import org.chromium.base.Log;

/**
 * Service for registering {@link TestMediaRouteProvider} using the support library.
 */
public class TestMediaRouteProviderService extends MediaRouteProviderService {
    private static final String TAG = "TestMRPService";

    @Override
    public MediaRouteProvider onCreateMediaRouteProvider() {
        Log.i(TAG, "creating TestMRP");
        return new TestMediaRouteProvider(this);
    }
}
