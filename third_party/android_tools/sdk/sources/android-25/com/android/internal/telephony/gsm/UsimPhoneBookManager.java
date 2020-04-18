/*
 * Copyright (C) 2009 The Android Open Source Project
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

package com.android.internal.telephony.gsm;

import android.os.AsyncResult;
import android.os.Handler;
import android.os.Message;
import android.telephony.Rlog;
import android.util.SparseArray;
import android.util.SparseIntArray;

import com.android.internal.telephony.uicc.AdnRecord;
import com.android.internal.telephony.uicc.AdnRecordCache;
import com.android.internal.telephony.uicc.IccConstants;
import com.android.internal.telephony.uicc.IccFileHandler;
import com.android.internal.telephony.uicc.IccUtils;
import java.util.ArrayList;

/**
 * This class implements reading and parsing USIM records.
 * Refer to Spec 3GPP TS 31.102 for more details.
 *
 * {@hide}
 */
public class UsimPhoneBookManager extends Handler implements IccConstants {
    private static final String LOG_TAG = "UsimPhoneBookManager";
    private static final boolean DBG = true;
    private ArrayList<PbrRecord> mPbrRecords;
    private Boolean mIsPbrPresent;
    private IccFileHandler mFh;
    private AdnRecordCache mAdnCache;
    private Object mLock = new Object();
    private ArrayList<AdnRecord> mPhoneBookRecords;
    private ArrayList<byte[]> mIapFileRecord;
    private ArrayList<byte[]> mEmailFileRecord;

    // email list for each ADN record. The key would be
    // ADN's efid << 8 + record #
    private SparseArray<ArrayList<String>> mEmailsForAdnRec;

    // SFI to ADN Efid mapping table
    private SparseIntArray mSfiEfidTable;

    private boolean mRefreshCache = false;


    private static final int EVENT_PBR_LOAD_DONE = 1;
    private static final int EVENT_USIM_ADN_LOAD_DONE = 2;
    private static final int EVENT_IAP_LOAD_DONE = 3;
    private static final int EVENT_EMAIL_LOAD_DONE = 4;

    private static final int USIM_TYPE1_TAG   = 0xA8;
    private static final int USIM_TYPE2_TAG   = 0xA9;
    private static final int USIM_TYPE3_TAG   = 0xAA;
    private static final int USIM_EFADN_TAG   = 0xC0;
    private static final int USIM_EFIAP_TAG   = 0xC1;
    private static final int USIM_EFEXT1_TAG  = 0xC2;
    private static final int USIM_EFSNE_TAG   = 0xC3;
    private static final int USIM_EFANR_TAG   = 0xC4;
    private static final int USIM_EFPBC_TAG   = 0xC5;
    private static final int USIM_EFGRP_TAG   = 0xC6;
    private static final int USIM_EFAAS_TAG   = 0xC7;
    private static final int USIM_EFGSD_TAG   = 0xC8;
    private static final int USIM_EFUID_TAG   = 0xC9;
    private static final int USIM_EFEMAIL_TAG = 0xCA;
    private static final int USIM_EFCCP1_TAG  = 0xCB;

    private static final int INVALID_SFI = -1;
    private static final byte INVALID_BYTE = -1;

    // class File represent a PBR record TLV object which points to the rest of the phonebook EFs
    private class File {
        // Phonebook reference file constructed tag defined in 3GPP TS 31.102
        // section 4.4.2.1 table 4.1
        private final int mParentTag;
        // EFID of the file
        private final int mEfid;
        // SFI (Short File Identification) of the file. 0xFF indicates invalid SFI.
        private final int mSfi;
        // The order of this tag showing in the PBR record.
        private final int mIndex;

        File(int parentTag, int efid, int sfi, int index) {
            mParentTag = parentTag;
            mEfid = efid;
            mSfi = sfi;
            mIndex = index;
        }

