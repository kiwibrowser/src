// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.vr_shell.nfc_apk;

import android.app.Activity;
import android.os.Bundle;

import org.chromium.chrome.browser.vr_shell.util.NfcSimUtils;

/**
 * Activity to simulate an NFC scan of a Daydream View VR headset. This is so
 * tests that cannot interact directly with Java, e.g. Telemetry tests, can
 * still force Chrome to enter VR.
 */
public class SimNfcActivity extends Activity {
    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        // Send off intent and close this application
        startActivity(NfcSimUtils.makeNfcIntent());
        finish();
    }
}
