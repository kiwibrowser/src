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

package com.android.internal.telephony.uicc;

import android.os.AsyncResult;
import android.os.Binder;
import android.os.Handler;
import android.os.Message;
import android.telephony.Rlog;
import android.telephony.TelephonyManager;

import com.android.internal.telephony.CommandException;
import com.android.internal.telephony.CommandsInterface;
import com.android.internal.telephony.uicc.IccUtils;
import com.android.internal.telephony.uicc.UiccCarrierPrivilegeRules.TLV;

import java.io.ByteArrayInputStream;
import java.io.FileDescriptor;
import java.io.PrintWriter;
import java.lang.IllegalArgumentException;
import java.lang.IndexOutOfBoundsException;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;
import java.util.Locale;

/**
 * Class that reads PKCS15-based rules for carrier privileges.
 *
 * The spec for the rules:
 *     GP Secure Element Access Control:
 *     https://www.globalplatform.org/specificationsdevice.asp
 *
 * The UiccPkcs15 class handles overall flow of finding/selecting PKCS15 applet
 * and reading/parsing each file. Because PKCS15 can be selected in 2 different ways:
 * via logical channel or EF_DIR, PKCS15Selector is a handler to encapsulate the flow.
 * Similarly, FileHandler is used for selecting/reading each file, so common codes are
 * all in same place.
 *
 * {@hide}
 */
public class UiccPkcs15 extends Handler {
    private static final String LOG_TAG = "UiccPkcs15";
    private static final boolean DBG = true;

    // File handler for PKCS15 files, select file and read binary,
    // convert to String then send to callback message.
    private class FileHandler extends Handler {
        // EF path for PKCS15 root, eg. "3F007F50"
        // null if logical channel is used for PKCS15 access.
        private final String mPkcs15Path;
        // Message to send when file has been parsed.
        private Message mCallback;
        // File id to read data from, eg. "5031"
        private String mFileId;

        // async events for the sequence of select and read
        static protected final int EVENT_SELECT_FILE_DONE = 101;
        static protected final int EVENT_READ_BINARY_DONE = 102;

        // pkcs15Path is nullable when using logical channel
        public FileHandler(String pkcs15Path) {
            log("Creating FileHandler, pkcs15Path: " + pkcs15Path);
            mPkcs15Path = pkcs15Path;
        }

        public boolean loadFile(String fileId, Message callBack) {
            log("loadFile: " + fileId);
            if (fileId == null || callBack == null) return false;
            mFileId = fileId;
            mCallback = callBack;
            selectFile();
            return true;
        }

        private void selectFile() {
            if (mChannelId >= 0) {
                mUiccCard.iccTransmitApduLogicalChannel(mChannelId, 0x00, 0xA4, 0x00, 0x04, 0x02,
                        mFileId, obtainMessage(EVENT_SELECT_FILE_DONE));
            } else {
                log("EF based");
            }
        }

        private void readBinary() {
            if (mChannelId >=0 ) {
                mUiccCard.iccTransmitApduLogicalChannel(mChannelId, 0x00, 0xB0, 0x00, 0x00, 0x00,
                        "", obtainMessage(EVENT_READ_BINARY_DONE));
            } else {
                log("EF based");
            }
        }

        @Override
        public void handleMessage(Message msg) {
            log("handleMessage: " + msg.what);
            AsyncResult ar = (AsyncResult) msg.obj;
            if (ar.exception != null || ar.result == null) {
                log("Error: " + ar.exception);
                AsyncResult.forMessage(mCallback, null, ar.exception);
                mCallback.sendToTarget();
                return;
            }

            switch (msg.what) {
                case EVENT_SELECT_FILE_DONE:
                    readBinary();
                    break;

                case EVENT_READ_BINARY_DONE:
                    IccIoResult response = (IccIoResult) ar.result;
                    String result = IccUtils.bytesToHexString(response.payload)
                            .toUpperCase(Locale.US);
                    log("IccIoResult: " + response + " payload: " + result);
                    AsyncResult.forMessage(mCallback, result, (result == null) ?
                            new IccException("Error: null response for " + mFileId) : null);
                    mCallback.sendToTarget();
                    break;

                default:
                    log("Unknown event" + msg.what);
            }
        }
    }

