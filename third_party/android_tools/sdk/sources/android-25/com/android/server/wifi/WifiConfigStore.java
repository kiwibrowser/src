/*
 * Copyright (C) 2016 The Android Open Source Project
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

import android.content.Context;
import android.net.IpConfiguration.IpAssignment;
import android.net.IpConfiguration.ProxySettings;
import android.net.wifi.WifiConfiguration;
import android.net.wifi.WifiConfiguration.Status;
import android.net.wifi.WifiEnterpriseConfig;
import android.net.wifi.WifiSsid;
import android.net.wifi.WpsInfo;
import android.net.wifi.WpsResult;
import android.os.FileObserver;
import android.os.Process;
import android.security.Credentials;
import android.security.KeyChain;
import android.security.KeyStore;
import android.text.TextUtils;
import android.util.ArraySet;
import android.util.LocalLog;
import android.util.Log;
import android.util.SparseArray;

import com.android.server.wifi.hotspot2.Utils;
import com.android.server.wifi.util.TelephonyUtil;

import org.json.JSONException;
import org.json.JSONObject;

import java.io.BufferedReader;
import java.io.File;
import java.io.FileNotFoundException;
import java.io.FileReader;
import java.io.IOException;
import java.net.URLDecoder;
import java.nio.charset.StandardCharsets;
import java.security.PrivateKey;
import java.security.cert.Certificate;
import java.security.cert.CertificateException;
import java.security.cert.X509Certificate;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.BitSet;
import java.util.Collection;
import java.util.HashMap;
import java.util.HashSet;
import java.util.List;
import java.util.Map;
import java.util.Set;

/**
 * This class provides the API's to save/load/modify network configurations from a persistent
 * config database.
 * We use wpa_supplicant as our config database currently, but will be migrating to a different
 * one sometime in the future.
 * We use keystore for certificate/key management operations.
 *
 * NOTE: This class should only be used from WifiConfigManager!!!
 */
public class WifiConfigStore {

    public static final String TAG = "WifiConfigStore";
    // This is the only variable whose contents will not be interpreted by wpa_supplicant. We use it
    // to store metadata that allows us to correlate a wpa_supplicant.conf entry with additional
    // information about the same network stored in other files. The metadata is stored as a
    // serialized JSON dictionary.
    public static final String ID_STRING_VAR_NAME = "id_str";
    public static final String ID_STRING_KEY_FQDN = "fqdn";
    public static final String ID_STRING_KEY_CREATOR_UID = "creatorUid";
    public static final String ID_STRING_KEY_CONFIG_KEY = "configKey";
    public static final String SUPPLICANT_CONFIG_FILE = "/data/misc/wifi/wpa_supplicant.conf";
    public static final String SUPPLICANT_CONFIG_FILE_BACKUP = SUPPLICANT_CONFIG_FILE + ".tmp";

    // Value stored by supplicant to requirePMF
    public static final int STORED_VALUE_FOR_REQUIRE_PMF = 2;

    private static final boolean DBG = true;
    private static boolean VDBG = false;

    private final LocalLog mLocalLog;
    private final WpaConfigFileObserver mFileObserver;
    private final Context mContext;
    private final WifiNative mWifiNative;
    private final KeyStore mKeyStore;
    private final boolean mShowNetworks;
    private final HashSet<String> mBssidBlacklist = new HashSet<String>();

    private final BackupManagerProxy mBackupManagerProxy;

    WifiConfigStore(Context context, WifiNative wifiNative, KeyStore keyStore, LocalLog localLog,
            boolean showNetworks, boolean verboseDebug) {
        mContext = context;
        mWifiNative = wifiNative;
        mKeyStore = keyStore;
        mShowNetworks = showNetworks;
        mBackupManagerProxy = new BackupManagerProxy();

        if (mShowNetworks) {
            mLocalLog = localLog;
            mFileObserver = new WpaConfigFileObserver();
            mFileObserver.startWatching();
        } else {
            mLocalLog = null;
            mFileObserver = null;
        }
        VDBG = verboseDebug;
    }

    private static String removeDoubleQuotes(String string) {
        int length = string.length();
        if ((length > 1) && (string.charAt(0) == '"')
                && (string.charAt(length - 1) == '"')) {
            return string.substring(1, length - 1);
        }
        return string;
    }

    /**
     * Generate a string to be used as a key value by wpa_supplicant from
     * 'set', within the set of strings from 'strings' for the variable concatenated.
     * Also transform the internal string format that uses _ (for bewildering
     * reasons) into a wpa_supplicant adjusted value, that uses - as a separator
     * (most of the time at least...).
     * @param set a bit set with a one for each corresponding string to be included from strings.
     * @param strings the set of string literals to concatenate strinfs from.
     * @return A wpa_supplicant formatted value.
     */
    private static String makeString(BitSet set, String[] strings) {
        return makeStringWithException(set, strings, null);
    }

    /**
     * Same as makeString with an exclusion parameter.
     * @param set a bit set with a one for each corresponding string to be included from strings.
     * @param strings the set of string literals to concatenate strinfs from.
     * @param exception literal string to be excluded from the _ to - transformation.
     * @return A wpa_supplicant formatted value.
     */
    private static String makeStringWithException(BitSet set, String[] strings, String exception) {
        StringBuilder result = new StringBuilder();

        /* Make sure all set bits are in [0, strings.length) to avoid
         * going out of bounds on strings.  (Shouldn't happen, but...) */
        BitSet trimmedSet = set.get(0, strings.length);

        List<String> valueSet = new ArrayList<>();
        for (int bit = trimmedSet.nextSetBit(0);
             bit >= 0;
             bit = trimmedSet.nextSetBit(bit+1)) {
            String currentName = strings[bit];
            if (exception != null && currentName.equals(exception)) {
                valueSet.add(currentName);
            } else {
                // Most wpa_supplicant strings use a dash whereas (for some bizarre
                // reason) the strings are defined with underscore in the code...
                valueSet.add(currentName.replace('_', '-'));
            }
        }
        return TextUtils.join(" ", valueSet);
    }