        public int getParentTag() { return mParentTag; }
        public int getEfid() { return mEfid; }
        public int getSfi() { return mSfi; }
        public int getIndex() { return mIndex; }
    }

    public UsimPhoneBookManager(IccFileHandler fh, AdnRecordCache cache) {
        mFh = fh;
        mPhoneBookRecords = new ArrayList<AdnRecord>();
        mPbrRecords = null;
        // We assume its present, after the first read this is updated.
        // So we don't have to read from UICC if its not present on subsequent reads.
        mIsPbrPresent = true;
        mAdnCache = cache;
        mEmailsForAdnRec = new SparseArray<ArrayList<String>>();
        mSfiEfidTable = new SparseIntArray();
    }

    public void reset() {
        mPhoneBookRecords.clear();
        mIapFileRecord = null;
        mEmailFileRecord = null;
        mPbrRecords = null;
        mIsPbrPresent = true;
        mRefreshCache = false;
        mEmailsForAdnRec.clear();
        mSfiEfidTable.clear();
    }

    // Load all phonebook related EFs from the SIM.
    public ArrayList<AdnRecord> loadEfFilesFromUsim() {
        synchronized (mLock) {
            if (!mPhoneBookRecords.isEmpty()) {
                if (mRefreshCache) {
                    mRefreshCache = false;
                    refreshCache();
                }
                return mPhoneBookRecords;
            }

            if (!mIsPbrPresent) return null;

            // Check if the PBR file is present in the cache, if not read it
            // from the USIM.
            if (mPbrRecords == null) {
                readPbrFileAndWait();
            }

            if (mPbrRecords == null)
                return null;

            int numRecs = mPbrRecords.size();

            log("loadEfFilesFromUsim: Loading adn and emails");
            for (int i = 0; i < numRecs; i++) {
                readAdnFileAndWait(i);
                readEmailFileAndWait(i);
            }

            updatePhoneAdnRecord();
            // All EF files are loaded, return all the records
        }
        return mPhoneBookRecords;
    }

    // Refresh the phonebook cache.
    private void refreshCache() {
        if (mPbrRecords == null) return;
        mPhoneBookRecords.clear();

        int numRecs = mPbrRecords.size();
        for (int i = 0; i < numRecs; i++) {
            readAdnFileAndWait(i);
        }
    }

    // Invalidate the phonebook cache.
    public void invalidateCache() {
        mRefreshCache = true;
    }

    // Read the phonebook reference file EF_PBR.
    private void readPbrFileAndWait() {
        mFh.loadEFLinearFixedAll(EF_PBR, obtainMessage(EVENT_PBR_LOAD_DONE));
        try {
            mLock.wait();
        } catch (InterruptedException e) {
            Rlog.e(LOG_TAG, "Interrupted Exception in readAdnFileAndWait");
        }
    }

