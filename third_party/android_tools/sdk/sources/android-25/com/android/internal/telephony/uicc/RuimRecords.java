/*
 * Copyright (C) 2008 The Android Open Source Project
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

import static com.android.internal.telephony.TelephonyProperties.PROPERTY_TEST_CSIM;

import java.io.FileDescriptor;
import java.io.PrintWriter;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Locale;
import android.content.Context;
import android.os.AsyncResult;
import android.os.Message;
import android.os.SystemProperties;
import android.telephony.SubscriptionInfo;
import android.telephony.SubscriptionManager;
import android.telephony.Rlog;
import android.text.TextUtils;
import android.util.Log;
import android.content.res.Resources;

import com.android.internal.telephony.CommandsInterface;
import com.android.internal.telephony.GsmAlphabet;
import com.android.internal.telephony.MccTable;
import com.android.internal.telephony.SubscriptionController;

import com.android.internal.telephony.cdma.sms.UserData;
import com.android.internal.telephony.uicc.IccCardApplicationStatus.AppType;
import com.android.internal.util.BitwiseInputStream;

/**
 * {@hide}
 */
public class RuimRecords extends IccRecords {
    static final String LOG_TAG = "RuimRecords";

    private boolean  mOtaCommited=false;

    // ***** Instance Variables

    private String mMyMobileNumber;
    private String mMin2Min1;

    private String mPrlVersion;
    // From CSIM application
    private byte[] mEFpl = null;
    private byte[] mEFli = null;
    boolean mCsimSpnDisplayCondition = false;
    private String mMdn;
    private String mMin;
    private String mHomeSystemId;
    private String mHomeNetworkId;
    private String mNai;

    @Override
    public String toString() {
        return "RuimRecords: " + super.toString()
                + " m_ota_commited" + mOtaCommited
                + " mMyMobileNumber=" + "xxxx"
                + " mMin2Min1=" + mMin2Min1
                + " mPrlVersion=" + mPrlVersion
                + " mEFpl=" + mEFpl
                + " mEFli=" + mEFli
                + " mCsimSpnDisplayCondition=" + mCsimSpnDisplayCondition
                + " mMdn=" + mMdn
                + " mMin=" + mMin
                + " mHomeSystemId=" + mHomeSystemId
                + " mHomeNetworkId=" + mHomeNetworkId;
    }

    // ***** Event Constants
    private static final int EVENT_GET_IMSI_DONE = 3;
    private static final int EVENT_GET_DEVICE_IDENTITY_DONE = 4;
    private static final int EVENT_GET_ICCID_DONE = 5;
    private static final int EVENT_GET_CDMA_SUBSCRIPTION_DONE = 10;
    private static final int EVENT_UPDATE_DONE = 14;
    private static final int EVENT_GET_SST_DONE = 17;
    private static final int EVENT_GET_ALL_SMS_DONE = 18;
    private static final int EVENT_MARK_SMS_READ_DONE = 19;

    private static final int EVENT_SMS_ON_RUIM = 21;
    private static final int EVENT_GET_SMS_DONE = 22;

    private static final int EVENT_RUIM_REFRESH = 31;

    public RuimRecords(UiccCardApplication app, Context c, CommandsInterface ci) {
        super(app, c, ci);

        mAdnCache = new AdnRecordCache(mFh);

        mRecordsRequested = false;  // No load request is made till SIM ready

        // recordsToLoad is set to 0 because no requests are made yet
        mRecordsToLoad = 0;

        // NOTE the EVENT_SMS_ON_RUIM is not registered
        mCi.registerForIccRefresh(this, EVENT_RUIM_REFRESH, null);

        // Start off by setting empty state
        resetRecords();

        mParentApp.registerForReady(this, EVENT_APP_READY, null);
        if (DBG) log("RuimRecords X ctor this=" + this);
    }

    @Override
    public void dispose() {
        if (DBG) log("Disposing RuimRecords " + this);
        //Unregister for all events
        mCi.unregisterForIccRefresh(this);
        mParentApp.unregisterForReady(this);
        resetRecords();
        super.dispose();
    }

    @Override
    protected void finalize() {
        if(DBG) log("RuimRecords finalized");
    }

