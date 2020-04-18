// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.support_lib_glue;

import org.chromium.android_webview.AwServiceWorkerSettings;
import org.chromium.support_lib_boundary.ServiceWorkerWebSettingsBoundaryInterface;

/**
 * Adapter between AwServiceWorkerSettings and ServiceWorkerWebSettingsBoundaryInterface.
 */
class SupportLibServiceWorkerSettingsAdapter implements ServiceWorkerWebSettingsBoundaryInterface {
    private AwServiceWorkerSettings mAwServiceWorkerSettings;

    SupportLibServiceWorkerSettingsAdapter(AwServiceWorkerSettings settings) {
        mAwServiceWorkerSettings = settings;
    }

    /* package */ AwServiceWorkerSettings getAwServiceWorkerSettings() {
        return mAwServiceWorkerSettings;
    }

    @Override
    public void setCacheMode(int mode) {
        mAwServiceWorkerSettings.setCacheMode(mode);
    }

    @Override
    public int getCacheMode() {
        return mAwServiceWorkerSettings.getCacheMode();
    }

    @Override
    public void setAllowContentAccess(boolean allow) {
        mAwServiceWorkerSettings.setAllowContentAccess(allow);
    }

    @Override
    public boolean getAllowContentAccess() {
        return mAwServiceWorkerSettings.getAllowContentAccess();
    }

    @Override
    public void setAllowFileAccess(boolean allow) {
        mAwServiceWorkerSettings.setAllowFileAccess(allow);
    }

    @Override
    public boolean getAllowFileAccess() {
        return mAwServiceWorkerSettings.getAllowFileAccess();
    }

    @Override
    public void setBlockNetworkLoads(boolean flag) {
        mAwServiceWorkerSettings.setBlockNetworkLoads(flag);
    }

    @Override
    public boolean getBlockNetworkLoads() {
        return mAwServiceWorkerSettings.getBlockNetworkLoads();
    }
}