    private class Pkcs15Selector extends Handler {
        private static final String PKCS15_AID = "A000000063504B43532D3135";
        private Message mCallback;
        private static final int EVENT_OPEN_LOGICAL_CHANNEL_DONE = 201;

        public Pkcs15Selector(Message callBack) {
            mCallback = callBack;
            mUiccCard.iccOpenLogicalChannel(PKCS15_AID,
                    obtainMessage(EVENT_OPEN_LOGICAL_CHANNEL_DONE));
        }

        @Override
        public void handleMessage(Message msg) {
            log("handleMessage: " + msg.what);
            AsyncResult ar;

            switch (msg.what) {
              case EVENT_OPEN_LOGICAL_CHANNEL_DONE:
                  ar = (AsyncResult) msg.obj;
                  if (ar.exception == null && ar.result != null) {
                      mChannelId = ((int[]) ar.result)[0];
                      log("mChannelId: " + mChannelId);
                      AsyncResult.forMessage(mCallback, null, null);
                  } else {
                      log("error: " + ar.exception);
                      AsyncResult.forMessage(mCallback, null, ar.exception);
                      // TODO: don't sendToTarget and read EF_DIR to find PKCS15
                  }
                  mCallback.sendToTarget();
                  break;

              default:
                  log("Unknown event" + msg.what);
            }
        }
    }

    private UiccCard mUiccCard;  // Parent
    private Message mLoadedCallback;
    private int mChannelId = -1; // Channel Id for communicating with UICC.
    private List<String> mRules = new ArrayList<String>();
    private Pkcs15Selector mPkcs15Selector;
    private FileHandler mFh;

    private static final int EVENT_SELECT_PKCS15_DONE = 1;
    private static final int EVENT_LOAD_ODF_DONE = 2;
    private static final int EVENT_LOAD_DODF_DONE = 3;
    private static final int EVENT_LOAD_ACMF_DONE = 4;
    private static final int EVENT_LOAD_ACRF_DONE = 5;
    private static final int EVENT_LOAD_ACCF_DONE = 6;
    private static final int EVENT_CLOSE_LOGICAL_CHANNEL_DONE = 7;

    public UiccPkcs15(UiccCard uiccCard, Message loadedCallback) {
        log("Creating UiccPkcs15");
        mUiccCard = uiccCard;
        mLoadedCallback = loadedCallback;
        mPkcs15Selector = new Pkcs15Selector(obtainMessage(EVENT_SELECT_PKCS15_DONE));
    }

    @Override
    public void handleMessage(Message msg) {
        log("handleMessage: " + msg.what);
        AsyncResult ar = (AsyncResult) msg.obj;

        switch (msg.what) {
          case EVENT_SELECT_PKCS15_DONE:
              if (ar.exception == null) {
                  // ar.result is null if using logical channel,
                  // or string for pkcs15 path if using file access.
                  mFh = new FileHandler((String)ar.result);
                  if (!mFh.loadFile(ID_ACRF, obtainMessage(EVENT_LOAD_ACRF_DONE))) {
                      cleanUp();
                  }
              } else {
                  log("select pkcs15 failed: " + ar.exception);
                  // select PKCS15 failed, notify uiccCarrierPrivilegeRules
                  mLoadedCallback.sendToTarget();
              }
              break;

          case EVENT_LOAD_ACRF_DONE:
              if (ar.exception == null && ar.result != null) {
                  String idAccf = parseAcrf((String)ar.result);
                  if (!mFh.loadFile(idAccf, obtainMessage(EVENT_LOAD_ACCF_DONE))) {
                      cleanUp();
                  }
              } else {
                  cleanUp();
              }
              break;

          case EVENT_LOAD_ACCF_DONE:
              if (ar.exception == null && ar.result != null) {
                  parseAccf((String)ar.result);
              }
              // We are done here, no more file to read
              cleanUp();
              break;

          case EVENT_CLOSE_LOGICAL_CHANNEL_DONE:
              break;

          default:
              Rlog.e(LOG_TAG, "Unknown event " + msg.what);
        }
    }