    protected void resetRecords() {
        mMncLength = UNINITIALIZED;
        log("setting0 mMncLength" + mMncLength);
        mIccId = null;
        mFullIccId = null;

        mAdnCache.reset();

        // Don't clean up PROPERTY_ICC_OPERATOR_ISO_COUNTRY and
        // PROPERTY_ICC_OPERATOR_NUMERIC here. Since not all CDMA
        // devices have RUIM, these properties should keep the original
        // values, e.g. build time settings, when there is no RUIM but
        // set new values when RUIM is available and loaded.

        // recordsRequested is set to false indicating that the SIM
        // read requests made so far are not valid. This is set to
        // true only when fresh set of read requests are made.
        mRecordsRequested = false;
    }

    @Override
    public String getIMSI() {
        return mImsi;
    }

    public String getMdnNumber() {
        return mMyMobileNumber;
    }

    public String getCdmaMin() {
         return mMin2Min1;
    }

    /** Returns null if RUIM is not yet ready */
    public String getPrlVersion() {
        return mPrlVersion;
    }

    @Override
    /** Returns null if RUIM is not yet ready */
    public String getNAI() {
        return mNai;
    }

    @Override
    public void setVoiceMailNumber(String alphaTag, String voiceNumber, Message onComplete){
        // In CDMA this is Operator/OEM dependent
        AsyncResult.forMessage((onComplete)).exception =
                new IccException("setVoiceMailNumber not implemented");
        onComplete.sendToTarget();
        loge("method setVoiceMailNumber is not implemented");
    }

    /**
     * Called by CCAT Service when REFRESH is received.
     * @param fileChanged indicates whether any files changed
     * @param fileList if non-null, a list of EF files that changed
     */
    @Override
    public void onRefresh(boolean fileChanged, int[] fileList) {
        if (fileChanged) {
            // A future optimization would be to inspect fileList and
            // only reload those files that we care about.  For now,
            // just re-fetch all RUIM records that we cache.
            fetchRuimRecords();
        }
    }

    private int adjstMinDigits (int digits) {
        // Per C.S0005 section 2.3.1.
        digits += 111;
        digits = (digits % 10 == 0)?(digits - 10):digits;
        digits = ((digits / 10) % 10 == 0)?(digits - 100):digits;
        digits = ((digits / 100) % 10 == 0)?(digits - 1000):digits;
        return digits;
    }

    /**
     * Returns the 5 or 6 digit MCC/MNC of the operator that
     *  provided the RUIM card. Returns null of RUIM is not yet ready
     */
    public String getRUIMOperatorNumeric() {
        if (mImsi == null) {
            return null;
        }

        if (mMncLength != UNINITIALIZED && mMncLength != UNKNOWN) {
            // Length = length of MCC + length of MNC
            // length of mcc = 3 (3GPP2 C.S0005 - Section 2.3)
            return mImsi.substring(0, 3 + mMncLength);
        }

        // Guess the MNC length based on the MCC if we don't
        // have a valid value in ef[ad]

        int mcc = Integer.parseInt(mImsi.substring(0,3));
        return mImsi.substring(0, 3 + MccTable.smallestDigitsMccForMnc(mcc));
    }

    // Refer to ETSI TS 102.221
    private class EfPlLoaded implements IccRecordLoaded {
        @Override
        public String getEfName() {
            return "EF_PL";
        }

        @Override
        public void onRecordLoaded(AsyncResult ar) {
            mEFpl = (byte[]) ar.result;
            if (DBG) log("EF_PL=" + IccUtils.bytesToHexString(mEFpl));
        }
    }

    // Refer to C.S0065 5.2.26
    private class EfCsimLiLoaded implements IccRecordLoaded {
        @Override
        public String getEfName() {
            return "EF_CSIM_LI";
        }

        @Override
        public void onRecordLoaded(AsyncResult ar) {
            mEFli = (byte[]) ar.result;
            // convert csim efli data to iso 639 format
            for (int i = 0; i < mEFli.length; i+=2) {
                switch(mEFli[i+1]) {
                case 0x01: mEFli[i] = 'e'; mEFli[i+1] = 'n';break;
                case 0x02: mEFli[i] = 'f'; mEFli[i+1] = 'r';break;
                case 0x03: mEFli[i] = 'e'; mEFli[i+1] = 's';break;
                case 0x04: mEFli[i] = 'j'; mEFli[i+1] = 'a';break;
                case 0x05: mEFli[i] = 'k'; mEFli[i+1] = 'o';break;
                case 0x06: mEFli[i] = 'z'; mEFli[i+1] = 'h';break;
                case 0x07: mEFli[i] = 'h'; mEFli[i+1] = 'e';break;
                default: mEFli[i] = ' '; mEFli[i+1] = ' ';
                }
            }

            if (DBG) log("EF_LI=" + IccUtils.bytesToHexString(mEFli));
        }
    }

