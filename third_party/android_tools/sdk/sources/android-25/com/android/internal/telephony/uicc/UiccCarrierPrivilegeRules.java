/*
 * Copyright (C) 2014 The Android Open Source Project
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

package com.android.internal.telephony.uicc;

import android.annotation.Nullable;
import android.content.Intent;
import android.content.pm.PackageInfo;
import android.content.pm.PackageManager;
import android.content.pm.ResolveInfo;
import android.content.pm.Signature;
import android.os.AsyncResult;
import android.os.Binder;
import android.os.Handler;
import android.os.Message;
import android.telephony.Rlog;
import android.telephony.TelephonyManager;
import android.text.TextUtils;

import com.android.internal.telephony.CommandException;
import com.android.internal.telephony.CommandsInterface;
import com.android.internal.telephony.uicc.IccUtils;

import java.io.ByteArrayInputStream;
import java.io.FileDescriptor;
import java.io.PrintWriter;
import java.lang.IllegalArgumentException;
import java.lang.IndexOutOfBoundsException;
import java.security.MessageDigest;
import java.security.NoSuchAlgorithmException;
import java.security.cert.Certificate;
import java.security.cert.CertificateException;
import java.security.cert.CertificateFactory;
import java.security.cert.X509Certificate;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;
import java.util.Locale;
import java.util.concurrent.atomic.AtomicInteger;

/**
 * Class that reads and stores the carrier privileged rules from the UICC.
 *
 * The rules are read when the class is created, hence it should only be created
 * after the UICC can be read. And it should be deleted when a UICC is changed.
 *
 * Document: https://source.android.com/devices/tech/config/uicc.html
 *
 * {@hide}
 */
public class UiccCarrierPrivilegeRules extends Handler {
    private static final String LOG_TAG = "UiccCarrierPrivilegeRules";
    private static final boolean DBG = false;

    private static final String AID = "A00000015141434C00";
    private static final int CLA = 0x80;
    private static final int COMMAND = 0xCA;
    private static final int P1 = 0xFF;
    private static final int P2 = 0x40;
    private static final int P2_EXTENDED_DATA = 0x60;
    private static final int P3 = 0x00;
    private static final String DATA = "";

    /*
     * Rules format:
     *   ALL_REF_AR_DO = TAG_ALL_REF_AR_DO + len + [REF_AR_DO]*n
     *   REF_AR_DO = TAG_REF_AR_DO + len + REF-DO + AR-DO
     *
     *   REF_DO = TAG_REF_DO + len + DEVICE_APP_ID_REF_DO + (optional) PKG_REF_DO
     *   AR_DO = TAG_AR_DO + len + PERM_AR_DO
     *
     *   DEVICE_APP_ID_REF_DO = TAG_DEVICE_APP_ID_REF_DO + len + sha256 hexstring of cert
     *   PKG_REF_DO = TAG_PKG_REF_DO + len + package name
     *   PERM_AR_DO = TAG_PERM_AR_DO + len + detailed permission (8 bytes)
     *
     * Data objects hierarchy by TAG:
     * FF40
     *   E2
     *     E1
     *       C1
     *       CA
     *     E3
     *       DB
     */
    // Values from the data standard.
    private static final String TAG_ALL_REF_AR_DO = "FF40";
    private static final String TAG_REF_AR_DO = "E2";
    private static final String TAG_REF_DO = "E1";
    private static final String TAG_DEVICE_APP_ID_REF_DO = "C1";
    private static final String TAG_PKG_REF_DO = "CA";
    private static final String TAG_AR_DO = "E3";
    private static final String TAG_PERM_AR_DO = "DB";

    private static final int EVENT_OPEN_LOGICAL_CHANNEL_DONE = 1;
    private static final int EVENT_TRANSMIT_LOGICAL_CHANNEL_DONE = 2;
    private static final int EVENT_CLOSE_LOGICAL_CHANNEL_DONE = 3;
    private static final int EVENT_PKCS15_READ_DONE = 4;

    // State of the object.
    private static final int STATE_LOADING  = 0;
    private static final int STATE_LOADED   = 1;
    private static final int STATE_ERROR    = 2;

    // Max number of retries for open logical channel, interval is 10s.
    private static final int MAX_RETRY = 1;
    private static final int RETRY_INTERVAL_MS = 10000;

