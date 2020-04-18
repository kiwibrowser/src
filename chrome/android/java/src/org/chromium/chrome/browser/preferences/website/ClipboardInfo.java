// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.preferences.website;

/**
 * Clipboard information for a given origin.
 */
public class ClipboardInfo extends PermissionInfo {
    public ClipboardInfo(String origin, String embedder, boolean isIncognito) {
        super(origin, embedder, isIncognito);
    }

    @Override
    protected int getNativePreferenceValue(String origin, String embedder, boolean isIncognito) {
        return WebsitePreferenceBridge.nativeGetClipboardSettingForOrigin(
                origin, isIncognito);
    }

    @Override
    protected void setNativePreferenceValue(
            String origin, String embedder, ContentSetting value, boolean isIncognito) {
        WebsitePreferenceBridge.nativeSetClipboardSettingForOrigin(
                origin, value.toInt(), isIncognito);
    }
}
