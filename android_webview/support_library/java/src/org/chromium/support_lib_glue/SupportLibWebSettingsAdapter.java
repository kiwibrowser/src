// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.support_lib_glue;

import org.chromium.android_webview.AwSettings;
import org.chromium.support_lib_boundary.WebSettingsBoundaryInterface;

/**
 * Adapter between WebSettingsBoundaryInterface and AwSettings.
 */
class SupportLibWebSettingsAdapter implements WebSettingsBoundaryInterface {
    private final AwSettings mAwSettings;

    public SupportLibWebSettingsAdapter(AwSettings awSettings) {
        mAwSettings = awSettings;
    }

    @Override
    public void setOffscreenPreRaster(boolean enabled) {
        mAwSettings.setOffscreenPreRaster(enabled);
    }

    @Override
    public boolean getOffscreenPreRaster() {
        return mAwSettings.getOffscreenPreRaster();
    }

    @Override
    public void setSafeBrowsingEnabled(boolean enabled) {
        mAwSettings.setSafeBrowsingEnabled(enabled);
    }

    @Override
    public boolean getSafeBrowsingEnabled() {
        return mAwSettings.getSafeBrowsingEnabled();
    }

    @Override
    public void setDisabledActionModeMenuItems(int menuItems) {
        mAwSettings.setDisabledActionModeMenuItems(menuItems);
    }

    @Override
    public int getDisabledActionModeMenuItems() {
        return mAwSettings.getDisabledActionModeMenuItems();
    }
}
