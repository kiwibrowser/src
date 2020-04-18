// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.device.nfc;

import org.chromium.device.mojom.NfcMessage;
import org.chromium.device.mojom.NfcRecord;
import org.chromium.device.mojom.NfcRecordType;

/**
 * Utility class that provides validation of NfcMessage.
 */
public final class NfcMessageValidator {
    /**
     * Validates NfcMessage.
     *
     * @param message to be validated.
     * @return true if message is valid, false otherwise.
     */
    public static boolean isValid(NfcMessage message) {
        if (message == null || message.data == null || message.data.length == 0) {
            return false;
        }

        for (int i = 0; i < message.data.length; ++i) {
            if (!isValid(message.data[i])) return false;
        }
        return true;
    }

    /**
     * Checks that NfcRecord#data and NfcRecord#mediaType fields are valid. NfcRecord#data and
     * NfcRecord#mediaType fields are omitted for the record with EMPTY type.
     */
    private static boolean isValid(NfcRecord record) {
        if (record == null) return false;
        if (record.recordType == NfcRecordType.EMPTY) return true;
        return record.data != null && record.mediaType != null && !record.mediaType.isEmpty();
    }
}