    /*
     * Convert string to Hexadecimal before passing to wifi native layer
     * In native function "doCommand()" have trouble in converting Unicode character string to UTF8
     * conversion to hex is required because SSIDs can have space characters in them;
     * and that can confuses the supplicant because it uses space charaters as delimiters
     */
    private static String encodeSSID(String str) {
        return Utils.toHex(removeDoubleQuotes(str).getBytes(StandardCharsets.UTF_8));
    }

    // Certificate and private key management for EnterpriseConfig
    private static boolean needsKeyStore(WifiEnterpriseConfig config) {
        return (!(config.getClientCertificate() == null && config.getCaCertificate() == null));
    }

    private static boolean isHardwareBackedKey(PrivateKey key) {
        return KeyChain.isBoundKeyAlgorithm(key.getAlgorithm());
    }

    private static boolean hasHardwareBackedKey(Certificate certificate) {
        return KeyChain.isBoundKeyAlgorithm(certificate.getPublicKey().getAlgorithm());
    }

    private static boolean needsSoftwareBackedKeyStore(WifiEnterpriseConfig config) {
        java.lang.String client = config.getClientCertificateAlias();
        if (!TextUtils.isEmpty(client)) {
            // a valid client certificate is configured

            // BUGBUG: keyStore.get() never returns certBytes; because it is not
            // taking WIFI_UID as a parameter. It always looks for certificate
            // with SYSTEM_UID, and never finds any Wifi certificates. Assuming that
            // all certificates need software keystore until we get the get() API
            // fixed.
            return true;
        }
        return false;
    }

    private int lookupString(String string, String[] strings) {
        int size = strings.length;

        string = string.replace('-', '_');

        for (int i = 0; i < size; i++) {
            if (string.equals(strings[i])) {
                return i;
            }
        }
        loge("Failed to look-up a string: " + string);
        return -1;
    }

    private void readNetworkBitsetVariable(int netId, BitSet variable, String varName,
            String[] strings) {
        String value = mWifiNative.getNetworkVariable(netId, varName);
        if (!TextUtils.isEmpty(value)) {
            variable.clear();
            String[] vals = value.split(" ");
            for (String val : vals) {
                int index = lookupString(val, strings);
                if (0 <= index) {
                    variable.set(index);
                }
            }
        }
    }

    /**
     * Read the variables from the supplicant daemon that are needed to
     * fill in the WifiConfiguration object.
     *
     * @param config the {@link WifiConfiguration} object to be filled in.
     */
    public void readNetworkVariables(WifiConfiguration config) {
        if (config == null) {
            return;
        }
        if (VDBG) localLog("readNetworkVariables: " + config.networkId);
        int netId = config.networkId;
        if (netId < 0) {
            return;
        }
        /*
         * TODO: maybe should have a native method that takes an array of
         * variable names and returns an array of values. But we'd still
         * be doing a round trip to the supplicant daemon for each variable.
         */
        String value;

        value = mWifiNative.getNetworkVariable(netId, WifiConfiguration.ssidVarName);
        if (!TextUtils.isEmpty(value)) {
            if (value.charAt(0) != '"') {
                config.SSID = "\"" + WifiSsid.createFromHex(value).toString() + "\"";
                //TODO: convert a hex string that is not UTF-8 decodable to a P-formatted
                //supplicant string
            } else {
                config.SSID = value;
            }
        } else {
            config.SSID = null;
        }

        value = mWifiNative.getNetworkVariable(netId, WifiConfiguration.bssidVarName);
        if (!TextUtils.isEmpty(value)) {
            config.getNetworkSelectionStatus().setNetworkSelectionBSSID(value);
        } else {
            config.getNetworkSelectionStatus().setNetworkSelectionBSSID(null);
        }

        value = mWifiNative.getNetworkVariable(netId, WifiConfiguration.priorityVarName);
        config.priority = -1;
        if (!TextUtils.isEmpty(value)) {
            try {
                config.priority = Integer.parseInt(value);
            } catch (NumberFormatException ignore) {
            }
        }

        value = mWifiNative.getNetworkVariable(netId, WifiConfiguration.hiddenSSIDVarName);
        config.hiddenSSID = false;
        if (!TextUtils.isEmpty(value)) {
            try {
                config.hiddenSSID = Integer.parseInt(value) != 0;
            } catch (NumberFormatException ignore) {
            }
        }

        value = mWifiNative.getNetworkVariable(netId, WifiConfiguration.pmfVarName);
        config.requirePMF = false;
        if (!TextUtils.isEmpty(value)) {
            try {
                config.requirePMF = Integer.parseInt(value) == STORED_VALUE_FOR_REQUIRE_PMF;
            } catch (NumberFormatException ignore) {
            }
        }

        value = mWifiNative.getNetworkVariable(netId, WifiConfiguration.wepTxKeyIdxVarName);
        config.wepTxKeyIndex = -1;
        if (!TextUtils.isEmpty(value)) {
            try {
                config.wepTxKeyIndex = Integer.parseInt(value);
            } catch (NumberFormatException ignore) {
            }
        }

        for (int i = 0; i < 4; i++) {
            value = mWifiNative.getNetworkVariable(netId,
                    WifiConfiguration.wepKeyVarNames[i]);
            if (!TextUtils.isEmpty(value)) {
                config.wepKeys[i] = value;
            } else {
                config.wepKeys[i] = null;
            }
        }

        value = mWifiNative.getNetworkVariable(netId, WifiConfiguration.pskVarName);
        if (!TextUtils.isEmpty(value)) {
            config.preSharedKey = value;
        } else {
            config.preSharedKey = null;
        }

        readNetworkBitsetVariable(config.networkId, config.allowedProtocols,
                WifiConfiguration.Protocol.varName, WifiConfiguration.Protocol.strings);

        readNetworkBitsetVariable(config.networkId, config.allowedKeyManagement,
                WifiConfiguration.KeyMgmt.varName, WifiConfiguration.KeyMgmt.strings);

        readNetworkBitsetVariable(config.networkId, config.allowedAuthAlgorithms,
                WifiConfiguration.AuthAlgorithm.varName, WifiConfiguration.AuthAlgorithm.strings);

        readNetworkBitsetVariable(config.networkId, config.allowedPairwiseCiphers,
                WifiConfiguration.PairwiseCipher.varName, WifiConfiguration.PairwiseCipher.strings);

        readNetworkBitsetVariable(config.networkId, config.allowedGroupCiphers,
                WifiConfiguration.GroupCipher.varName, WifiConfiguration.GroupCipher.strings);

        if (config.enterpriseConfig == null) {
            config.enterpriseConfig = new WifiEnterpriseConfig();
        }
        config.enterpriseConfig.loadFromSupplicant(new SupplicantLoader(netId));
    }