    // Read EF_EMAIL which contains the email records.
    private void readEmailFileAndWait(int recId) {
        SparseArray<File> files;
        files = mPbrRecords.get(recId).mFileIds;
        if (files == null) return;

        File email = files.get(USIM_EFEMAIL_TAG);
        if (email != null) {

            /**
             * Check if the EF_EMAIL is a Type 1 file or a type 2 file.
             * If mEmailPresentInIap is true, its a type 2 file.
             * So we read the IAP file and then read the email records.
             * instead of reading directly.
             */
            if (email.getParentTag() == USIM_TYPE2_TAG) {
                if (files.get(USIM_EFIAP_TAG) == null) {
                    Rlog.e(LOG_TAG, "Can't locate EF_IAP in EF_PBR.");
                    return;
                }

                log("EF_IAP exists. Loading EF_IAP to retrieve the index.");
                readIapFileAndWait(files.get(USIM_EFIAP_TAG).getEfid());
                if (mIapFileRecord == null) {
                    Rlog.e(LOG_TAG, "Error: IAP file is empty");
                    return;
                }

                log("EF_EMAIL order in PBR record: " + email.getIndex());
            }

            int emailEfid = email.getEfid();
            log("EF_EMAIL exists in PBR. efid = 0x" +
                    Integer.toHexString(emailEfid).toUpperCase());

            /**
             * Make sure this EF_EMAIL was never read earlier. Sometimes two PBR record points
             */
            // to the same EF_EMAIL
            for (int i = 0; i < recId; i++) {
                if (mPbrRecords.get(i) != null) {
                    SparseArray<File> previousFileIds = mPbrRecords.get(i).mFileIds;
                    if (previousFileIds != null) {
                        File id = previousFileIds.get(USIM_EFEMAIL_TAG);
                        if (id != null && id.getEfid() == emailEfid) {
                            log("Skipped this EF_EMAIL which was loaded earlier");
                            return;
                        }
                    }
                }
            }

            // Read the EFEmail file.
            mFh.loadEFLinearFixedAll(emailEfid,
                    obtainMessage(EVENT_EMAIL_LOAD_DONE));
            try {
                mLock.wait();
            } catch (InterruptedException e) {
                Rlog.e(LOG_TAG, "Interrupted Exception in readEmailFileAndWait");
            }

            if (mEmailFileRecord == null) {
                Rlog.e(LOG_TAG, "Error: Email file is empty");
                return;
            }

            // Build email list
            if (email.getParentTag() == USIM_TYPE2_TAG && mIapFileRecord != null) {
                // If the tag is type 2 and EF_IAP exists, we need to build tpe 2 email list
                buildType2EmailList(recId);
            }
            else {
                // If one the followings is true, we build type 1 email list
                // 1. EF_IAP does not exist or it is failed to load
                // 2. ICC cards can be made such that they have an IAP file but all
                //    records are empty. In that case buildType2EmailList will fail and
                //    we need to build type 1 email list.

                // Build type 1 email list
                buildType1EmailList(recId);
            }
        }
    }

    // Build type 1 email list
    private void buildType1EmailList(int recId) {
        /**
         * If this is type 1, the number of records in EF_EMAIL would be same as the record number
         * in the master/reference file.
         */
        if (mPbrRecords.get(recId) == null)
            return;

        int numRecs = mPbrRecords.get(recId).mMasterFileRecordNum;
        log("Building type 1 email list. recId = "
                + recId + ", numRecs = " + numRecs);

        byte[] emailRec;
        for (int i = 0; i < numRecs; i++) {
            try {
                emailRec = mEmailFileRecord.get(i);
            } catch (IndexOutOfBoundsException e) {
                Rlog.e(LOG_TAG, "Error: Improper ICC card: No email record for ADN, continuing");
                break;
            }

            /**
             *  3GPP TS 31.102 4.4.2.13 EF_EMAIL (e-mail address)
             *
             *  The fields below are mandatory if and only if the file
             *  is not type 1 (as specified in EF_PBR)
             *
             *  Byte [X + 1]: ADN file SFI (Short File Identification)
             *  Byte [X + 2]: ADN file Record Identifier
             */
            int sfi = emailRec[emailRec.length - 2];
            int adnRecId = emailRec[emailRec.length - 1];

            String email = readEmailRecord(i);

            if (email == null || email.equals("")) {
                continue;
            }

            // Get the associated ADN's efid first.
            int adnEfid = 0;
            if (sfi == INVALID_SFI || mSfiEfidTable.get(sfi) == 0) {

                // If SFI is invalid or cannot be mapped to any ADN, use the ADN's efid
                // in the same PBR files.
                File file = mPbrRecords.get(recId).mFileIds.get(USIM_EFADN_TAG);
                if (file == null)
                    continue;
                adnEfid = file.getEfid();
            }
            else {
                adnEfid = mSfiEfidTable.get(sfi);
            }
            /**
             * SIM record numbers are 1 based.
             * The key is constructed by efid and record index.
             */
            int index = (((adnEfid & 0xFFFF) << 8) | ((adnRecId - 1) & 0xFF));
            ArrayList<String> emailList = mEmailsForAdnRec.get(index);
            if (emailList == null) {
                emailList = new ArrayList<String>();
            }
            log("Adding email #" + i + " list to index 0x" +
                    Integer.toHexString(index).toUpperCase());
            emailList.add(email);
            mEmailsForAdnRec.put(index, emailList);
        }
    }