    // Refer to C.S0065 5.2.32
    private class EfCsimSpnLoaded implements IccRecordLoaded {
        @Override
        public String getEfName() {
            return "EF_CSIM_SPN";
        }

        @Override
        public void onRecordLoaded(AsyncResult ar) {
            byte[] data = (byte[]) ar.result;
            if (DBG) log("CSIM_SPN=" +
                         IccUtils.bytesToHexString(data));

            // C.S0065 for EF_SPN decoding
            mCsimSpnDisplayCondition = ((0x01 & data[0]) != 0);

            int encoding = data[1];
            int language = data[2];
            byte[] spnData = new byte[32];
            int len = ((data.length - 3) < 32) ? (data.length - 3) : 32;
            System.arraycopy(data, 3, spnData, 0, len);

            int numBytes;
            for (numBytes = 0; numBytes < spnData.length; numBytes++) {
                if ((spnData[numBytes] & 0xFF) == 0xFF) break;
            }

            if (numBytes == 0) {
                setServiceProviderName("");
                return;
            }
            try {
                switch (encoding) {
                case UserData.ENCODING_OCTET:
                case UserData.ENCODING_LATIN:
                    setServiceProviderName(new String(spnData, 0, numBytes, "ISO-8859-1"));
                    break;
                case UserData.ENCODING_IA5:
                case UserData.ENCODING_GSM_7BIT_ALPHABET:
                    setServiceProviderName(
                            GsmAlphabet.gsm7BitPackedToString(spnData, 0, (numBytes*8)/7));
                    break;
                case UserData.ENCODING_7BIT_ASCII:
                    String spn = new String(spnData, 0, numBytes, "US-ASCII");
                    // To address issues with incorrect encoding scheme
                    // programmed in some commercial CSIM cards, the decoded
                    // SPN is checked to have characters in printable ASCII
                    // range. If not, they are decoded with
                    // ENCODING_GSM_7BIT_ALPHABET scheme.
                    if (TextUtils.isPrintableAsciiOnly(spn)) {
                        setServiceProviderName(spn);
                    } else {
                        if (DBG) log("Some corruption in SPN decoding = " + spn);
                        if (DBG) log("Using ENCODING_GSM_7BIT_ALPHABET scheme...");
                        setServiceProviderName(
                                GsmAlphabet.gsm7BitPackedToString(spnData, 0, (numBytes * 8) / 7));
                    }
                break;
                case UserData.ENCODING_UNICODE_16:
                    setServiceProviderName(new String(spnData, 0, numBytes, "utf-16"));
                    break;
                default:
                    log("SPN encoding not supported");
                }
            } catch(Exception e) {
                log("spn decode error: " + e);
            }
            if (DBG) log("spn=" + getServiceProviderName());
            if (DBG) log("spnCondition=" + mCsimSpnDisplayCondition);
            mTelephonyManager.setSimOperatorNameForPhone(
                    mParentApp.getPhoneId(), getServiceProviderName());
        }
    }

    private class EfCsimMdnLoaded implements IccRecordLoaded {
        @Override
        public String getEfName() {
            return "EF_CSIM_MDN";
        }

        @Override
        public void onRecordLoaded(AsyncResult ar) {
            byte[] data = (byte[]) ar.result;
            if (DBG) log("CSIM_MDN=" + IccUtils.bytesToHexString(data));
            // Refer to C.S0065 5.2.35
            int mdnDigitsNum = 0x0F & data[0];
            mMdn = IccUtils.cdmaBcdToString(data, 1, mdnDigitsNum);
            if (DBG) log("CSIM MDN=" + mMdn);
        }
    }

    private class EfCsimImsimLoaded implements IccRecordLoaded {
        @Override
        public String getEfName() {
            return "EF_CSIM_IMSIM";
        }