    // Describes a single rule.
    private static class AccessRule {
        public byte[] certificateHash;
        public String packageName;
        public long accessType;   // This bit is not currently used, but reserved for future use.

        AccessRule(byte[] certificateHash, String packageName, long accessType) {
            this.certificateHash = certificateHash;
            this.packageName = packageName;
            this.accessType = accessType;
        }

        boolean matches(byte[] certHash, String packageName) {
          return certHash != null && Arrays.equals(this.certificateHash, certHash) &&
                (TextUtils.isEmpty(this.packageName) || this.packageName.equals(packageName));
        }

        @Override
        public String toString() {
            return "cert: " + IccUtils.bytesToHexString(certificateHash) + " pkg: " +
                packageName + " access: " + accessType;
        }
    }

    // Used for parsing the data from the UICC.
    public static class TLV {
        private static final int SINGLE_BYTE_MAX_LENGTH = 0x80;
        private String tag;
        // Length encoding is in GPC_Specification_2.2.1: 11.1.5 APDU Message and Data Length.
        // Length field could be either 1 byte if length < 128, or multiple bytes with first byte
        // specifying how many bytes are used for length, followed by length bytes.
        // Bytes for the length field, in ASCII HEX string form.
        private String lengthBytes;
        // Decoded length as integer.
        private Integer length;
        private String value;

        public TLV(String tag) {
            this.tag = tag;
        }

        public String getValue() {
            if (value == null) return "";
            return value;
        }

        public String parseLength(String data) {
            int offset = tag.length();
            int firstByte = Integer.parseInt(data.substring(offset, offset + 2), 16);
            if (firstByte < SINGLE_BYTE_MAX_LENGTH) {
                length = firstByte * 2;
                lengthBytes = data.substring(offset, offset + 2);
            } else {
                int numBytes = firstByte - SINGLE_BYTE_MAX_LENGTH;
                length = Integer.parseInt(data.substring(offset + 2, offset + 2 + numBytes * 2), 16) * 2;
                lengthBytes = data.substring(offset, offset + 2 + numBytes * 2);
            }
            log("TLV parseLength length=" + length + "lenghtBytes: " + lengthBytes);
            return lengthBytes;
        }

        public String parse(String data, boolean shouldConsumeAll) {
            log("Parse TLV: " + tag);
            if (!data.startsWith(tag)) {
                throw new IllegalArgumentException("Tags don't match.");
            }
            int index = tag.length();
            if (index + 2 > data.length()) {
                throw new IllegalArgumentException("No length.");
            }

            parseLength(data);
            index += lengthBytes.length();

            log("index="+index+" length="+length+"data.length="+data.length());
            int remainingLength = data.length() - (index + length);
            if (remainingLength < 0) {
                throw new IllegalArgumentException("Not enough data.");
            }
            if (shouldConsumeAll && (remainingLength != 0)) {
                throw new IllegalArgumentException("Did not consume all.");
            }
            value = data.substring(index, index + length);

            log("Got TLV: " + tag + "," + length + "," + value);

            return data.substring(index + length);
        }
    }

    private UiccCard mUiccCard;  // Parent
    private UiccPkcs15 mUiccPkcs15; // ARF fallback
    private AtomicInteger mState;
    private List<AccessRule> mAccessRules;
    private String mRules;
    private Message mLoadedCallback;
    private String mStatusMessage;  // Only used for debugging.
    private int mChannelId; // Channel Id for communicating with UICC.
    private int mRetryCount;  // Number of retries for open logical channel.
    private final Runnable mRetryRunnable = new Runnable() {
        @Override
        public void run() {
            openChannel();
        }
    };

    private void openChannel() {
        // Send open logical channel request.
        mUiccCard.iccOpenLogicalChannel(AID,
            obtainMessage(EVENT_OPEN_LOGICAL_CHANNEL_DONE, null));
    }

    public UiccCarrierPrivilegeRules(UiccCard uiccCard, Message loadedCallback) {
        log("Creating UiccCarrierPrivilegeRules");
        mUiccCard = uiccCard;
        mState = new AtomicInteger(STATE_LOADING);
        mStatusMessage = "Not loaded.";
        mLoadedCallback = loadedCallback;
        mRules = "";
        mAccessRules = new ArrayList<AccessRule>();

        openChannel();
    }