    /**
     * Load all the configured networks from wpa_supplicant.
     *
     * @param configs       Map of configuration key to configuration objects corresponding to all
     *                      the networks.
     * @param networkExtras Map of extra configuration parameters stored in wpa_supplicant.conf
     * @return Max priority of all the configs.
     */
    public int loadNetworks(Map<String, WifiConfiguration> configs,
            SparseArray<Map<String, String>> networkExtras) {
        int lastPriority = 0;
        int last_id = -1;
        boolean done = false;
        while (!done) {
            String listStr = mWifiNative.listNetworks(last_id);
            if (listStr == null) {
                return lastPriority;
            }
            String[] lines = listStr.split("\n");
            if (mShowNetworks) {
                localLog("loadNetworks:  ");
                for (String net : lines) {
                    localLog(net);
                }
            }
            // Skip the first line, which is a header
            for (int i = 1; i < lines.length; i++) {
                String[] result = lines[i].split("\t");
                // network-id | ssid | bssid | flags
                WifiConfiguration config = new WifiConfiguration();
                try {
                    config.networkId = Integer.parseInt(result[0]);
                    last_id = config.networkId;
                } catch (NumberFormatException e) {
                    loge("Failed to read network-id '" + result[0] + "'");
                    continue;
                }
                // Ignore the supplicant status, start all networks disabled.
                config.status = WifiConfiguration.Status.DISABLED;
                readNetworkVariables(config);
                // Parse the serialized JSON dictionary in ID_STRING_VAR_NAME once and cache the
                // result for efficiency.
                Map<String, String> extras = mWifiNative.getNetworkExtra(config.networkId,
                        ID_STRING_VAR_NAME);
                if (extras == null) {
                    extras = new HashMap<String, String>();
                    // If ID_STRING_VAR_NAME did not contain a dictionary, assume that it contains
                    // just a quoted FQDN. This is the legacy format that was used in Marshmallow.
                    final String fqdn = Utils.unquote(mWifiNative.getNetworkVariable(
                            config.networkId, ID_STRING_VAR_NAME));
                    if (fqdn != null) {
                        extras.put(ID_STRING_KEY_FQDN, fqdn);
                        config.FQDN = fqdn;
                        // Mark the configuration as a Hotspot 2.0 network.
                        config.providerFriendlyName = "";
                    }
                }
                networkExtras.put(config.networkId, extras);

                if (config.priority > lastPriority) {
                    lastPriority = config.priority;
                }
                config.setIpAssignment(IpAssignment.DHCP);
                config.setProxySettings(ProxySettings.NONE);
                if (!WifiServiceImpl.isValid(config)) {
                    if (mShowNetworks) {
                        localLog("Ignoring network " + config.networkId + " because configuration "
                                + "loaded from wpa_supplicant.conf is not valid.");
                    }
                    continue;
                }
                // The configKey is explicitly stored in wpa_supplicant.conf, because config does
                // not contain sufficient information to compute it at this point.
                String configKey = extras.get(ID_STRING_KEY_CONFIG_KEY);
                if (configKey == null) {
                    // Handle the legacy case where the configKey is not stored in
                    // wpa_supplicant.conf but can be computed straight away.
                    // Force an update of this legacy network configuration by writing
                    // the configKey for this network into wpa_supplicant.conf.
                    configKey = config.configKey();
                    saveNetworkMetadata(config);
                }
                final WifiConfiguration duplicateConfig = configs.put(configKey, config);
                if (duplicateConfig != null) {
                    // The network is already known. Overwrite the duplicate entry.
                    if (mShowNetworks) {
                        localLog("Replacing duplicate network " + duplicateConfig.networkId
                                + " with " + config.networkId + ".");
                    }
                    // This can happen after the user manually connected to an AP and tried to use
                    // WPS to connect the AP later. In this case, the supplicant will create a new
                    // network for the AP although there is an existing network already.
                    mWifiNative.removeNetwork(duplicateConfig.networkId);
                }
            }
            done = (lines.length == 1);
        }
        return lastPriority;
    }