    // Build type 2 email list
    private boolean buildType2EmailList(int recId) {

        if (mPbrRecords.get(recId) == null)
            return false;

        int numRecs = mPbrRecords.get(recId).mMasterFileRecordNum;
        log("Building type 2 email list. recId = "
                + recId + ", numRecs = " + numRecs);

        /**
         * 3GPP TS 31.102 4.4.2.1 EF_PBR (Phone Book Reference file) table 4.1

         * The number of records in the IAP file is same as the number of records in the master
         * file (e.g EF_ADN). The order of the pointers in an EF_IAP shall be the same as the
         * order of file IDs that appear in the TLV object indicated by Tag 'A9' in the
         * reference file record (e.g value of mEmailTagNumberInIap)
         */

        File adnFile = mPbrRecords.get(recId).mFileIds.get(USIM_EFADN_TAG);
        if (adnFile == null) {
            Rlog.e(LOG_TAG, "Error: Improper ICC card: EF_ADN does not exist in PBR files");
            return false;
        }
        int adnEfid = adnFile.getEfid();

        for (int i = 0; i < numRecs; i++) {
            byte[] record;
            int emailRecId;
            try {
                record = mIapFileRecord.get(i);
                emailRecId =
                        record[mPbrRecords.get(recId).mFileIds.get(USIM_EFEMAIL_TAG).getIndex()];
            } catch (IndexOutOfBoundsException e) {
                Rlog.e(LOG_TAG, "Error: Improper ICC card: Corrupted EF_IAP");
                continue;
            }

            String email = readEmailRecord(emailRecId - 1);
            if (email != null && !email.equals("")) {
                // The key is constructed by efid and record index.
                int index = (((adnEfid & 0xFFFF) << 8) | (i & 0xFF));
                ArrayList<String> emailList = mEmailsForAdnRec.get(index);
                if (emailList == null) {
                    emailList = new ArrayList<String>();
                }
                emailList.add(email);
                log("Adding email list to index 0x" +
                        Integer.toHexString(index).toUpperCase());
                mEmailsForAdnRec.put(index, emailList);
            }
        }
        return true;
    }

    // Read Phonebook Index Admistration EF_IAP file
    private void readIapFileAndWait(int efid) {
        mFh.loadEFLinearFixedAll(efid, obtainMessage(EVENT_IAP_LOAD_DONE));
        try {
            mLock.wait();
        } catch (InterruptedException e) {
            Rlog.e(LOG_TAG, "Interrupted Exception in readIapFileAndWait");
        }
    }

    private void updatePhoneAdnRecord() {

        int numAdnRecs = mPhoneBookRecords.size();

        for (int i = 0; i < numAdnRecs; i++) {

            AdnRecord rec = mPhoneBookRecords.get(i);

            int adnEfid = rec.getEfid();
            int adnRecId = rec.getRecId();

            int index = (((adnEfid & 0xFFFF) << 8) | ((adnRecId - 1) & 0xFF));

            ArrayList<String> emailList;
            try {
                emailList = mEmailsForAdnRec.get(index);
            } catch (IndexOutOfBoundsException e) {
                continue;
            }

            if (emailList == null)
                continue;

            String[] emails = new String[emailList.size()];
            System.arraycopy(emailList.toArray(), 0, emails, 0, emailList.size());
            rec.setEmails(emails);
            log("Adding email list to ADN (0x" +
                    Integer.toHexString(mPhoneBookRecords.get(i).getEfid()).toUpperCase() +
                    ") record #" + mPhoneBookRecords.get(i).getRecId());
            mPhoneBookRecords.set(i, rec);
        }
    }