    /**
     * Returns true if the carrier privilege rules have finished loading.
     */
    public boolean areCarrierPriviligeRulesLoaded() {
        return mState.get() != STATE_LOADING;
    }

    /**
     * Returns true if the carrier privilege rules have finished loading and some rules were
     * specified.
     */
    public boolean hasCarrierPrivilegeRules() {
        return mState.get() != STATE_LOADING && mAccessRules != null && mAccessRules.size() > 0;
    }

    /**
     * Returns package names for privilege rules.
     * Return empty list if no rules defined or package name is empty string.
     */
    public List<String> getPackageNames() {
        List<String> pkgNames = new ArrayList<String>();
        if (mAccessRules != null) {
            for (AccessRule ar : mAccessRules) {
                if(!TextUtils.isEmpty(ar.packageName)) {
                    pkgNames.add(ar.packageName);
                }
            }
        }
        return pkgNames;
    }

    /**
     * Returns the status of the carrier privileges for the input certificate and package name.
     *
     * @param signature The signature of the certificate.
     * @param packageName name of the package.
     * @return Access status.
     */
    public int getCarrierPrivilegeStatus(Signature signature, String packageName) {
        int state = mState.get();
        if (state == STATE_LOADING) {
            return TelephonyManager.CARRIER_PRIVILEGE_STATUS_RULES_NOT_LOADED;
        } else if (state == STATE_ERROR) {
            return TelephonyManager.CARRIER_PRIVILEGE_STATUS_ERROR_LOADING_RULES;
        }

        // SHA-1 is for backward compatible support only, strongly discouraged for new use.
        byte[] certHash = getCertHash(signature, "SHA-1");
        byte[] certHash256 = getCertHash(signature, "SHA-256");
        for (AccessRule ar : mAccessRules) {
            if (ar.matches(certHash, packageName) || ar.matches(certHash256, packageName)) {
                return TelephonyManager.CARRIER_PRIVILEGE_STATUS_HAS_ACCESS;
            }
        }

        return TelephonyManager.CARRIER_PRIVILEGE_STATUS_NO_ACCESS;
    }

    /**
     * Returns the status of the carrier privileges for the input package name.
     *
     * @param packageManager PackageManager for getting signatures.
     * @param packageName name of the package.
     * @return Access status.
     */
    public int getCarrierPrivilegeStatus(PackageManager packageManager, String packageName) {
        try {
            // Short-circuit if there are no rules to check against, so we don't need to fetch
            // the package info with signatures.
            if (!hasCarrierPrivilegeRules()) {
                int state = mState.get();
                if (state == STATE_LOADING) {
                    return TelephonyManager.CARRIER_PRIVILEGE_STATUS_RULES_NOT_LOADED;
                } else if (state == STATE_ERROR) {
                    return TelephonyManager.CARRIER_PRIVILEGE_STATUS_ERROR_LOADING_RULES;
                }
                return TelephonyManager.CARRIER_PRIVILEGE_STATUS_NO_ACCESS;
            }
            // Include DISABLED_UNTIL_USED components. This facilitates cases where a carrier app
            // is disabled by default, and some other component wants to enable it when it has
            // gained carrier privileges (as an indication that a matching SIM has been inserted).
            PackageInfo pInfo = packageManager.getPackageInfo(packageName,
                PackageManager.GET_SIGNATURES | PackageManager.GET_DISABLED_UNTIL_USED_COMPONENTS);
            return getCarrierPrivilegeStatus(pInfo);
        } catch (PackageManager.NameNotFoundException ex) {
            Rlog.e(LOG_TAG, "NameNotFoundException", ex);
        }
        return TelephonyManager.CARRIER_PRIVILEGE_STATUS_NO_ACCESS;
    }

    /**
     * Returns the status of the carrier privileges for the input package info.
     *
     * @param packageInfo PackageInfo for the package, containing the package signatures.
     * @return Access status.
     */
    public int getCarrierPrivilegeStatus(PackageInfo packageInfo) {
        Signature[] signatures = packageInfo.signatures;
        for (Signature sig : signatures) {
            int accessStatus = getCarrierPrivilegeStatus(sig, packageInfo.packageName);
            if (accessStatus != TelephonyManager.CARRIER_PRIVILEGE_STATUS_NO_ACCESS) {
                return accessStatus;
            }
        }
        return TelephonyManager.CARRIER_PRIVILEGE_STATUS_NO_ACCESS;
    }