    /**
     * Install keys for given enterprise network.
     *
     * @param existingConfig Existing config corresponding to the network already stored in our
     *                       database. This maybe null if it's a new network.
     * @param config         Config corresponding to the network.
     * @return true if successful, false otherwise.
     */
    private boolean installKeys(WifiEnterpriseConfig existingConfig, WifiEnterpriseConfig config,
            String name) {
        boolean ret = true;
        String privKeyName = Credentials.USER_PRIVATE_KEY + name;
        String userCertName = Credentials.USER_CERTIFICATE + name;
        if (config.getClientCertificate() != null) {
            byte[] privKeyData = config.getClientPrivateKey().getEncoded();
            if (DBG) {
                if (isHardwareBackedKey(config.getClientPrivateKey())) {
                    Log.d(TAG, "importing keys " + name + " in hardware backed store");
                } else {
                    Log.d(TAG, "importing keys " + name + " in software backed store");
                }
            }
            ret = mKeyStore.importKey(privKeyName, privKeyData, Process.WIFI_UID,
                    KeyStore.FLAG_NONE);

            if (!ret) {
                return ret;
            }

            ret = putCertInKeyStore(userCertName, config.getClientCertificate());
            if (!ret) {
                // Remove private key installed
                mKeyStore.delete(privKeyName, Process.WIFI_UID);
                return ret;
            }
        }

        X509Certificate[] caCertificates = config.getCaCertificates();
        Set<String> oldCaCertificatesToRemove = new ArraySet<String>();
        if (existingConfig != null && existingConfig.getCaCertificateAliases() != null) {
            oldCaCertificatesToRemove.addAll(
                    Arrays.asList(existingConfig.getCaCertificateAliases()));
        }
        List<String> caCertificateAliases = null;
        if (caCertificates != null) {
            caCertificateAliases = new ArrayList<String>();
            for (int i = 0; i < caCertificates.length; i++) {
                String alias = caCertificates.length == 1 ? name
                        : String.format("%s_%d", name, i);

                oldCaCertificatesToRemove.remove(alias);
                ret = putCertInKeyStore(Credentials.CA_CERTIFICATE + alias, caCertificates[i]);
                if (!ret) {
                    // Remove client key+cert
                    if (config.getClientCertificate() != null) {
                        mKeyStore.delete(privKeyName, Process.WIFI_UID);
                        mKeyStore.delete(userCertName, Process.WIFI_UID);
                    }
                    // Remove added CA certs.
                    for (String addedAlias : caCertificateAliases) {
                        mKeyStore.delete(Credentials.CA_CERTIFICATE + addedAlias, Process.WIFI_UID);
                    }
                    return ret;
                } else {
                    caCertificateAliases.add(alias);
                }
            }
        }
        // Remove old CA certs.
        for (String oldAlias : oldCaCertificatesToRemove) {
            mKeyStore.delete(Credentials.CA_CERTIFICATE + oldAlias, Process.WIFI_UID);
        }
        // Set alias names
        if (config.getClientCertificate() != null) {
            config.setClientCertificateAlias(name);
            config.resetClientKeyEntry();
        }

        if (caCertificates != null) {
            config.setCaCertificateAliases(
                    caCertificateAliases.toArray(new String[caCertificateAliases.size()]));
            config.resetCaCertificate();
        }
        return ret;
    }

    private boolean putCertInKeyStore(String name, Certificate cert) {
        try {
            byte[] certData = Credentials.convertToPem(cert);
            if (DBG) Log.d(TAG, "putting certificate " + name + " in keystore");
            return mKeyStore.put(name, certData, Process.WIFI_UID, KeyStore.FLAG_NONE);

        } catch (IOException e1) {
            return false;
        } catch (CertificateException e2) {
            return false;
        }
    }

    /**
     * Remove enterprise keys from the network config.
     *
     * @param config Config corresponding to the network.
     */
    private void removeKeys(WifiEnterpriseConfig config) {
        String client = config.getClientCertificateAlias();
        // a valid client certificate is configured
        if (!TextUtils.isEmpty(client)) {
            if (DBG) Log.d(TAG, "removing client private key and user cert");
            mKeyStore.delete(Credentials.USER_PRIVATE_KEY + client, Process.WIFI_UID);
            mKeyStore.delete(Credentials.USER_CERTIFICATE + client, Process.WIFI_UID);
        }

        String[] aliases = config.getCaCertificateAliases();
        // a valid ca certificate is configured
        if (aliases != null) {
            for (String ca : aliases) {
                if (!TextUtils.isEmpty(ca)) {
                    if (DBG) Log.d(TAG, "removing CA cert: " + ca);
                    mKeyStore.delete(Credentials.CA_CERTIFICATE + ca, Process.WIFI_UID);
                }
            }
        }
    }

    /**
     * Update the network metadata info stored in wpa_supplicant network extra field.
     * @param config Config corresponding to the network.
     * @return true if successful, false otherwise.
     */
    public boolean saveNetworkMetadata(WifiConfiguration config) {
        final Map<String, String> metadata = new HashMap<String, String>();
        if (config.isPasspoint()) {
            metadata.put(ID_STRING_KEY_FQDN, config.FQDN);
        }
        metadata.put(ID_STRING_KEY_CONFIG_KEY, config.configKey());
        metadata.put(ID_STRING_KEY_CREATOR_UID, Integer.toString(config.creatorUid));
        if (!mWifiNative.setNetworkExtra(config.networkId, ID_STRING_VAR_NAME, metadata)) {
            loge("failed to set id_str: " + metadata.toString());
            return false;
        }
        return true;
    }

