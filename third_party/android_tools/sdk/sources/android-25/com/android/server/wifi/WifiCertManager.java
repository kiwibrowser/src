/*
 * Copyright (C) 2015 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package com.android.server.wifi;

import android.app.admin.IDevicePolicyManager;
import android.content.Context;
import android.os.Environment;
import android.os.ServiceManager;
import android.os.UserHandle;
import android.security.Credentials;
import android.security.KeyStore;
import android.text.TextUtils;
import android.util.Log;

import com.android.server.net.DelayedDiskWrite;

import java.io.DataInputStream;
import java.io.DataOutputStream;
import java.io.File;
import java.io.FileInputStream;
import java.io.IOException;
import java.nio.charset.StandardCharsets;
import java.util.Arrays;
import java.util.HashSet;
import java.util.Set;

/**
 * Manager class for affiliated Wifi certificates.
 */
public class WifiCertManager {
    private static final String TAG = "WifiCertManager";
    private static final String SEP = "\n";

    private final Context mContext;
    private final Set<String> mAffiliatedUserOnlyCerts = new HashSet<String>();
    private final String mConfigFile;

    private static final String CONFIG_FILE =
            Environment.getDataDirectory() + "/misc/wifi/affiliatedcerts.txt";

    private final DelayedDiskWrite mWriter = new DelayedDiskWrite();


    WifiCertManager(Context context) {
        this(context, CONFIG_FILE);
    }

    WifiCertManager(Context context, String configFile) {
        mContext = context;
        mConfigFile = configFile;
        final byte[] bytes = readConfigFile();
        if (bytes == null) {
            // Config file does not exist or empty.
            return;
        }

        String[] keys = new String(bytes, StandardCharsets.UTF_8).split(SEP);
        for (String key : keys) {
            mAffiliatedUserOnlyCerts.add(key);
        }

        // Remove keys that no longer exist in KeyStore.
        if (mAffiliatedUserOnlyCerts.retainAll(Arrays.asList(listClientCertsForAllUsers()))) {
            writeConfig();
        }
    }

    /** @param  key Unprefixed cert key to hide from unaffiliated users. */
    public void hideCertFromUnaffiliatedUsers(String key) {
        if (mAffiliatedUserOnlyCerts.add(Credentials.USER_PRIVATE_KEY + key)) {
            writeConfig();
        }
    }

    /** @return Prefixed cert keys that are visible to the current user. */
    public String[] listClientCertsForCurrentUser() {
        HashSet<String> results = new HashSet<String>();

        String[] keys = listClientCertsForAllUsers();
        if (isAffiliatedUser()) {
            return keys;
        }

        for (String key : keys) {
            if (!mAffiliatedUserOnlyCerts.contains(key)) {
                results.add(key);
            }
        }
        return results.toArray(new String[results.size()]);
    }

    private void writeConfig() {
        String[] values =
                mAffiliatedUserOnlyCerts.toArray(new String[mAffiliatedUserOnlyCerts.size()]);
        String value = TextUtils.join(SEP, values);
        writeConfigFile(value.getBytes(StandardCharsets.UTF_8));
    }

    protected byte[] readConfigFile() {
        byte[] bytes = null;
        try {
            final File file = new File(mConfigFile);
            final long fileSize = file.exists() ? file.length() : 0;
            if (fileSize == 0 || fileSize >= Integer.MAX_VALUE) {
                // Config file is empty/corrupted/non-existing.
                return bytes;
            }

            bytes = new byte[(int) file.length()];
            final DataInputStream stream = new DataInputStream(new FileInputStream(file));
            stream.readFully(bytes);
        } catch (IOException e) {
            Log.e(TAG, "readConfigFile: failed to read " + e, e);
        }
        return bytes;
    }

    protected void writeConfigFile(byte[] payload) {
        final byte[] data = payload;
        mWriter.write(mConfigFile, new DelayedDiskWrite.Writer() {
            public void onWriteCalled(DataOutputStream out) throws IOException {
                out.write(data, 0, data.length);
            }
        });
    }

    protected String[] listClientCertsForAllUsers() {
        return KeyStore.getInstance().list(Credentials.USER_PRIVATE_KEY, UserHandle.myUserId());
    }

    protected boolean isAffiliatedUser() {
        IDevicePolicyManager pm = IDevicePolicyManager.Stub.asInterface(
                ServiceManager.getService(Context.DEVICE_POLICY_SERVICE));
        boolean result = false;
        try {
            result = pm.isAffiliatedUser();
        } catch (Exception e) {
            Log.e(TAG, "failed to check user affiliation", e);
        }
        return result;
    }
}