    /**
     * Returns the status of the carrier privileges for the caller of the current transaction.
     *
     * @param packageManager PackageManager for getting signatures and package names.
     * @return Access status.
     */
    public int getCarrierPrivilegeStatusForCurrentTransaction(PackageManager packageManager) {
        String[] packages = packageManager.getPackagesForUid(Binder.getCallingUid());

        for (String pkg : packages) {
            int accessStatus = getCarrierPrivilegeStatus(packageManager, pkg);
            if (accessStatus != TelephonyManager.CARRIER_PRIVILEGE_STATUS_NO_ACCESS) {
                return accessStatus;
            }
        }
        return TelephonyManager.CARRIER_PRIVILEGE_STATUS_NO_ACCESS;
    }

    /**
     * Returns the package name of the carrier app that should handle the input intent.
     *
     * @param packageManager PackageManager for getting receivers.
     * @param intent Intent that will be sent.
     * @return list of carrier app package names that can handle the intent.
     *         Returns null if there is an error and an empty list if there
     *         are no matching packages.
     */
    public List<String> getCarrierPackageNamesForIntent(
            PackageManager packageManager, Intent intent) {
        List<String> packages = new ArrayList<String>();
        List<ResolveInfo> receivers = new ArrayList<ResolveInfo>();
        receivers.addAll(packageManager.queryBroadcastReceivers(intent, 0));
        receivers.addAll(packageManager.queryIntentContentProviders(intent, 0));
        receivers.addAll(packageManager.queryIntentActivities(intent, 0));
        receivers.addAll(packageManager.queryIntentServices(intent, 0));

        for (ResolveInfo resolveInfo : receivers) {
            String packageName = getPackageName(resolveInfo);
            if (packageName == null) {
                continue;
            }

            int status = getCarrierPrivilegeStatus(packageManager, packageName);
            if (status == TelephonyManager.CARRIER_PRIVILEGE_STATUS_HAS_ACCESS) {
                packages.add(packageName);
            } else if (status != TelephonyManager.CARRIER_PRIVILEGE_STATUS_NO_ACCESS) {
                // Any status apart from HAS_ACCESS and NO_ACCESS is considered an error.
                return null;
            }
        }

        return packages;
    }

    @Nullable
    private String getPackageName(ResolveInfo resolveInfo) {
        if (resolveInfo.activityInfo != null) {
            return resolveInfo.activityInfo.packageName;
        } else if (resolveInfo.serviceInfo != null) {
            return resolveInfo.serviceInfo.packageName;
        } else if (resolveInfo.providerInfo != null) {
            return resolveInfo.providerInfo.packageName;
        }
        return null;
    }