    /**
     * Save an entire network configuration to wpa_supplicant.
     *
     * @param config Config corresponding to the network.
     * @param netId  Net Id of the network.
     * @return true if successful, false otherwise.
     */
    private boolean saveNetwork(WifiConfiguration config, int netId) {
        if (config == null) {
            return false;
        }
        if (VDBG) localLog("saveNetwork: " + netId);
        if (config.SSID != null && !mWifiNative.setNetworkVariable(
                netId,
                WifiConfiguration.ssidVarName,
                encodeSSID(config.SSID))) {
            loge("failed to set SSID: " + config.SSID);
            return false;
        }
        if (!saveNetworkMetadata(config)) {
            return false;
        }
        //set selected BSSID to supplicant
        if (config.getNetworkSelectionStatus().getNetworkSelectionBSSID() != null) {
            String bssid = config.getNetworkSelectionStatus().getNetworkSelectionBSSID();
            if (!mWifiNative.setNetworkVariable(netId, WifiConfiguration.bssidVarName, bssid)) {
                loge("failed to set BSSID: " + bssid);
                return false;
            }
        }
        String allowedKeyManagementString =
                makeString(config.allowedKeyManagement, WifiConfiguration.KeyMgmt.strings);
        if (config.allowedKeyManagement.cardinality() != 0 && !mWifiNative.setNetworkVariable(
                netId,
                WifiConfiguration.KeyMgmt.varName,
                allowedKeyManagementString)) {
            loge("failed to set key_mgmt: " + allowedKeyManagementString);
            return false;
        }
        String allowedProtocolsString =
                makeString(config.allowedProtocols, WifiConfiguration.Protocol.strings);
        if (config.allowedProtocols.cardinality() != 0 && !mWifiNative.setNetworkVariable(
                netId,
                WifiConfiguration.Protocol.varName,
                allowedProtocolsString)) {
            loge("failed to set proto: " + allowedProtocolsString);
            return false;
        }
        String allowedAuthAlgorithmsString =
                makeString(config.allowedAuthAlgorithms,
                        WifiConfiguration.AuthAlgorithm.strings);
        if (config.allowedAuthAlgorithms.cardinality() != 0 && !mWifiNative.setNetworkVariable(
                netId,
                WifiConfiguration.AuthAlgorithm.varName,
                allowedAuthAlgorithmsString)) {
            loge("failed to set auth_alg: " + allowedAuthAlgorithmsString);
            return false;
        }
        String allowedPairwiseCiphersString = makeString(config.allowedPairwiseCiphers,
                WifiConfiguration.PairwiseCipher.strings);
        if (config.allowedPairwiseCiphers.cardinality() != 0 && !mWifiNative.setNetworkVariable(
                netId,
                WifiConfiguration.PairwiseCipher.varName,
                allowedPairwiseCiphersString)) {
            loge("failed to set pairwise: " + allowedPairwiseCiphersString);
            return false;
        }
        // Make sure that the string "GTK_NOT_USED" is /not/ transformed - wpa_supplicant
        // uses this literal value and not the 'dashed' version.
        String allowedGroupCiphersString =
                makeStringWithException(config.allowedGroupCiphers,
                        WifiConfiguration.GroupCipher.strings,
                        WifiConfiguration.GroupCipher
                                .strings[WifiConfiguration.GroupCipher.GTK_NOT_USED]);
        if (config.allowedGroupCiphers.cardinality() != 0 && !mWifiNative.setNetworkVariable(
                netId,
                WifiConfiguration.GroupCipher.varName,
                allowedGroupCiphersString)) {
            loge("failed to set group: " + allowedGroupCiphersString);
            return false;
        }
        // Prevent client screw-up by passing in a WifiConfiguration we gave it
        // by preventing "*" as a key.
        if (config.preSharedKey != null && !config.preSharedKey.equals("*")
                && !mWifiNative.setNetworkVariable(
                netId,
                WifiConfiguration.pskVarName,
                config.preSharedKey)) {
            loge("failed to set psk");
            return false;
        }
        boolean hasSetKey = false;
        if (config.wepKeys != null) {
            for (int i = 0; i < config.wepKeys.length; i++) {
                // Prevent client screw-up by passing in a WifiConfiguration we gave it
                // by preventing "*" as a key.
                if (config.wepKeys[i] != null && !config.wepKeys[i].equals("*")) {
                    if (!mWifiNative.setNetworkVariable(
                            netId,
                            WifiConfiguration.wepKeyVarNames[i],
                            config.wepKeys[i])) {
                        loge("failed to set wep_key" + i + ": " + config.wepKeys[i]);
                        return false;
                    }
                    hasSetKey = true;
                }
            }
        }
        if (hasSetKey) {
            if (!mWifiNative.setNetworkVariable(
                    netId,
                    WifiConfiguration.wepTxKeyIdxVarName,
                    Integer.toString(config.wepTxKeyIndex))) {
                loge("failed to set wep_tx_keyidx: " + config.wepTxKeyIndex);
                return false;
            }
        }
        if (!mWifiNative.setNetworkVariable(
                netId,
                WifiConfiguration.priorityVarName,
                Integer.toString(config.priority))) {
            loge(config.SSID + ": failed to set priority: " + config.priority);
            return false;
        }
        if (config.hiddenSSID && !mWifiNative.setNetworkVariable(
                netId,
                WifiConfiguration.hiddenSSIDVarName,
                Integer.toString(config.hiddenSSID ? 1 : 0))) {
            loge(config.SSID + ": failed to set hiddenSSID: " + config.hiddenSSID);
            return false;
        }
        if (config.requirePMF && !mWifiNative.setNetworkVariable(
                netId,
                WifiConfiguration.pmfVarName,
                Integer.toString(STORED_VALUE_FOR_REQUIRE_PMF))) {
            loge(config.SSID + ": failed to set requirePMF: " + config.requirePMF);
            return false;
        }
        if (config.updateIdentifier != null && !mWifiNative.setNetworkVariable(
                netId,
                WifiConfiguration.updateIdentiferVarName,
                config.updateIdentifier)) {
            loge(config.SSID + ": failed to set updateIdentifier: " + config.updateIdentifier);
            return false;
        }
        return true;
    }

    /**
     * Update/Install keys for given enterprise network.
     *
     * @param config         Config corresponding to the network.
     * @param existingConfig Existing config corresponding to the network already stored in our
     *                       database. This maybe null if it's a new network.
     * @return true if successful, false otherwise.
     */
    private boolean updateNetworkKeys(WifiConfiguration config, WifiConfiguration existingConfig) {
        WifiEnterpriseConfig enterpriseConfig = config.enterpriseConfig;
        if (needsKeyStore(enterpriseConfig)) {
            try {
                /* config passed may include only fields being updated.
                 * In order to generate the key id, fetch uninitialized
                 * fields from the currently tracked configuration
                 */
                String keyId = config.getKeyIdForCredentials(existingConfig);

                if (!installKeys(existingConfig != null
                        ? existingConfig.enterpriseConfig : null, enterpriseConfig, keyId)) {
                    loge(config.SSID + ": failed to install keys");
                    return false;
                }
            } catch (IllegalStateException e) {
                loge(config.SSID + " invalid config for key installation: " + e.getMessage());
                return false;
            }
        }
        if (!enterpriseConfig.saveToSupplicant(
                new SupplicantSaver(config.networkId, config.SSID))) {
            removeKeys(enterpriseConfig);
            return false;
        }
        return true;
    }