    private void cleanUp() {
        log("cleanUp");
        if (mChannelId >= 0) {
            mUiccCard.iccCloseLogicalChannel(mChannelId, obtainMessage(
                    EVENT_CLOSE_LOGICAL_CHANNEL_DONE));
            mChannelId = -1;
        }
        mLoadedCallback.sendToTarget();
    }

    // Constants defined in specs, needed for parsing
    private static final String CARRIER_RULE_AID = "FFFFFFFFFFFF"; // AID for carrier privilege rule
    private static final String ID_ACRF = "4300";
    private static final String TAG_ASN_SEQUENCE = "30";
    private static final String TAG_ASN_OCTET_STRING = "04";
    private static final String TAG_TARGET_AID = "A0";

    // parse ACRF file to get file id for ACCF file
    // data is hex string, return file id if parse success, null otherwise
    private String parseAcrf(String data) {
        String ret = null;

        String acRules = data;
        while (!acRules.isEmpty()) {
            TLV tlvRule = new TLV(TAG_ASN_SEQUENCE);
            try {
                acRules = tlvRule.parse(acRules, false);
                String ruleString = tlvRule.getValue();
                if (ruleString.startsWith(TAG_TARGET_AID)) {
                    // rule string consists of target AID + path, example:
                    // [A0] 08 [04] 06 FF FF FF FF FF FF [30] 04 [04] 02 43 10
                    // bytes in [] are tags for the data
                    TLV tlvTarget = new TLV(TAG_TARGET_AID); // A0
                    TLV tlvAid = new TLV(TAG_ASN_OCTET_STRING); // 04
                    TLV tlvAsnPath = new TLV(TAG_ASN_SEQUENCE); // 30
                    TLV tlvPath = new TLV(TAG_ASN_OCTET_STRING);  // 04

                    // populate tlvTarget.value with aid data,
                    // ruleString has remaining data for path
                    ruleString = tlvTarget.parse(ruleString, false);
                    // parse tlvTarget.value to get actual strings for AID.
                    // no other tags expected so shouldConsumeAll is true.
                    tlvAid.parse(tlvTarget.getValue(), true);

                    if (CARRIER_RULE_AID.equals(tlvAid.getValue())) {
                        tlvAsnPath.parse(ruleString, true);
                        tlvPath.parse(tlvAsnPath.getValue(), true);
                        ret = tlvPath.getValue();
                    }
                }
                continue; // skip current rule as it doesn't have expected TAG
            } catch (IllegalArgumentException|IndexOutOfBoundsException ex) {
                log("Error: " + ex);
                break; // Bad data, ignore all remaining ACRules
            }
        }
        return ret;
    }

    // parse ACCF and add to mRules
    private void parseAccf(String data) {
        String acCondition = data;
        while (!acCondition.isEmpty()) {
            TLV tlvCondition = new TLV(TAG_ASN_SEQUENCE);
            TLV tlvCert = new TLV(TAG_ASN_OCTET_STRING);
            try {
                acCondition = tlvCondition.parse(acCondition, false);
                tlvCert.parse(tlvCondition.getValue(), true);
                if (!tlvCert.getValue().isEmpty()) {
                    mRules.add(tlvCert.getValue());
                }
            } catch (IllegalArgumentException|IndexOutOfBoundsException ex) {
                log("Error: " + ex);
                break; // Bad data, ignore all remaining acCondition data
            }
        }
    }

    public List<String> getRules() {
        return mRules;
    }

    private static void log(String msg) {
        if (DBG) Rlog.d(LOG_TAG, msg);
    }

    /**
     * Dumps info to Dumpsys - useful for debugging.
     */
    public void dump(FileDescriptor fd, PrintWriter pw, String[] args) {
        if (mRules != null) {
            pw.println(" mRules:");
            for (String cert : mRules) {
                pw.println("  " + cert);
            }
        }
    }
}