    @Override
    public void handleMessage(Message msg) {
        AsyncResult ar;

        switch (msg.what) {

          case EVENT_OPEN_LOGICAL_CHANNEL_DONE:
              log("EVENT_OPEN_LOGICAL_CHANNEL_DONE");
              ar = (AsyncResult) msg.obj;
              if (ar.exception == null && ar.result != null) {
                  mChannelId = ((int[]) ar.result)[0];
                  mUiccCard.iccTransmitApduLogicalChannel(mChannelId, CLA, COMMAND, P1, P2, P3, DATA,
                      obtainMessage(EVENT_TRANSMIT_LOGICAL_CHANNEL_DONE, new Integer(mChannelId)));
              } else {
                  // MISSING_RESOURCE could be due to logical channels temporarily unavailable,
                  // so we retry up to MAX_RETRY times, with an interval of RETRY_INTERVAL_MS.
                  if (ar.exception instanceof CommandException && mRetryCount < MAX_RETRY &&
                      ((CommandException) (ar.exception)).getCommandError() ==
                              CommandException.Error.MISSING_RESOURCE) {
                      mRetryCount++;
                      removeCallbacks(mRetryRunnable);
                      postDelayed(mRetryRunnable, RETRY_INTERVAL_MS);
                  } else {
                      // if rules cannot be read from ARA applet,
                      // fallback to PKCS15-based ARF.
                      log("No ARA, try ARF next.");
                      mUiccPkcs15 = new UiccPkcs15(mUiccCard,
                              obtainMessage(EVENT_PKCS15_READ_DONE));
                  }
              }
              break;

          case EVENT_TRANSMIT_LOGICAL_CHANNEL_DONE:
              log("EVENT_TRANSMIT_LOGICAL_CHANNEL_DONE");
              ar = (AsyncResult) msg.obj;
              if (ar.exception == null && ar.result != null) {
                  IccIoResult response = (IccIoResult) ar.result;
                  if (response.sw1 == 0x90 && response.sw2 == 0x00 &&
                      response.payload != null && response.payload.length > 0) {
                      try {
                          mRules += IccUtils.bytesToHexString(response.payload).toUpperCase(Locale.US);
                          if (isDataComplete()) {
                              mAccessRules = parseRules(mRules);
                              updateState(STATE_LOADED, "Success!");
                          } else {
                              mUiccCard.iccTransmitApduLogicalChannel(mChannelId, CLA, COMMAND, P1, P2_EXTENDED_DATA, P3, DATA,
                                  obtainMessage(EVENT_TRANSMIT_LOGICAL_CHANNEL_DONE, new Integer(mChannelId)));
                              break;
                          }
                      } catch (IllegalArgumentException ex) {
                          updateState(STATE_ERROR, "Error parsing rules: " + ex);
                      } catch (IndexOutOfBoundsException ex) {
                          updateState(STATE_ERROR, "Error parsing rules: " + ex);
                      }
                   } else {
                      String errorMsg = "Invalid response: payload=" + response.payload +
                              " sw1=" + response.sw1 + " sw2=" + response.sw2;
                      updateState(STATE_ERROR, errorMsg);
                   }
              } else {
                  updateState(STATE_ERROR, "Error reading value from SIM.");
              }

              mUiccCard.iccCloseLogicalChannel(mChannelId, obtainMessage(
                      EVENT_CLOSE_LOGICAL_CHANNEL_DONE));
              mChannelId = -1;
              break;

          case EVENT_CLOSE_LOGICAL_CHANNEL_DONE:
              log("EVENT_CLOSE_LOGICAL_CHANNEL_DONE");
              break;

          case EVENT_PKCS15_READ_DONE:
              log("EVENT_PKCS15_READ_DONE");
              if (mUiccPkcs15 == null || mUiccPkcs15.getRules() == null) {
                  updateState(STATE_ERROR, "No ARA or ARF.");
              } else {
                  for (String cert : mUiccPkcs15.getRules()) {
                      AccessRule accessRule = new AccessRule(
                              IccUtils.hexStringToBytes(cert), "", 0x00);
                      mAccessRules.add(accessRule);
                  }
                  updateState(STATE_LOADED, "Success!");
              }
              break;

          default:
              Rlog.e(LOG_TAG, "Unknown event " + msg.what);
        }
    }

    /*
     * Check if all rule bytes have been read from UICC.
     * For long payload, we need to fetch it repeatly before start parsing it.
     */
    private boolean isDataComplete() {
        log("isDataComplete mRules:" + mRules);
        if (mRules.startsWith(TAG_ALL_REF_AR_DO)) {
            TLV allRules = new TLV(TAG_ALL_REF_AR_DO);
            String lengthBytes = allRules.parseLength(mRules);
            log("isDataComplete lengthBytes: " + lengthBytes);
            if (mRules.length() == TAG_ALL_REF_AR_DO.length() + lengthBytes.length() +
                                   allRules.length) {
                log("isDataComplete yes");
                return true;
            } else {
                log("isDataComplete no");
                return false;
            }
        } else {
            throw new IllegalArgumentException("Tags don't match.");
        }
    }

    /*
     * Parses the rules from the input string.
     */
    private static List<AccessRule> parseRules(String rules) {
        log("Got rules: " + rules);

        TLV allRefArDo = new TLV(TAG_ALL_REF_AR_DO); //FF40
        allRefArDo.parse(rules, true);

        String arDos = allRefArDo.value;
        List<AccessRule> accessRules = new ArrayList<AccessRule>();
        while (!arDos.isEmpty()) {
            TLV refArDo = new TLV(TAG_REF_AR_DO); //E2
            arDos = refArDo.parse(arDos, false);
            AccessRule accessRule = parseRefArdo(refArDo.value);
            if (accessRule != null) {
                accessRules.add(accessRule);
            } else {
              Rlog.e(LOG_TAG, "Skip unrecognized rule." + refArDo.value);
            }
        }
        return accessRules;
    }