    /**
     * Add or update a network configuration to wpa_supplicant.
     *
     * @param config         Config corresponding to the network.
     * @param existingConfig Existing config corresponding to the network saved in our database.
     * @return true if successful, false otherwise.
     */
    public boolean addOrUpdateNetwork(WifiConfiguration config, WifiConfiguration existingConfig) {
        if (config == null) {
            return false;
        }
        if (VDBG) localLog("addOrUpdateNetwork: " + config.networkId);
        int netId = config.networkId;
        boolean newNetwork = false;
        /*
         * If the supplied networkId is INVALID_NETWORK_ID, we create a new empty
         * network configuration. Otherwise, the networkId should
         * refer to an existing configuration.
         */
        if (netId == WifiConfiguration.INVALID_NETWORK_ID) {
            newNetwork = true;
            netId = mWifiNative.addNetwork();
            if (netId < 0) {
                loge("Failed to add a network!");
                return false;
            } else {
                logi("addOrUpdateNetwork created netId=" + netId);
            }
            // Save the new network ID to the config
            config.networkId = netId;
        }
        if (!saveNetwork(config, netId)) {
            if (newNetwork) {
                mWifiNative.removeNetwork(netId);
                loge("Failed to set a network variable, removed network: " + netId);
            }
            return false;
        }
        if (config.enterpriseConfig != null
                && config.enterpriseConfig.getEapMethod() != WifiEnterpriseConfig.Eap.NONE) {
            return updateNetworkKeys(config, existingConfig);
        }
        // Stage the backup of the SettingsProvider package which backs this up
        mBackupManagerProxy.notifyDataChanged();
        return true;
    }

    /**
     * Remove the specified network and save config
     *
     * @param config Config corresponding to the network.
     * @return {@code true} if it succeeds, {@code false} otherwise
     */
    public boolean removeNetwork(WifiConfiguration config) {
        if (config == null) {
            return false;
        }
        if (VDBG) localLog("removeNetwork: " + config.networkId);
        if (!mWifiNative.removeNetwork(config.networkId)) {
            loge("Remove network in wpa_supplicant failed on " + config.networkId);
            return false;
        }
        // Remove any associated keys
        if (config.enterpriseConfig != null) {
            removeKeys(config.enterpriseConfig);
        }
        // Stage the backup of the SettingsProvider package which backs this up
        mBackupManagerProxy.notifyDataChanged();
        return true;
    }

    /**
     * Select a network in wpa_supplicant.
     *
     * @param config Config corresponding to the network.
     * @return true if successful, false otherwise.
     */
    public boolean selectNetwork(WifiConfiguration config, Collection<WifiConfiguration> configs) {
        if (config == null) {
            return false;
        }
        if (VDBG) localLog("selectNetwork: " + config.networkId);
        if (!mWifiNative.selectNetwork(config.networkId)) {
            loge("Select network in wpa_supplicant failed on " + config.networkId);
            return false;
        }
        config.status = Status.ENABLED;
        markAllNetworksDisabledExcept(config.networkId, configs);
        return true;
    }

    /**
     * Disable a network in wpa_supplicant.
     *
     * @param config Config corresponding to the network.
     * @return true if successful, false otherwise.
     */
    boolean disableNetwork(WifiConfiguration config) {
        if (config == null) {
            return false;
        }
        if (VDBG) localLog("disableNetwork: " + config.networkId);
        if (!mWifiNative.disableNetwork(config.networkId)) {
            loge("Disable network in wpa_supplicant failed on " + config.networkId);
            return false;
        }
        config.status = Status.DISABLED;
        return true;
    }

    /**
     * Set priority for a network in wpa_supplicant.
     *
     * @param config Config corresponding to the network.
     * @return true if successful, false otherwise.
     */
    public boolean setNetworkPriority(WifiConfiguration config, int priority) {
        if (config == null) {
            return false;
        }
        if (VDBG) localLog("setNetworkPriority: " + config.networkId);
        if (!mWifiNative.setNetworkVariable(config.networkId,
                WifiConfiguration.priorityVarName, Integer.toString(priority))) {
            loge("Set priority of network in wpa_supplicant failed on " + config.networkId);
            return false;
        }
        config.priority = priority;
        return true;
    }

    /**
     * Set SSID for a network in wpa_supplicant.
     *
     * @param config Config corresponding to the network.
     * @return true if successful, false otherwise.
     */
    public boolean setNetworkSSID(WifiConfiguration config, String ssid) {
        if (config == null) {
            return false;
        }
        if (VDBG) localLog("setNetworkSSID: " + config.networkId);
        if (!mWifiNative.setNetworkVariable(config.networkId, WifiConfiguration.ssidVarName,
                encodeSSID(ssid))) {
            loge("Set SSID of network in wpa_supplicant failed on " + config.networkId);
            return false;
        }
        config.SSID = ssid;
        return true;
    }

    /**
     * Set BSSID for a network in wpa_supplicant from network selection.
     *
     * @param config Config corresponding to the network.
     * @param bssid  BSSID to be set.
     * @return true if successful, false otherwise.
     */
    public boolean setNetworkBSSID(WifiConfiguration config, String bssid) {
        // Sanity check the config is valid
        if (config == null
                || (config.networkId == WifiConfiguration.INVALID_NETWORK_ID
                && config.SSID == null)) {
            return false;
        }
        if (VDBG) localLog("setNetworkBSSID: " + config.networkId);
        if (!mWifiNative.setNetworkVariable(config.networkId, WifiConfiguration.bssidVarName,
                bssid)) {
            loge("Set BSSID of network in wpa_supplicant failed on " + config.networkId);
            return false;
        }
        config.getNetworkSelectionStatus().setNetworkSelectionBSSID(bssid);
        return true;
    }

    /**
     * Enable/Disable HS20 parameter in wpa_supplicant.
     *
     * @param enable Enable/Disable the parameter.
     */
    public void enableHS20(boolean enable) {
        mWifiNative.setHs20(enable);
    }

    /**
     * Disables all the networks in the provided list in wpa_supplicant.
     *
     * @param configs Collection of configs which needs to be enabled.
     * @return true if successful, false otherwise.
     */
    public boolean disableAllNetworks(Collection<WifiConfiguration> configs) {
        if (VDBG) localLog("disableAllNetworks");
        boolean networkDisabled = false;
        for (WifiConfiguration enabled : configs) {
            if (disableNetwork(enabled)) {
                networkDisabled = true;
            }
        }
        saveConfig();
        return networkDisabled;
    }

