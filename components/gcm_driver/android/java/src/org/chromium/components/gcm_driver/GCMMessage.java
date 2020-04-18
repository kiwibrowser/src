// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.components.gcm_driver;

import android.annotation.TargetApi;
import android.os.Build;
import android.os.Bundle;

import java.nio.charset.StandardCharsets;
import java.util.ArrayList;
import java.util.List;

import javax.annotation.Nullable;

/**
 * Represents the contents of a GCM Message that is to be handled by the GCM Driver. Can be created
 * based on data received from GCM, or serialized and deserialized to and from a Bundle.
 */
public class GCMMessage {
    /**
     * Keys used to store information in the bundle for serialization purposes.
     */
    private static final String BUNDLE_KEY_APP_ID = "appId";
    private static final String BUNDLE_KEY_COLLAPSE_KEY = "collapseKey";
    private static final String BUNDLE_KEY_DATA = "data";
    private static final String BUNDLE_KEY_RAW_DATA = "rawData";
    private static final String BUNDLE_KEY_SENDER_ID = "senderId";

    private final String mSenderId;
    private final String mAppId;

    @Nullable
    private final String mCollapseKey;
    @Nullable
    private final byte[] mRawData;

    /**
     * Array that contains pairs of entries in the format of {key, value}.
     */
    private final String[] mDataKeysAndValuesArray;

    /**
     * Validates that all required fields have been set in the given bundle.
     */
    @TargetApi(Build.VERSION_CODES.LOLLIPOP)
    public static boolean validateBundle(Bundle bundle) {
        return bundle.containsKey(BUNDLE_KEY_APP_ID) && bundle.containsKey(BUNDLE_KEY_COLLAPSE_KEY)
                && bundle.containsKey(BUNDLE_KEY_DATA) && bundle.containsKey(BUNDLE_KEY_RAW_DATA)
                && bundle.containsKey(BUNDLE_KEY_SENDER_ID);
    }

    /**
     * Creates a GCMMessage object based on data received from GCM. The extras will be filtered.
     */
    public GCMMessage(String senderId, Bundle extras) {
        String bundleCollapseKey = "collapse_key";
        String bundleGcmplex = "com.google.ipc.invalidation.gcmmplex.";
        String bundleRawData = "rawData";
        String bundleSenderId = "from";
        String bundleSubtype = "subtype";

        if (!extras.containsKey(bundleSubtype)) {
            throw new IllegalArgumentException("Received push message with no subtype");
        }

        mSenderId = senderId;
        mAppId = extras.getString(bundleSubtype);

        mCollapseKey = extras.getString(bundleCollapseKey); // May be null.
        mRawData = extras.getByteArray(bundleRawData); // May be null.

        List<String> dataKeysAndValues = new ArrayList<String>();
        for (String key : extras.keySet()) {
            if (key.equals(bundleSubtype) || key.equals(bundleSenderId)
                    || key.equals(bundleCollapseKey) || key.equals(bundleRawData)
                    || key.startsWith(bundleGcmplex)) {
                continue;
            }

            Object value = extras.get(key);
            if (!(value instanceof String)) {
                continue;
            }

            dataKeysAndValues.add(key);
            dataKeysAndValues.add((String) value);
        }

        mDataKeysAndValuesArray = dataKeysAndValues.toArray(new String[dataKeysAndValues.size()]);
    }

    /**
     * Creates a GCMMessage object based on the given bundle. Assumes that the bundle has previously
     * been created through {@link #toBundle}.
     */
    @TargetApi(Build.VERSION_CODES.LOLLIPOP)
    public GCMMessage(Bundle bundle) {
        mSenderId = bundle.getString(BUNDLE_KEY_SENDER_ID);
        mAppId = bundle.getString(BUNDLE_KEY_APP_ID);
        mCollapseKey = bundle.getString(BUNDLE_KEY_COLLAPSE_KEY);
        // The rawData field needs to distinguish between {not set, set but empty, set with data}.
        String rawDataString = bundle.getString(BUNDLE_KEY_RAW_DATA);
        if (rawDataString != null) {
            if (rawDataString.length() > 0) {
                mRawData = rawDataString.getBytes(StandardCharsets.ISO_8859_1);
            } else {
                mRawData = new byte[0];
            }
        } else {
            mRawData = null;
        }

        mDataKeysAndValuesArray = bundle.getStringArray(BUNDLE_KEY_DATA);
    }

    public String getSenderId() {
        return mSenderId;
    }

    public String getAppId() {
        return mAppId;
    }

    @Nullable
    public String getCollapseKey() {
        return mCollapseKey;
    }

    /**
     * Callers are expected to not modify values in the returned byte array.
     */
    @Nullable
    public byte[] getRawData() {
        return mRawData;
    }

    /**
     * Callers are expected to not modify values in the returned byte array.
     */
    public String[] getDataKeysAndValuesArray() {
        return mDataKeysAndValuesArray;
    }

    /**
     * Serializes the contents of this GCM Message to a new bundle that can be stored, for example
     * for purposes of scheduling a job. Only methods available in BaseBundle may be used here,
     * as it may have to be converted to a PersistableBundle.
     */
    @TargetApi(Build.VERSION_CODES.LOLLIPOP)
    public Bundle toBundle() {
        Bundle bundle = new Bundle();
        bundle.putString(BUNDLE_KEY_SENDER_ID, mSenderId);
        bundle.putString(BUNDLE_KEY_APP_ID, mAppId);
        bundle.putString(BUNDLE_KEY_COLLAPSE_KEY, mCollapseKey);

        // The rawData field needs to distinguish between {not set, set but empty, set with data}.
        if (mRawData != null) {
            if (mRawData.length > 0) {
                bundle.putString(
                        BUNDLE_KEY_RAW_DATA, new String(mRawData, StandardCharsets.ISO_8859_1));
            } else {
                bundle.putString(BUNDLE_KEY_RAW_DATA, "");
            }
        } else {
            bundle.putString(BUNDLE_KEY_RAW_DATA, null);
        }

        bundle.putStringArray(BUNDLE_KEY_DATA, mDataKeysAndValuesArray);
        return bundle;
    }
}