        @Override
        public void onRecordLoaded(AsyncResult ar) {
            byte[] data = (byte[]) ar.result;
            if (VDBG) log("CSIM_IMSIM=" + IccUtils.bytesToHexString(data));
            // C.S0065 section 5.2.2 for IMSI_M encoding
            // C.S0005 section 2.3.1 for MIN encoding in IMSI_M.
            boolean provisioned = ((data[7] & 0x80) == 0x80);

            if (provisioned) {
                int first3digits = ((0x03 & data[2]) << 8) + (0xFF & data[1]);
                int second3digits = (((0xFF & data[5]) << 8) | (0xFF & data[4])) >> 6;
                int digit7 = 0x0F & (data[4] >> 2);
                if (digit7 > 0x09) digit7 = 0;
                int last3digits = ((0x03 & data[4]) << 8) | (0xFF & data[3]);
                first3digits = adjstMinDigits(first3digits);
                second3digits = adjstMinDigits(second3digits);
                last3digits = adjstMinDigits(last3digits);

                StringBuilder builder = new StringBuilder();
                builder.append(String.format(Locale.US, "%03d", first3digits));
                builder.append(String.format(Locale.US, "%03d", second3digits));
                builder.append(String.format(Locale.US, "%d", digit7));
                builder.append(String.format(Locale.US, "%03d", last3digits));
                mMin = builder.toString();
                if (DBG) log("min present=" + mMin);
            } else {
                if (DBG) log("min not present");
            }
        }
    }

    private class EfCsimCdmaHomeLoaded implements IccRecordLoaded {
        @Override
        public String getEfName() {
            return "EF_CSIM_CDMAHOME";
        }

        @Override
        public void onRecordLoaded(AsyncResult ar) {
            // Per C.S0065 section 5.2.8
            ArrayList<byte[]> dataList = (ArrayList<byte[]>) ar.result;
            if (DBG) log("CSIM_CDMAHOME data size=" + dataList.size());
            if (dataList.isEmpty()) {
                return;
            }
            StringBuilder sidBuf = new StringBuilder();
            StringBuilder nidBuf = new StringBuilder();

            for (byte[] data : dataList) {
                if (data.length == 5) {
                    int sid = ((data[1] & 0xFF) << 8) | (data[0] & 0xFF);
                    int nid = ((data[3] & 0xFF) << 8) | (data[2] & 0xFF);
                    sidBuf.append(sid).append(',');
                    nidBuf.append(nid).append(',');
                }
            }
            // remove trailing ","
            sidBuf.setLength(sidBuf.length()-1);
            nidBuf.setLength(nidBuf.length()-1);

            mHomeSystemId = sidBuf.toString();
            mHomeNetworkId = nidBuf.toString();
        }
    }

    private class EfCsimEprlLoaded implements IccRecordLoaded {
        @Override
        public String getEfName() {
            return "EF_CSIM_EPRL";
        }
        @Override
        public void onRecordLoaded(AsyncResult ar) {
            onGetCSimEprlDone(ar);
        }
    }

    private void onGetCSimEprlDone(AsyncResult ar) {
        // C.S0065 section 5.2.57 for EFeprl encoding
        // C.S0016 section 3.5.5 for PRL format.
        byte[] data = (byte[]) ar.result;
        if (DBG) log("CSIM_EPRL=" + IccUtils.bytesToHexString(data));

        // Only need the first 4 bytes of record
        if (data.length > 3) {
            int prlId = ((data[2] & 0xFF) << 8) | (data[3] & 0xFF);
            mPrlVersion = Integer.toString(prlId);
        }
        if (DBG) log("CSIM PRL version=" + mPrlVersion);
    }

    private class EfCsimMipUppLoaded implements IccRecordLoaded {
        @Override
        public String getEfName() {
            return "EF_CSIM_MIPUPP";
        }

        boolean checkLengthLegal(int length, int expectLength) {
            if(length < expectLength) {
                Log.e(LOG_TAG, "CSIM MIPUPP format error, length = " + length  +
                        "expected length at least =" + expectLength);
                return false;
            } else {
                return true;
            }
        }

