// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.vr;

import android.content.Intent;
import android.view.WindowManager;

import org.chromium.chrome.browser.customtabs.SeparateTaskCustomTabActivity;
import org.chromium.chrome.browser.util.IntentUtils;
import org.chromium.chrome.browser.vr_shell.VrIntentUtils;
import org.chromium.chrome.browser.vr_shell.VrShellDelegate;

/**
 * A subclass of SeparateTaskCustomTabActivity created when starting Chrome in VR mode.
 *
 * The main purpose of this activity is to add flexibility to the way Chrome is started when the
 * user's phone is already in their VR headset (e.g, we want to hide the System UI).
 */
public class CustomTabVrActivity extends SeparateTaskCustomTabActivity {
    @Override
    public void preInflationStartup() {
        assert VrIntentUtils.getHandlerInstance().isTrustedDaydreamIntent(getIntent());

        // Set VR specific window mode.
        getWindow().addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);
        getWindow().getDecorView().setSystemUiVisibility(VrShellDelegate.VR_SYSTEM_UI_FLAGS);

        super.preInflationStartup();
    }

    @Override
    protected boolean isStartedUpCorrectly(Intent intent) {
        if (!VrIntentUtils.getHandlerInstance().isTrustedDaydreamIntent(getIntent())) {
            return false;
        }
        return super.isStartedUpCorrectly(intent);
    }

    @Override
    protected Intent validateIntent(final Intent intent) {
        return IntentUtils.sanitizeIntent(intent);
    }
}