    // Read email from the record of EF_EMAIL
    private String readEmailRecord(int recId) {
        byte[] emailRec;
        try {
            emailRec = mEmailFileRecord.get(recId);
        } catch (IndexOutOfBoundsException e) {
            return null;
        }

        // The length of the record is X+2 byte, where X bytes is the email address
        return IccUtils.adnStringFieldToString(emailRec, 0, emailRec.length - 2);
    }

    // Read EF_ADN file
    private void readAdnFileAndWait(int recId) {
        SparseArray<File> files;
        files = mPbrRecords.get(recId).mFileIds;
        if (files == null || files.size() == 0) return;

        int extEf = 0;
        // Only call fileIds.get while EF_EXT1_TAG is available
        if (files.get(USIM_EFEXT1_TAG) != null) {
            extEf = files.get(USIM_EFEXT1_TAG).getEfid();
        }

        if (files.get(USIM_EFADN_TAG) == null)
            return;

        int previousSize = mPhoneBookRecords.size();
        mAdnCache.requestLoadAllAdnLike(files.get(USIM_EFADN_TAG).getEfid(),
            extEf, obtainMessage(EVENT_USIM_ADN_LOAD_DONE));
        try {
            mLock.wait();
        } catch (InterruptedException e) {
            Rlog.e(LOG_TAG, "Interrupted Exception in readAdnFileAndWait");
        }

        /**
         * The recent added ADN record # would be the reference record size
         * for the rest of EFs associated within this PBR.
         */
        mPbrRecords.get(recId).mMasterFileRecordNum = mPhoneBookRecords.size() - previousSize;
    }

    // Create the phonebook reference file based on EF_PBR
    private void createPbrFile(ArrayList<byte[]> records) {
        if (records == null) {
            mPbrRecords = null;
            mIsPbrPresent = false;
            return;
        }

        mPbrRecords = new ArrayList<PbrRecord>();
        for (int i = 0; i < records.size(); i++) {
            // Some cards have two records but the 2nd record is filled with all invalid char 0xff.
            // So we need to check if the record is valid or not before adding into the PBR records.
            if (records.get(i)[0] != INVALID_BYTE) {
                mPbrRecords.add(new PbrRecord(records.get(i)));
            }
        }

        for (PbrRecord record : mPbrRecords) {
            File file = record.mFileIds.get(USIM_EFADN_TAG);
            // If the file does not contain EF_ADN, we'll just skip it.
            if (file != null) {
                int sfi = file.getSfi();
                if (sfi != INVALID_SFI) {
                    mSfiEfidTable.put(sfi, record.mFileIds.get(USIM_EFADN_TAG).getEfid());
                }
            }
        }
    }

    @Override
    public void handleMessage(Message msg) {
        AsyncResult ar;

        switch(msg.what) {
        case EVENT_PBR_LOAD_DONE:
            ar = (AsyncResult) msg.obj;
            if (ar.exception == null) {
                createPbrFile((ArrayList<byte[]>)ar.result);
            }
            synchronized (mLock) {
                mLock.notify();
            }
            break;
        case EVENT_USIM_ADN_LOAD_DONE:
            log("Loading USIM ADN records done");
            ar = (AsyncResult) msg.obj;
            if (ar.exception == null) {
                mPhoneBookRecords.addAll((ArrayList<AdnRecord>)ar.result);
            }
            synchronized (mLock) {
                mLock.notify();
            }
            break;
        case EVENT_IAP_LOAD_DONE:
            log("Loading USIM IAP records done");
            ar = (AsyncResult) msg.obj;
            if (ar.exception == null) {
                mIapFileRecord = ((ArrayList<byte[]>)ar.result);
            }
            synchronized (mLock) {
                mLock.notify();
            }
            break;
        case EVENT_EMAIL_LOAD_DONE:
            log("Loading USIM Email records done");
            ar = (AsyncResult) msg.obj;
            if (ar.exception == null) {
                mEmailFileRecord = ((ArrayList<byte[]>)ar.result);
            }

            synchronized (mLock) {
                mLock.notify();
            }
            break;
        }
    }