        @Override
        public void onRecordLoaded(AsyncResult ar) {
            // 3GPP2 C.S0065 section 5.2.24
            byte[] data = (byte[]) ar.result;

            if(data.length < 1) {
                Log.e(LOG_TAG,"MIPUPP read error");
                return;
            }

            BitwiseInputStream bitStream = new BitwiseInputStream(data);
            try {
                int  mipUppLength = bitStream.read(8);
                //transfer length from byte to bit
                mipUppLength = (mipUppLength << 3);

                if (!checkLengthLegal(mipUppLength, 1)) {
                    return;
                }
                //parse the MIPUPP body 3GPP2 C.S0016-C 3.5.8.6
                int retryInfoInclude = bitStream.read(1);
                mipUppLength--;

                if(retryInfoInclude == 1) {
                    if (!checkLengthLegal(mipUppLength, 11)) {
                        return;
                    }
                    bitStream.skip(11); //not used now
                    //transfer length from byte to bit
                    mipUppLength -= 11;
                }

                if (!checkLengthLegal(mipUppLength, 4)) {
                    return;
                }
                int numNai = bitStream.read(4);
                mipUppLength -= 4;

                //start parse NAI body
                for(int index = 0; index < numNai; index++) {
                    if (!checkLengthLegal(mipUppLength, 4)) {
                        return;
                    }
                    int naiEntryIndex = bitStream.read(4);
                    mipUppLength -= 4;

                    if (!checkLengthLegal(mipUppLength, 8)) {
                        return;
                    }
                    int naiLength = bitStream.read(8);
                    mipUppLength -= 8;

                    if(naiEntryIndex == 0) {
                        //we find the one!
                        if (!checkLengthLegal(mipUppLength, naiLength << 3)) {
                            return;
                        }
                        char naiCharArray[] = new char[naiLength];
                        for(int index1 = 0; index1 < naiLength; index1++) {
                            naiCharArray[index1] = (char)(bitStream.read(8) & 0xFF);
                        }
                        mNai =  new String(naiCharArray);
                        if (Log.isLoggable(LOG_TAG, Log.VERBOSE)) {
                            Log.v(LOG_TAG,"MIPUPP Nai = " + mNai);
                        }
                        return; //need not parsing further
                    } else {
                        //ignore this NAI body
                        if (!checkLengthLegal(mipUppLength, (naiLength << 3) + 102)) {
                            return;
                        }
                        bitStream.skip((naiLength << 3) + 101);//not used
                        int mnAaaSpiIndicator = bitStream.read(1);
                        mipUppLength -= ((naiLength << 3) + 102);

                        if(mnAaaSpiIndicator == 1) {
                            if (!checkLengthLegal(mipUppLength, 32)) {
                                return;
                            }
                            bitStream.skip(32); //not used
                            mipUppLength -= 32;
                        }

                        //MN-HA_AUTH_ALGORITHM
                        if (!checkLengthLegal(mipUppLength, 5)) {
                            return;
                        }
                        bitStream.skip(4);
                        mipUppLength -= 4;
                        int mnHaSpiIndicator = bitStream.read(1);
                        mipUppLength--;

                        if(mnHaSpiIndicator == 1) {
                            if (!checkLengthLegal(mipUppLength, 32)) {
                                return;
                            }
                            bitStream.skip(32);
                            mipUppLength -= 32;
                        }
                    }
                }
            } catch(Exception e) {
              Log.e(LOG_TAG,"MIPUPP read Exception error!");
                return;
            }
        }
    }

