// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.support_lib_boundary;

// Technically this interface is not needed until we add a method to WebSettings with an
// android.webkit parameter or android.webkit return value. But for forwards compatibility all
// app-facing classes should have a boundary-interface that the WebView glue layer can build
// against.
/**
 * Boundary interface for WebSettingsCompat.
 */
public interface WebSettingsBoundaryInterface {
    void setOffscreenPreRaster(boolean enabled);
    boolean getOffscreenPreRaster();

    void setSafeBrowsingEnabled(boolean enabled);
    boolean getSafeBrowsingEnabled();

    void setDisabledActionModeMenuItems(int menuItems);
    int getDisabledActionModeMenuItems();
}