    // PbrRecord represents a record in EF_PBR
    private class PbrRecord {
        // TLV tags
        private SparseArray<File> mFileIds;

        /**
         * 3GPP TS 31.102 4.4.2.1 EF_PBR (Phone Book Reference file)
         * If this is type 1 files, files that contain as many records as the
         * reference/master file (EF_ADN, EF_ADN1) and are linked on record number
         * bases (Rec1 -> Rec1). The master file record number is the reference.
         */
        private int mMasterFileRecordNum;

        PbrRecord(byte[] record) {
            mFileIds = new SparseArray<File>();
            SimTlv recTlv;
            log("PBR rec: " + IccUtils.bytesToHexString(record));
            recTlv = new SimTlv(record, 0, record.length);
            parseTag(recTlv);
        }

        void parseTag(SimTlv tlv) {
            SimTlv tlvEfSfi;
            int tag;
            byte[] data;

            do {
                tag = tlv.getTag();
                switch(tag) {
                case USIM_TYPE1_TAG: // A8
                case USIM_TYPE3_TAG: // AA
                case USIM_TYPE2_TAG: // A9
                    data = tlv.getData();
                    tlvEfSfi = new SimTlv(data, 0, data.length);
                    parseEfAndSFI(tlvEfSfi, tag);
                    break;
                }
            } while (tlv.nextObject());
        }

        void parseEfAndSFI(SimTlv tlv, int parentTag) {
            int tag;
            byte[] data;
            int tagNumberWithinParentTag = 0;
            do {
                tag = tlv.getTag();
                switch(tag) {
                    case USIM_EFEMAIL_TAG:
                    case USIM_EFADN_TAG:
                    case USIM_EFEXT1_TAG:
                    case USIM_EFANR_TAG:
                    case USIM_EFPBC_TAG:
                    case USIM_EFGRP_TAG:
                    case USIM_EFAAS_TAG:
                    case USIM_EFGSD_TAG:
                    case USIM_EFUID_TAG:
                    case USIM_EFCCP1_TAG:
                    case USIM_EFIAP_TAG:
                    case USIM_EFSNE_TAG:
                        /** 3GPP TS 31.102, 4.4.2.1 EF_PBR (Phone Book Reference file)
                         *
                         * The SFI value assigned to an EF which is indicated in EF_PBR shall
                         * correspond to the SFI indicated in the TLV object in EF_PBR.

                         * The primitive tag identifies clearly the type of data, its value
                         * field indicates the file identifier and, if applicable, the SFI
                         * value of the specified EF. That is, the length value of a primitive
                         * tag indicates if an SFI value is available for the EF or not:
                         * - Length = '02' Value: 'EFID (2 bytes)'
                         * - Length = '03' Value: 'EFID (2 bytes)', 'SFI (1 byte)'
                         */

                        int sfi = INVALID_SFI;
                        data = tlv.getData();

                        if (data.length < 2 || data.length > 3) {
                            log("Invalid TLV length: " + data.length);
                            break;
                        }

                        if (data.length == 3) {
                            sfi = data[2] & 0xFF;
                        }

                        int efid = ((data[0] & 0xFF) << 8) | (data[1] & 0xFF);

                        mFileIds.put(tag, new File(parentTag, efid, sfi, tagNumberWithinParentTag));
                        break;
                }
                tagNumberWithinParentTag++;
            } while(tlv.nextObject());
        }
    }

    private void log(String msg) {
        if(DBG) Rlog.d(LOG_TAG, msg);
    }
}