    @Override
    public void handleMessage(Message msg) {
        AsyncResult ar;

        byte data[];

        boolean isRecordLoadResponse = false;

        if (mDestroyed.get()) {
            loge("Received message " + msg +
                    "[" + msg.what + "] while being destroyed. Ignoring.");
            return;
        }

        try { switch (msg.what) {
            case EVENT_APP_READY:
                onReady();
                break;

            case EVENT_GET_DEVICE_IDENTITY_DONE:
                log("Event EVENT_GET_DEVICE_IDENTITY_DONE Received");
            break;

            /* IO events */
            case EVENT_GET_IMSI_DONE:
                isRecordLoadResponse = true;

                ar = (AsyncResult)msg.obj;
                if (ar.exception != null) {
                    loge("Exception querying IMSI, Exception:" + ar.exception);
                    break;
                }

                mImsi = (String) ar.result;

                // IMSI (MCC+MNC+MSIN) is at least 6 digits, but not more
                // than 15 (and usually 15).
                if (mImsi != null && (mImsi.length() < 6 || mImsi.length() > 15)) {
                    loge("invalid IMSI " + mImsi);
                    mImsi = null;
                }

                // FIXME: CSIM IMSI may not contain the MNC.
                if (false) {
                    log("IMSI: " + mImsi.substring(0, 6) + "xxxxxxxxx");

                    String operatorNumeric = getRUIMOperatorNumeric();
                    if (operatorNumeric != null) {
                        if (operatorNumeric.length() <= 6) {
                            log("update mccmnc=" + operatorNumeric);
                            MccTable.updateMccMncConfiguration(mContext, operatorNumeric, false);
                        }
                    }
                } else {
                    String operatorNumeric = getRUIMOperatorNumeric();
                    log("NO update mccmnc=" + operatorNumeric);
                }

            break;

            case EVENT_GET_CDMA_SUBSCRIPTION_DONE:
                ar = (AsyncResult)msg.obj;
                String localTemp[] = (String[])ar.result;
                if (ar.exception != null) {
                    break;
                }

                mMyMobileNumber = localTemp[0];
                mMin2Min1 = localTemp[3];
                mPrlVersion = localTemp[4];

                log("MDN: " + mMyMobileNumber + " MIN: " + mMin2Min1);

            break;

            case EVENT_GET_ICCID_DONE:
                isRecordLoadResponse = true;

                ar = (AsyncResult)msg.obj;
                data = (byte[])ar.result;

                if (ar.exception != null) {
                    break;
                }

                mIccId = IccUtils.bcdToString(data, 0, data.length);
                mFullIccId = IccUtils.bchToString(data, 0, data.length);

                log("iccid: " + SubscriptionInfo.givePrintableIccid(mFullIccId));

            break;

            case EVENT_UPDATE_DONE:
                ar = (AsyncResult)msg.obj;
                if (ar.exception != null) {
                    Rlog.i(LOG_TAG, "RuimRecords update failed", ar.exception);
                }
            break;

            case EVENT_GET_ALL_SMS_DONE:
            case EVENT_MARK_SMS_READ_DONE:
            case EVENT_SMS_ON_RUIM:
            case EVENT_GET_SMS_DONE:
                Rlog.w(LOG_TAG, "Event not supported: " + msg.what);
                break;

            // TODO: probably EF_CST should be read instead
            case EVENT_GET_SST_DONE:
                log("Event EVENT_GET_SST_DONE Received");
            break;

            case EVENT_RUIM_REFRESH:
                isRecordLoadResponse = false;
                ar = (AsyncResult)msg.obj;
                if (ar.exception == null) {
                    handleRuimRefresh((IccRefreshResponse)ar.result);
                }
                break;

            default:
                super.handleMessage(msg);   // IccRecords handles generic record load responses

        }}catch (RuntimeException exc) {
            // I don't want these exceptions to be fatal
            Rlog.w(LOG_TAG, "Exception parsing RUIM record", exc);
        } finally {
            // Count up record load responses even if they are fails
            if (isRecordLoadResponse) {
                onRecordLoaded();
            }
        }
    }

    /**
     * Returns an array of languages we have assets for.
     *
     * NOTE: This array will have duplicates. If this method will be caused
     * frequently or in a tight loop, it can be rewritten for efficiency.
     */
    private static String[] getAssetLanguages(Context ctx) {
        final String[] locales = ctx.getAssets().getLocales();
        final String[] localeLangs = new String[locales.length];
        for (int i = 0; i < locales.length; ++i) {
            final String localeStr = locales[i];
            final int separator = localeStr.indexOf('-');
            if (separator < 0) {
                localeLangs[i] = localeStr;
            } else {
                localeLangs[i] = localeStr.substring(0, separator);
            }
        }

        return localeLangs;
    }

    @Override
    protected void onRecordLoaded() {
        // One record loaded successfully or failed, In either case
        // we need to update the recordsToLoad count
        mRecordsToLoad -= 1;
        if (DBG) log("onRecordLoaded " + mRecordsToLoad + " requested: " + mRecordsRequested);

        if (mRecordsToLoad == 0 && mRecordsRequested == true) {
            onAllRecordsLoaded();
        } else if (mRecordsToLoad < 0) {
            loge("recordsToLoad <0, programmer error suspected");
            mRecordsToLoad = 0;
        }
    }

