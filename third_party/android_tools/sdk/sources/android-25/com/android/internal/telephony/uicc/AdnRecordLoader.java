/*
 * Copyright (C) 2006 The Android Open Source Project
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

import java.util.ArrayList;

import android.os.AsyncResult;
import android.os.Handler;
import android.os.Looper;
import android.os.Message;
import android.telephony.Rlog;

import com.android.internal.telephony.uicc.IccConstants;

public class AdnRecordLoader extends Handler {
    final static String LOG_TAG = "AdnRecordLoader";
    final static boolean VDBG = false;

    //***** Instance Variables

    private IccFileHandler mFh;
    int mEf;
    int mExtensionEF;
    int mPendingExtLoads;
    Message mUserResponse;
    String mPin2;

    // For "load one"
    int mRecordNumber;

    // for "load all"
    ArrayList<AdnRecord> mAdns; // only valid after EVENT_ADN_LOAD_ALL_DONE

    // Either an AdnRecord or a reference to adns depending
    // if this is a load one or load all operation
    Object mResult;

    //***** Event Constants

    static final int EVENT_ADN_LOAD_DONE = 1;
    static final int EVENT_EXT_RECORD_LOAD_DONE = 2;
    static final int EVENT_ADN_LOAD_ALL_DONE = 3;
    static final int EVENT_EF_LINEAR_RECORD_SIZE_DONE = 4;
    static final int EVENT_UPDATE_RECORD_DONE = 5;

    //***** Constructor

    AdnRecordLoader(IccFileHandler fh) {
        // The telephony unit-test cases may create AdnRecords
        // in secondary threads
        super(Looper.getMainLooper());
        mFh = fh;
    }

    private String getEFPath(int efid) {
        if (efid == IccConstants.EF_ADN) {
            return IccConstants.MF_SIM + IccConstants.DF_TELECOM;
        }

        return null;
    }

    /**
     * Resulting AdnRecord is placed in response.obj.result
     * or response.obj.exception is set
     */
    public void
    loadFromEF(int ef, int extensionEF, int recordNumber,
                Message response) {
        mEf = ef;
        mExtensionEF = extensionEF;
        mRecordNumber = recordNumber;
        mUserResponse = response;

       mFh.loadEFLinearFixed(
               ef, getEFPath(ef), recordNumber,
               obtainMessage(EVENT_ADN_LOAD_DONE));
    }


    /**
     * Resulting ArrayList&lt;adnRecord> is placed in response.obj.result
     * or response.obj.exception is set
     */
    public void
    loadAllFromEF(int ef, int extensionEF,
                Message response) {
        mEf = ef;
        mExtensionEF = extensionEF;
        mUserResponse = response;

        /* If we are loading from EF_ADN, specifically
         * specify the path as well, since, on some cards,
         * the fileid is not unique.
         */
        mFh.loadEFLinearFixedAll(
                ef, getEFPath(ef),
                obtainMessage(EVENT_ADN_LOAD_ALL_DONE));
    }

    /**
     * Write adn to a EF SIM record
     * It will get the record size of EF record and compose hex adn array
     * then write the hex array to EF record
     *
     * @param adn is set with alphaTag and phone number
     * @param ef EF fileid
     * @param extensionEF extension EF fileid
     * @param recordNumber 1-based record index
     * @param pin2 for CHV2 operations, must be null if pin2 is not needed
     * @param response will be sent to its handler when completed
     */
    public void
    updateEF(AdnRecord adn, int ef, int extensionEF, int recordNumber,
            String pin2, Message response) {
        mEf = ef;
        mExtensionEF = extensionEF;
        mRecordNumber = recordNumber;
        mUserResponse = response;
        mPin2 = pin2;
 
        mFh.getEFLinearRecordSize( ef, getEFPath(ef),
                obtainMessage(EVENT_EF_LINEAR_RECORD_SIZE_DONE, adn));
     }

    //***** Overridden from Handler

    @Override
    public void
    handleMessage(Message msg) {
        AsyncResult ar;
        byte data[];
        AdnRecord adn;

        try {
            switch (msg.what) {
                case EVENT_EF_LINEAR_RECORD_SIZE_DONE:
                    ar = (AsyncResult)(msg.obj);
                    adn = (AdnRecord)(ar.userObj);

                    if (ar.exception != null) {
                        throw new RuntimeException("get EF record size failed",
                                ar.exception);
                    }

                    int[] recordSize = (int[])ar.result;
                    // recordSize is int[3] array
                    // int[0]  is the record length
                    // int[1]  is the total length of the EF file
                    // int[2]  is the number of records in the EF file
                    // So int[0] * int[2] = int[1]
                   if (recordSize.length != 3 || mRecordNumber > recordSize[2]) {
                        throw new RuntimeException("get wrong EF record size format",
                                ar.exception);
                    }

                    data = adn.buildAdnString(recordSize[0]);

                    if(data == null) {
                        throw new RuntimeException("wrong ADN format",
                                ar.exception);
                    }


                    mFh.updateEFLinearFixed(mEf, getEFPath(mEf), mRecordNumber,
                            data, mPin2, obtainMessage(EVENT_UPDATE_RECORD_DONE));

                    mPendingExtLoads = 1;

                    break;
                case EVENT_UPDATE_RECORD_DONE:
                    ar = (AsyncResult)(msg.obj);
                    if (ar.exception != null) {
                        throw new RuntimeException("update EF adn record failed",
                                ar.exception);
                    }
                    mPendingExtLoads = 0;
                    mResult = null;
                    break;
                case EVENT_ADN_LOAD_DONE:
                    ar = (AsyncResult)(msg.obj);
                    data = (byte[])(ar.result);

                    if (ar.exception != null) {
                        throw new RuntimeException("load failed", ar.exception);
                    }

                    if (VDBG) {
                        Rlog.d(LOG_TAG,"ADN EF: 0x"
                            + Integer.toHexString(mEf)
                            + ":" + mRecordNumber
                            + "\n" + IccUtils.bytesToHexString(data));
                    }

                    adn = new AdnRecord(mEf, mRecordNumber, data);
                    mResult = adn;

                    if (adn.hasExtendedRecord()) {
                        // If we have a valid value in the ext record field,
                        // we're not done yet: we need to read the corresponding
                        // ext record and append it

                        mPendingExtLoads = 1;

                        mFh.loadEFLinearFixed(
                            mExtensionEF, adn.mExtRecord,
                            obtainMessage(EVENT_EXT_RECORD_LOAD_DONE, adn));
                    }
                break;

                case EVENT_EXT_RECORD_LOAD_DONE:
                    ar = (AsyncResult)(msg.obj);
                    data = (byte[])(ar.result);
                    adn = (AdnRecord)(ar.userObj);

                    if (ar.exception == null) {
                        Rlog.d(LOG_TAG,"ADN extension EF: 0x"
                                + Integer.toHexString(mExtensionEF)
                                + ":" + adn.mExtRecord
                                + "\n" + IccUtils.bytesToHexString(data));

                        adn.appendExtRecord(data);
                    }
                    else {
                        // If we can't get the rest of the number from EF_EXT1, rather than
                        // providing the partial number, we clear the number since it's not
                        // dialable anyway. Do not throw exception here otherwise the rest
                        // of the good records will be dropped.

                        Rlog.e(LOG_TAG, "Failed to read ext record. Clear the number now.");
                        adn.setNumber("");
                    }

                    mPendingExtLoads--;
                    // result should have been set in
                    // EVENT_ADN_LOAD_DONE or EVENT_ADN_LOAD_ALL_DONE
                break;

                case EVENT_ADN_LOAD_ALL_DONE:
                    ar = (AsyncResult)(msg.obj);
                    ArrayList<byte[]> datas = (ArrayList<byte[]>)(ar.result);

                    if (ar.exception != null) {
                        throw new RuntimeException("load failed", ar.exception);
                    }

                    mAdns = new ArrayList<AdnRecord>(datas.size());
                    mResult = mAdns;
                    mPendingExtLoads = 0;

                    for(int i = 0, s = datas.size() ; i < s ; i++) {
                        adn = new AdnRecord(mEf, 1 + i, datas.get(i));
                        mAdns.add(adn);

                        if (adn.hasExtendedRecord()) {
                            // If we have a valid value in the ext record field,
                            // we're not done yet: we need to read the corresponding
                            // ext record and append it

                            mPendingExtLoads++;

                            mFh.loadEFLinearFixed(
                                mExtensionEF, adn.mExtRecord,
                                obtainMessage(EVENT_EXT_RECORD_LOAD_DONE, adn));
                        }
                    }
                break;
            }
        } catch (RuntimeException exc) {
            if (mUserResponse != null) {
                AsyncResult.forMessage(mUserResponse)
                                .exception = exc;
                mUserResponse.sendToTarget();
                // Loading is all or nothing--either every load succeeds
                // or we fail the whole thing.
                mUserResponse = null;
            }
            return;
        }

        if (mUserResponse != null && mPendingExtLoads == 0) {
            AsyncResult.forMessage(mUserResponse).result
                = mResult;

            mUserResponse.sendToTarget();
            mUserResponse = null;
        }
    }
}
