// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.vr_shell.util;

import android.content.ActivityNotFoundException;
import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.nfc.NdefMessage;
import android.nfc.NdefRecord;
import android.nfc.NfcAdapter;

import org.chromium.chrome.browser.vr_shell.TestVrShellDelegate;
import org.chromium.content.browser.test.util.Criteria;
import org.chromium.content.browser.test.util.CriteriaHelper;

import java.nio.ByteBuffer;
import java.nio.ByteOrder;

/**
 * Utility class to simulate a Daydream View NFC tag being scanned.
 */
public class NfcSimUtils {
    private static final String DETECTION_ACTIVITY = ".nfc.ViewerDetectionActivity";
    // TODO(bsheedy): Use constants from VrCore if ever exposed
    private static final String APPLICATION_RECORD_STRING = "com.google.vr.vrcore";
    private static final String RESERVED_KEY = "google.vr/rsvd";
    private static final String VERSION_KEY = "google.vr/ver";
    private static final String DATA_KEY = "google.vr/data";
    private static final ByteOrder BYTE_ORDER = ByteOrder.BIG_ENDIAN;
    // Hard coded viewer ID (0x03) instead of using NfcParams and converting
    // to a byte array because NfcParams were removed from the GVR SDK
    private static final byte[] PROTO_BYTES = new byte[] {(byte) 0x08, (byte) 0x03};
    private static final int VERSION = 123;
    private static final int RESERVED = 456;

    // VrCore always seems to recover within ~10 seconds, so give it that plus some extra to be safe
    private static final int NFC_SCAN_TIMEOUT_MS = 15000;
    private static final int NFC_SCAN_INTERVAL_MS = 5000;

    /**
     * Makes an Intent that triggers VrCore as if the Daydream headset's NFC
     * tag was scanned.
     * @return The intent to send to VrCore to simulate an NFC scan.
     */
    public static Intent makeNfcIntent() {
        Intent nfcIntent = new Intent(NfcAdapter.ACTION_NDEF_DISCOVERED);
        NdefMessage[] messages = new NdefMessage[] {new NdefMessage(
                new NdefRecord[] {NdefRecord.createMime(RESERVED_KEY, intToByteArray(RESERVED)),
                        NdefRecord.createMime(VERSION_KEY, intToByteArray(VERSION)),
                        NdefRecord.createMime(DATA_KEY, PROTO_BYTES),
                        NdefRecord.createApplicationRecord(APPLICATION_RECORD_STRING)})};
        nfcIntent.putExtra(NfcAdapter.EXTRA_NDEF_MESSAGES, messages);
        nfcIntent.setComponent(new ComponentName(
                APPLICATION_RECORD_STRING, APPLICATION_RECORD_STRING + DETECTION_ACTIVITY));
        return nfcIntent;
    }

    /**
     * Simulates the NFC tag of the Daydream headset being scanned.
     * @param context The Context that the NFC scan activity will be started from.
     */
    public static void simNfcScan(Context context) {
        Intent nfcIntent = NfcSimUtils.makeNfcIntent();
        try {
            context.startActivity(nfcIntent);
        } catch (ActivityNotFoundException e) {
            // On unsupported devices, won't find VrCore -> Do nothing
        }
    }

    /**
     * Repeatedly simulates an NFC scan until VR is actually entered. This is a workaround for
     * https://crbug.com/736527.
     * @param context The Context that the NFC scan activity will be started from.
     */
    public static void simNfcScanUntilVrEntry(final Context context) {
        final TestVrShellDelegate delegate = VrShellDelegateUtils.getDelegateInstance();
        delegate.setExpectingBroadcast();
        CriteriaHelper.pollInstrumentationThread(new Criteria() {
            @Override
            public boolean isSatisfied() {
                if (!delegate.isExpectingBroadcast()) return true;
                simNfcScan(context);
                return false;
            }
        }, NFC_SCAN_TIMEOUT_MS, NFC_SCAN_INTERVAL_MS);
    }

    private static byte[] intToByteArray(int i) {
        final ByteBuffer bb = ByteBuffer.allocate(Integer.SIZE / Byte.SIZE);
        bb.order(BYTE_ORDER);
        bb.putInt(i);
        return bb.array();
    }
}