    @Override
    protected void onAllRecordsLoaded() {
        if (DBG) log("record load complete");

        // Further records that can be inserted are Operator/OEM dependent

        // FIXME: CSIM IMSI may not contain the MNC.
        if (false) {
            String operator = getRUIMOperatorNumeric();
            if (!TextUtils.isEmpty(operator)) {
                log("onAllRecordsLoaded set 'gsm.sim.operator.numeric' to operator='" +
                        operator + "'");
                log("update icc_operator_numeric=" + operator);
                mTelephonyManager.setSimOperatorNumericForPhone(
                        mParentApp.getPhoneId(), operator);
            } else {
                log("onAllRecordsLoaded empty 'gsm.sim.operator.numeric' skipping");
            }

            if (!TextUtils.isEmpty(mImsi)) {
                log("onAllRecordsLoaded set mcc imsi=" + (VDBG ? ("=" + mImsi) : ""));
                mTelephonyManager.setSimCountryIsoForPhone(
                        mParentApp.getPhoneId(),
                        MccTable.countryCodeForMcc(
                        Integer.parseInt(mImsi.substring(0,3))));
            } else {
                log("onAllRecordsLoaded empty imsi skipping setting mcc");
            }
        }

        Resources resource = Resources.getSystem();
        if (resource.getBoolean(com.android.internal.R.bool.config_use_sim_language_file)) {
            setSimLanguage(mEFli, mEFpl);
        }

        mRecordsLoadedRegistrants.notifyRegistrants(
            new AsyncResult(null, null, null));

        // TODO: The below is hacky since the SubscriptionController may not be ready at this time.
        if (!TextUtils.isEmpty(mMdn)) {
            int phoneId = mParentApp.getUiccCard().getPhoneId();
            int[] subIds = SubscriptionController.getInstance().getSubId(phoneId);
            if (subIds != null) {
                SubscriptionManager.from(mContext).setDisplayNumber(mMdn, subIds[0]);
            } else {
                log("Cannot call setDisplayNumber: invalid subId");
            }
        }
    }

    @Override
    public void onReady() {
        fetchRuimRecords();

        mCi.getCDMASubscription(obtainMessage(EVENT_GET_CDMA_SUBSCRIPTION_DONE));
    }


    private void fetchRuimRecords() {
        mRecordsRequested = true;

        if (DBG) log("fetchRuimRecords " + mRecordsToLoad);

        mCi.getIMSIForApp(mParentApp.getAid(), obtainMessage(EVENT_GET_IMSI_DONE));
        mRecordsToLoad++;

        mFh.loadEFTransparent(EF_ICCID,
                obtainMessage(EVENT_GET_ICCID_DONE));
        mRecordsToLoad++;

        mFh.loadEFTransparent(EF_PL,
                obtainMessage(EVENT_GET_ICC_RECORD_DONE, new EfPlLoaded()));
        mRecordsToLoad++;

        mFh.loadEFTransparent(EF_CSIM_LI,
                obtainMessage(EVENT_GET_ICC_RECORD_DONE, new EfCsimLiLoaded()));
        mRecordsToLoad++;

        mFh.loadEFTransparent(EF_CSIM_SPN,
                obtainMessage(EVENT_GET_ICC_RECORD_DONE, new EfCsimSpnLoaded()));
        mRecordsToLoad++;

        mFh.loadEFLinearFixed(EF_CSIM_MDN, 1,
                obtainMessage(EVENT_GET_ICC_RECORD_DONE, new EfCsimMdnLoaded()));
        mRecordsToLoad++;

        mFh.loadEFTransparent(EF_CSIM_IMSIM,
                obtainMessage(EVENT_GET_ICC_RECORD_DONE, new EfCsimImsimLoaded()));
        mRecordsToLoad++;

        mFh.loadEFLinearFixedAll(EF_CSIM_CDMAHOME,
                obtainMessage(EVENT_GET_ICC_RECORD_DONE, new EfCsimCdmaHomeLoaded()));
        mRecordsToLoad++;

        // Entire PRL could be huge. We are only interested in
        // the first 4 bytes of the record.
        mFh.loadEFTransparent(EF_CSIM_EPRL, 4,
                obtainMessage(EVENT_GET_ICC_RECORD_DONE, new EfCsimEprlLoaded()));
        mRecordsToLoad++;

        mFh.loadEFTransparent(EF_CSIM_MIPUPP,
                obtainMessage(EVENT_GET_ICC_RECORD_DONE, new EfCsimMipUppLoaded()));
        mRecordsToLoad++;

        if (DBG) log("fetchRuimRecords " + mRecordsToLoad + " requested: " + mRecordsRequested);
        // Further records that can be inserted are Operator/OEM dependent
    }

