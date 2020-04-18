// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.device.nfc;

import android.net.Uri;
import android.nfc.NdefMessage;
import android.nfc.NdefRecord;
import android.os.Build;

import org.chromium.base.ApiCompatibilityUtils;
import org.chromium.base.Log;
import org.chromium.device.mojom.NfcMessage;
import org.chromium.device.mojom.NfcRecord;
import org.chromium.device.mojom.NfcRecordType;

import java.io.UnsupportedEncodingException;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;

/**
 * Utility class that provides convesion between Android NdefMessage
 * and mojo NfcMessage data structures.
 */
public final class NfcTypeConverter {
    private static final String TAG = "NfcTypeConverter";
    private static final String DOMAIN = "w3.org";
    private static final String TYPE = "webnfc";
    private static final String WEBNFC_URN = DOMAIN + ":" + TYPE;
    private static final String TEXT_MIME = "text/plain";
    private static final String JSON_MIME = "application/json";
    private static final String CHARSET_UTF8 = ";charset=UTF-8";
    private static final String CHARSET_UTF16 = ";charset=UTF-16";

    /**
     * Converts mojo NfcMessage to android.nfc.NdefMessage
     */
    public static NdefMessage toNdefMessage(NfcMessage message) throws InvalidNfcMessageException {
        try {
            List<NdefRecord> records = new ArrayList<NdefRecord>();
            for (int i = 0; i < message.data.length; ++i) {
                records.add(toNdefRecord(message.data[i]));
            }
            records.add(NdefRecord.createExternal(
                    DOMAIN, TYPE, ApiCompatibilityUtils.getBytesUtf8(message.url)));
            NdefRecord[] ndefRecords = new NdefRecord[records.size()];
            records.toArray(ndefRecords);
            return new NdefMessage(ndefRecords);
        } catch (UnsupportedEncodingException | InvalidNfcMessageException
                | IllegalArgumentException e) {
            throw new InvalidNfcMessageException();
        }
    }

    /**
     * Converts android.nfc.NdefMessage to mojo NfcMessage
     */
    public static NfcMessage toNfcMessage(NdefMessage ndefMessage)
            throws UnsupportedEncodingException {
        NdefRecord[] ndefRecords = ndefMessage.getRecords();
        NfcMessage nfcMessage = new NfcMessage();
        List<NfcRecord> nfcRecords = new ArrayList<NfcRecord>();

        for (int i = 0; i < ndefRecords.length; i++) {
            if ((ndefRecords[i].getTnf() == NdefRecord.TNF_EXTERNAL_TYPE)
                    && (Arrays.equals(ndefRecords[i].getType(),
                               ApiCompatibilityUtils.getBytesUtf8(WEBNFC_URN)))) {
                nfcMessage.url = new String(ndefRecords[i].getPayload(), "UTF-8");
                continue;
            }

            NfcRecord nfcRecord = toNfcRecord(ndefRecords[i]);
            if (nfcRecord != null) nfcRecords.add(nfcRecord);
        }

        nfcMessage.data = new NfcRecord[nfcRecords.size()];
        nfcRecords.toArray(nfcMessage.data);
        return nfcMessage;
    }

    /**
     * Returns charset of mojo NfcRecord. Only applicable for URL and TEXT records.
     * If charset cannot be determined, UTF-8 charset is used by default.
     */
    private static String getCharset(NfcRecord record) {
        if (record.mediaType.endsWith(CHARSET_UTF8)) return "UTF-8";

        // When 16bit WTF::String data is converted to bytearray, it is in LE byte order, without
        // BOM. By default, Android interprets UTF-16 charset without BOM as UTF-16BE, thus, use
        // UTF-16LE as encoding for text data.

        if (record.mediaType.endsWith(CHARSET_UTF16)) return "UTF-16LE";

        Log.w(TAG, "Unknown charset, defaulting to UTF-8.");
        return "UTF-8";
    }