    /**
     * Save the current configuration to wpa_supplicant.conf.
     */
    public boolean saveConfig() {
        return mWifiNative.saveConfig();
    }

    /**
     * Read network variables from wpa_supplicant.conf.
     *
     * @param key The parameter to be parsed.
     * @return Map of corresponding configKey to the value of the param requested.
     */
    public Map<String, String> readNetworkVariablesFromSupplicantFile(String key) {
        Map<String, String> result = new HashMap<>();
        BufferedReader reader = null;
        try {
            reader = new BufferedReader(new FileReader(SUPPLICANT_CONFIG_FILE));
            result = readNetworkVariablesFromReader(reader, key);
        } catch (FileNotFoundException e) {
            if (VDBG) loge("Could not open " + SUPPLICANT_CONFIG_FILE + ", " + e);
        } catch (IOException e) {
            if (VDBG) loge("Could not read " + SUPPLICANT_CONFIG_FILE + ", " + e);
        } finally {
            try {
                if (reader != null) {
                    reader.close();
                }
            } catch (IOException e) {
                if (VDBG) {
                    loge("Could not close reader for " + SUPPLICANT_CONFIG_FILE + ", " + e);
                }
            }
        }
        return result;
    }

    /**
     * Read network variables from a given reader. This method is separate from
     * readNetworkVariablesFromSupplicantFile() for testing.
     *
     * @param reader The reader to read the network variables from.
     * @param key The parameter to be parsed.
     * @return Map of corresponding configKey to the value of the param requested.
     */
    public Map<String, String> readNetworkVariablesFromReader(BufferedReader reader, String key)
            throws IOException {
        Map<String, String> result = new HashMap<>();
        if (VDBG) localLog("readNetworkVariablesFromReader key=" + key);
        boolean found = false;
        String configKey = null;
        String value = null;
        for (String line = reader.readLine(); line != null; line = reader.readLine()) {
            if (line.matches("[ \\t]*network=\\{")) {
                found = true;
                configKey = null;
                value = null;
            } else if (line.matches("[ \\t]*\\}")) {
                found = false;
                configKey = null;
                value = null;
            }
            if (found) {
                String trimmedLine = line.trim();
                if (trimmedLine.startsWith(ID_STRING_VAR_NAME + "=")) {
                    try {
                        // Trim the quotes wrapping the id_str value.
                        final String encodedExtras = trimmedLine.substring(
                                8, trimmedLine.length() -1);
                        final JSONObject json =
                                new JSONObject(URLDecoder.decode(encodedExtras, "UTF-8"));
                        if (json.has(WifiConfigStore.ID_STRING_KEY_CONFIG_KEY)) {
                            final Object configKeyFromJson =
                                    json.get(WifiConfigStore.ID_STRING_KEY_CONFIG_KEY);
                            if (configKeyFromJson instanceof String) {
                                configKey = (String) configKeyFromJson;
                            }
                        }
                    } catch (JSONException e) {
                        if (VDBG) {
                            loge("Could not get "+ WifiConfigStore.ID_STRING_KEY_CONFIG_KEY
                                    + ", " + e);
                        }
                    }
                }
                if (trimmedLine.startsWith(key + "=")) {
                    value = trimmedLine.substring(key.length() + 1);
                }
                if (configKey != null && value != null) {
                    result.put(configKey, value);
                }
            }
        }
        return result;
    }

    /**
     * Resets all sim networks from the provided network list.
     *
     * @param configs List of all the networks.
     */
    public void resetSimNetworks(Collection<WifiConfiguration> configs) {
        if (VDBG) localLog("resetSimNetworks");
        for (WifiConfiguration config : configs) {
            if (TelephonyUtil.isSimConfig(config)) {
                String currentIdentity = TelephonyUtil.getSimIdentity(mContext,
                        config.enterpriseConfig.getEapMethod());
                String supplicantIdentity =
                        mWifiNative.getNetworkVariable(config.networkId, "identity");
                if(supplicantIdentity != null) {
                    supplicantIdentity = removeDoubleQuotes(supplicantIdentity);
                }
                if (currentIdentity == null || !currentIdentity.equals(supplicantIdentity)) {
                    // Identity differs so update the identity
                    mWifiNative.setNetworkVariable(config.networkId,
                            WifiEnterpriseConfig.IDENTITY_KEY, WifiEnterpriseConfig.EMPTY_VALUE);
                    // This configuration may have cached Pseudonym IDs; lets remove them
                    mWifiNative.setNetworkVariable(config.networkId,
                            WifiEnterpriseConfig.ANON_IDENTITY_KEY,
                            WifiEnterpriseConfig.EMPTY_VALUE);
                }
                // Update the loaded config
                config.enterpriseConfig.setIdentity(currentIdentity);
                config.enterpriseConfig.setAnonymousIdentity("");
            }
        }
    }

    /**
     * Clear BSSID blacklist in wpa_supplicant.
     */
    public void clearBssidBlacklist() {
        if (VDBG) localLog("clearBlacklist");
        mBssidBlacklist.clear();
        mWifiNative.clearBlacklist();
        mWifiNative.setBssidBlacklist(null);
    }

    /**
     * Add a BSSID to the blacklist.
     *
     * @param bssid bssid to be added.
     */
    public void blackListBssid(String bssid) {
        if (bssid == null) {
            return;
        }
        if (VDBG) localLog("blackListBssid: " + bssid);
        mBssidBlacklist.add(bssid);
        // Blacklist at wpa_supplicant
        mWifiNative.addToBlacklist(bssid);
        // Blacklist at firmware
        String[] list = mBssidBlacklist.toArray(new String[mBssidBlacklist.size()]);
        mWifiNative.setBssidBlacklist(list);
    }

    /**
     * Checks if the provided bssid is blacklisted or not.
     *
     * @param bssid bssid to be checked.
     * @return true if present, false otherwise.
     */
    public boolean isBssidBlacklisted(String bssid) {
        return mBssidBlacklist.contains(bssid);
    }