    /**
     * {@inheritDoc}
     *
     * No Display rule for RUIMs yet.
     */
    @Override
    public int getDisplayRule(String plmn) {
        // TODO together with spn
        return 0;
    }

    @Override
    public boolean isProvisioned() {
        // If UICC card has CSIM app, look for MDN and MIN field
        // to determine if the SIM is provisioned.  Otherwise,
        // consider the SIM is provisioned. (for case of ordinal
        // USIM only UICC.)
        // If PROPERTY_TEST_CSIM is defined, bypess provision check
        // and consider the SIM is provisioned.
        if (SystemProperties.getBoolean(PROPERTY_TEST_CSIM, false)) {
            return true;
        }

        if (mParentApp == null) {
            return false;
        }

        if (mParentApp.getType() == AppType.APPTYPE_CSIM &&
            ((mMdn == null) || (mMin == null))) {
            return false;
        }
        return true;
    }

    @Override
    public void setVoiceMessageWaiting(int line, int countWaiting) {
        // Will be used in future to store voice mail count in UIM
        // C.S0023-D_v1.0 does not have a file id in UIM for MWI
        log("RuimRecords:setVoiceMessageWaiting - NOP for CDMA");
    }

    @Override
    public int getVoiceMessageCount() {
        // Will be used in future to retrieve voice mail count for UIM
        // C.S0023-D_v1.0 does not have a file id in UIM for MWI
        log("RuimRecords:getVoiceMessageCount - NOP for CDMA");
        return 0;
    }

    private void handleRuimRefresh(IccRefreshResponse refreshResponse) {
        if (refreshResponse == null) {
            if (DBG) log("handleRuimRefresh received without input");
            return;
        }

        if (refreshResponse.aid != null &&
                !refreshResponse.aid.equals(mParentApp.getAid())) {
            // This is for different app. Ignore.
            return;
        }

        switch (refreshResponse.refreshResult) {
            case IccRefreshResponse.REFRESH_RESULT_FILE_UPDATE:
                if (DBG) log("handleRuimRefresh with SIM_REFRESH_FILE_UPDATED");
                mAdnCache.reset();
                fetchRuimRecords();
                break;
            case IccRefreshResponse.REFRESH_RESULT_INIT:
                if (DBG) log("handleRuimRefresh with SIM_REFRESH_INIT");
                // need to reload all files (that we care about)
                onIccRefreshInit();
                break;
            case IccRefreshResponse.REFRESH_RESULT_RESET:
                // Refresh reset is handled by the UiccCard object.
                if (DBG) log("handleRuimRefresh with SIM_REFRESH_RESET");
                break;
            default:
                // unknown refresh operation
                if (DBG) log("handleRuimRefresh with unknown operation");
                break;
        }
    }

    public String getMdn() {
        return mMdn;
    }

    public String getMin() {
        return mMin;
    }

    public String getSid() {
        return mHomeSystemId;
    }

    public String getNid() {
        return mHomeNetworkId;
    }

    public boolean getCsimSpnDisplayCondition() {
        return mCsimSpnDisplayCondition;
    }
    @Override
    protected void log(String s) {
        Rlog.d(LOG_TAG, "[RuimRecords] " + s);
    }

    @Override
    protected void loge(String s) {
        Rlog.e(LOG_TAG, "[RuimRecords] " + s);
    }

    @Override
    public void dump(FileDescriptor fd, PrintWriter pw, String[] args) {
        pw.println("RuimRecords: " + this);
        pw.println(" extends:");
        super.dump(fd, pw, args);
        pw.println(" mOtaCommited=" + mOtaCommited);
        pw.println(" mMyMobileNumber=" + mMyMobileNumber);
        pw.println(" mMin2Min1=" + mMin2Min1);
        pw.println(" mPrlVersion=" + mPrlVersion);
        pw.println(" mEFpl[]=" + Arrays.toString(mEFpl));
        pw.println(" mEFli[]=" + Arrays.toString(mEFli));
        pw.println(" mCsimSpnDisplayCondition=" + mCsimSpnDisplayCondition);
        pw.println(" mMdn=" + mMdn);
        pw.println(" mMin=" + mMin);
        pw.println(" mHomeSystemId=" + mHomeSystemId);
        pw.println(" mHomeNetworkId=" + mHomeNetworkId);
        pw.flush();
    }
}