    /**
     * Converts mojo NfcRecord to android.nfc.NdefRecord
     */
    private static NdefRecord toNdefRecord(NfcRecord record) throws InvalidNfcMessageException,
                                                                    IllegalArgumentException,
                                                                    UnsupportedEncodingException {
        switch (record.recordType) {
            case NfcRecordType.URL:
                return NdefRecord.createUri(new String(record.data, getCharset(record)));
            case NfcRecordType.TEXT:
                if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.LOLLIPOP) {
                    return NdefRecord.createTextRecord(
                            "en-US", new String(record.data, getCharset(record)));
                } else {
                    return NdefRecord.createMime(TEXT_MIME, record.data);
                }
            case NfcRecordType.JSON:
            case NfcRecordType.OPAQUE_RECORD:
                return NdefRecord.createMime(record.mediaType, record.data);
            case NfcRecordType.EMPTY:
                return new NdefRecord(NdefRecord.TNF_EMPTY, null, null, null);
            default:
                throw new InvalidNfcMessageException();
        }
    }

    /**
     * Converts android.nfc.NdefRecord to mojo NfcRecord
     */
    private static NfcRecord toNfcRecord(NdefRecord ndefRecord)
            throws UnsupportedEncodingException {
        switch (ndefRecord.getTnf()) {
            case NdefRecord.TNF_EMPTY:
                return createEmptyRecord();
            case NdefRecord.TNF_MIME_MEDIA:
                return createMIMERecord(
                        new String(ndefRecord.getType(), "UTF-8"), ndefRecord.getPayload());
            case NdefRecord.TNF_ABSOLUTE_URI:
                return createURLRecord(ndefRecord.toUri());
            case NdefRecord.TNF_WELL_KNOWN:
                return createWellKnownRecord(ndefRecord);
        }
        return null;
    }

    /**
     * Constructs empty NdefMessage
     */
    public static NdefMessage emptyNdefMessage() {
        return new NdefMessage(new NdefRecord(NdefRecord.TNF_EMPTY, null, null, null));
    }

    /**
     * Constructs empty NfcRecord
     */
    private static NfcRecord createEmptyRecord() {
        NfcRecord nfcRecord = new NfcRecord();
        nfcRecord.recordType = NfcRecordType.EMPTY;
        nfcRecord.mediaType = "";
        nfcRecord.data = new byte[0];
        return nfcRecord;
    }

    /**
     * Constructs URL NfcRecord
     */
    private static NfcRecord createURLRecord(Uri uri) {
        if (uri == null) return null;
        NfcRecord nfcRecord = new NfcRecord();
        nfcRecord.recordType = NfcRecordType.URL;
        nfcRecord.mediaType = TEXT_MIME;
        nfcRecord.data = ApiCompatibilityUtils.getBytesUtf8(uri.toString());
        return nfcRecord;
    }

    /**
     * Constructs MIME or JSON NfcRecord
     */
    private static NfcRecord createMIMERecord(String mediaType, byte[] payload) {
        NfcRecord nfcRecord = new NfcRecord();
        if (mediaType.equals(JSON_MIME)) {
            nfcRecord.recordType = NfcRecordType.JSON;
        } else {
            nfcRecord.recordType = NfcRecordType.OPAQUE_RECORD;
        }
        nfcRecord.mediaType = mediaType;
        nfcRecord.data = payload;
        return nfcRecord;
    }

    /**
     * Constructs TEXT NfcRecord
     */
    private static NfcRecord createTextRecord(byte[] text) {
        // Check that text byte array is not empty.
        if (text.length == 0) {
            return null;
        }

        NfcRecord nfcRecord = new NfcRecord();
        nfcRecord.recordType = NfcRecordType.TEXT;
        nfcRecord.mediaType = TEXT_MIME;
        // According to NFCForum-TS-RTD_Text_1.0 specification, section 3.2.1 Syntax.
        // First byte of the payload is status byte, defined in Table 3: Status Byte Encodings.
        // 0-5: lang code length
        // 6  : must be zero
        // 8  : 0 - text is in UTF-8 encoding, 1 - text is in UTF-16 encoding.
        int langCodeLength = (text[0] & (byte) 0x3F);
        int textBodyStartPos = langCodeLength + 1;
        if (textBodyStartPos > text.length) {
            return null;
        }
        nfcRecord.data = Arrays.copyOfRange(text, textBodyStartPos, text.length);
        return nfcRecord;
    }

    /**
     * Constructs well known type (TEXT or URI) NfcRecord
     */
    private static NfcRecord createWellKnownRecord(NdefRecord record) {
        if (Arrays.equals(record.getType(), NdefRecord.RTD_URI)) {
            return createURLRecord(record.toUri());
        }

        if (Arrays.equals(record.getType(), NdefRecord.RTD_TEXT)) {
            return createTextRecord(record.getPayload());
        }

        return null;
    }
}