    /* Mark all networks except specified netId as disabled */
    private void markAllNetworksDisabledExcept(int netId, Collection<WifiConfiguration> configs) {
        for (WifiConfiguration config : configs) {
            if (config != null && config.networkId != netId) {
                if (config.status != Status.DISABLED) {
                    config.status = Status.DISABLED;
                }
            }
        }
    }

    private void markAllNetworksDisabled(Collection<WifiConfiguration> configs) {
        markAllNetworksDisabledExcept(WifiConfiguration.INVALID_NETWORK_ID, configs);
    }

    /**
     * Start WPS pin method configuration with pin obtained
     * from the access point
     *
     * @param config WPS configuration
     * @return Wps result containing status and pin
     */
    public WpsResult startWpsWithPinFromAccessPoint(WpsInfo config,
            Collection<WifiConfiguration> configs) {
        WpsResult result = new WpsResult();
        if (mWifiNative.startWpsRegistrar(config.BSSID, config.pin)) {
            /* WPS leaves all networks disabled */
            markAllNetworksDisabled(configs);
            result.status = WpsResult.Status.SUCCESS;
        } else {
            loge("Failed to start WPS pin method configuration");
            result.status = WpsResult.Status.FAILURE;
        }
        return result;
    }

    /**
     * Start WPS pin method configuration with obtained
     * from the device
     *
     * @return WpsResult indicating status and pin
     */
    public WpsResult startWpsWithPinFromDevice(WpsInfo config,
            Collection<WifiConfiguration> configs) {
        WpsResult result = new WpsResult();
        result.pin = mWifiNative.startWpsPinDisplay(config.BSSID);
        /* WPS leaves all networks disabled */
        if (!TextUtils.isEmpty(result.pin)) {
            markAllNetworksDisabled(configs);
            result.status = WpsResult.Status.SUCCESS;
        } else {
            loge("Failed to start WPS pin method configuration");
            result.status = WpsResult.Status.FAILURE;
        }
        return result;
    }

    /**
     * Start WPS push button configuration
     *
     * @param config WPS configuration
     * @return WpsResult indicating status and pin
     */
    public WpsResult startWpsPbc(WpsInfo config,
            Collection<WifiConfiguration> configs) {
        WpsResult result = new WpsResult();
        if (mWifiNative.startWpsPbc(config.BSSID)) {
            /* WPS leaves all networks disabled */
            markAllNetworksDisabled(configs);
            result.status = WpsResult.Status.SUCCESS;
        } else {
            loge("Failed to start WPS push button configuration");
            result.status = WpsResult.Status.FAILURE;
        }
        return result;
    }

    protected void logd(String s) {
        Log.d(TAG, s);
    }

    protected void logi(String s) {
        Log.i(TAG, s);
    }

    protected void loge(String s) {
        loge(s, false);
    }

    protected void loge(String s, boolean stack) {
        if (stack) {
            Log.e(TAG, s + " stack:" + Thread.currentThread().getStackTrace()[2].getMethodName()
                    + " - " + Thread.currentThread().getStackTrace()[3].getMethodName()
                    + " - " + Thread.currentThread().getStackTrace()[4].getMethodName()
                    + " - " + Thread.currentThread().getStackTrace()[5].getMethodName());
        } else {
            Log.e(TAG, s);
        }
    }

    protected void log(String s) {
        Log.d(TAG, s);
    }

    private void localLog(String s) {
        if (mLocalLog != null) {
            mLocalLog.log(TAG + ": " + s);
        }
    }

    private void localLogAndLogcat(String s) {
        localLog(s);
        Log.d(TAG, s);
    }

    private class SupplicantSaver implements WifiEnterpriseConfig.SupplicantSaver {
        private final int mNetId;
        private final String mSetterSSID;

        SupplicantSaver(int netId, String setterSSID) {
            mNetId = netId;
            mSetterSSID = setterSSID;
        }

        @Override
        public boolean saveValue(String key, String value) {
            if (key.equals(WifiEnterpriseConfig.PASSWORD_KEY)
                    && value != null && value.equals("*")) {
                // No need to try to set an obfuscated password, which will fail
                return true;
            }
            if (key.equals(WifiEnterpriseConfig.REALM_KEY)
                    || key.equals(WifiEnterpriseConfig.PLMN_KEY)) {
                // No need to save realm or PLMN in supplicant
                return true;
            }
            // TODO: We need a way to clear values in wpa_supplicant as opposed to
            // mapping unset values to empty strings.
            if (value == null) {
                value = "\"\"";
            }
            if (!mWifiNative.setNetworkVariable(mNetId, key, value)) {
                loge(mSetterSSID + ": failed to set " + key + ": " + value);
                return false;
            }
            return true;
        }
    }

    private class SupplicantLoader implements WifiEnterpriseConfig.SupplicantLoader {
        private final int mNetId;

        SupplicantLoader(int netId) {
            mNetId = netId;
        }

        @Override
        public String loadValue(String key) {
            String value = mWifiNative.getNetworkVariable(mNetId, key);
            if (!TextUtils.isEmpty(value)) {
                if (!enterpriseConfigKeyShouldBeQuoted(key)) {
                    value = removeDoubleQuotes(value);
                }
                return value;
            } else {
                return null;
            }
        }

        /**
         * Returns true if a particular config key needs to be quoted when passed to the supplicant.
         */
        private boolean enterpriseConfigKeyShouldBeQuoted(String key) {
            switch (key) {
                case WifiEnterpriseConfig.EAP_KEY:
                case WifiEnterpriseConfig.ENGINE_KEY:
                    return false;
                default:
                    return true;
            }
        }
    }

    // TODO(rpius): Remove this.
    private class WpaConfigFileObserver extends FileObserver {

        WpaConfigFileObserver() {
            super(SUPPLICANT_CONFIG_FILE, CLOSE_WRITE);
        }

        @Override
        public void onEvent(int event, String path) {
            if (event == CLOSE_WRITE) {
                File file = new File(SUPPLICANT_CONFIG_FILE);
                if (VDBG) localLog("wpa_supplicant.conf changed; new size = " + file.length());
            }
        }
    }
}