    /*
     * Parses a single rule.
     */
    private static AccessRule parseRefArdo(String rule) {
        log("Got rule: " + rule);

        String certificateHash = null;
        String packageName = null;
        String tmp = null;
        long accessType = 0;

        while (!rule.isEmpty()) {
            if (rule.startsWith(TAG_REF_DO)) {
                TLV refDo = new TLV(TAG_REF_DO); //E1
                rule = refDo.parse(rule, false);

                // Skip unrelated rules.
                if (!refDo.value.startsWith(TAG_DEVICE_APP_ID_REF_DO)) {
                    return null;
                }

                TLV deviceDo = new TLV(TAG_DEVICE_APP_ID_REF_DO); //C1
                tmp = deviceDo.parse(refDo.value, false);
                certificateHash = deviceDo.value;

                if (!tmp.isEmpty()) {
                  if (!tmp.startsWith(TAG_PKG_REF_DO)) {
                      return null;
                  }
                  TLV pkgDo = new TLV(TAG_PKG_REF_DO); //CA
                  pkgDo.parse(tmp, true);
                  packageName = new String(IccUtils.hexStringToBytes(pkgDo.value));
                } else {
                  packageName = null;
                }
            } else if (rule.startsWith(TAG_AR_DO)) {
                TLV arDo = new TLV(TAG_AR_DO); //E3
                rule = arDo.parse(rule, false);

                // Skip unrelated rules.
                if (!arDo.value.startsWith(TAG_PERM_AR_DO)) {
                    return null;
                }

                TLV permDo = new TLV(TAG_PERM_AR_DO); //DB
                permDo.parse(arDo.value, true);
            } else  {
                // Spec requires it must be either TAG_REF_DO or TAG_AR_DO.
                throw new RuntimeException("Invalid Rule type");
            }
        }

        AccessRule accessRule = new AccessRule(IccUtils.hexStringToBytes(certificateHash),
            packageName, accessType);
        return accessRule;
    }

    /*
     * Converts a Signature into a Certificate hash usable for comparison.
     */
    private static byte[] getCertHash(Signature signature, String algo) {
        try {
            MessageDigest md = MessageDigest.getInstance(algo);
            return md.digest(signature.toByteArray());
        } catch (NoSuchAlgorithmException ex) {
            Rlog.e(LOG_TAG, "NoSuchAlgorithmException: " + ex);
        }
        return null;
    }

    /*
     * Updates the state and notifies the UiccCard that the rules have finished loading.
     */
    private void updateState(int newState, String statusMessage) {
        mState.set(newState);
        if (mLoadedCallback != null) {
            mLoadedCallback.sendToTarget();
        }

        mStatusMessage = statusMessage;
    }

    private static void log(String msg) {
        if (DBG) Rlog.d(LOG_TAG, msg);
    }

    /**
     * Dumps info to Dumpsys - useful for debugging.
     */
    public void dump(FileDescriptor fd, PrintWriter pw, String[] args) {
        pw.println("UiccCarrierPrivilegeRules: " + this);
        pw.println(" mState=" + getStateString(mState.get()));
        pw.println(" mStatusMessage='" + mStatusMessage + "'");
        if (mAccessRules != null) {
            pw.println(" mAccessRules: ");
            for (AccessRule ar : mAccessRules) {
                pw.println("  rule='" + ar + "'");
            }
        } else {
            pw.println(" mAccessRules: null");
        }
        if (mUiccPkcs15 != null) {
            pw.println(" mUiccPkcs15: " + mUiccPkcs15);
            mUiccPkcs15.dump(fd, pw, args);
        } else {
            pw.println(" mUiccPkcs15: null");
        }
        pw.flush();
    }

    /*
     * Converts state into human readable format.
     */
    private String getStateString(int state) {
      switch (state) {
        case STATE_LOADING:
            return "STATE_LOADING";
        case STATE_LOADED:
            return "STATE_LOADED";
        case STATE_ERROR:
            return "STATE_ERROR";
        default:
            return "UNKNOWN";
      }
    }
}
