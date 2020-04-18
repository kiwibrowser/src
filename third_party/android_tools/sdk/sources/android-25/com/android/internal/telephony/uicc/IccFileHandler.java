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

import android.os.*;
import com.android.internal.telephony.CommandsInterface;

import java.util.ArrayList;

/**
 * {@hide}
 */
public abstract class IccFileHandler extends Handler implements IccConstants {

    //from TS 11.11 9.1 or elsewhere
    static protected final int COMMAND_READ_BINARY = 0xb0;
    static protected final int COMMAND_UPDATE_BINARY = 0xd6;
    static protected final int COMMAND_READ_RECORD = 0xb2;
    static protected final int COMMAND_UPDATE_RECORD = 0xdc;
    static protected final int COMMAND_SEEK = 0xa2;
    static protected final int COMMAND_GET_RESPONSE = 0xc0;

    // from TS 11.11 9.2.5
    static protected final int READ_RECORD_MODE_ABSOLUTE = 4;

    //***** types of files  TS 11.11 9.3
    static protected final int EF_TYPE_TRANSPARENT = 0;
    static protected final int EF_TYPE_LINEAR_FIXED = 1;
    static protected final int EF_TYPE_CYCLIC = 3;

    //***** types of files  TS 11.11 9.3
    static protected final int TYPE_RFU = 0;
    static protected final int TYPE_MF  = 1;
    static protected final int TYPE_DF  = 2;
    static protected final int TYPE_EF  = 4;

    // size of GET_RESPONSE for EF's
    static protected final int GET_RESPONSE_EF_SIZE_BYTES = 15;
    static protected final int GET_RESPONSE_EF_IMG_SIZE_BYTES = 10;

    // Byte order received in response to COMMAND_GET_RESPONSE
    // Refer TS 51.011 Section 9.2.1
    static protected final int RESPONSE_DATA_RFU_1 = 0;
    static protected final int RESPONSE_DATA_RFU_2 = 1;

    static protected final int RESPONSE_DATA_FILE_SIZE_1 = 2;
    static protected final int RESPONSE_DATA_FILE_SIZE_2 = 3;

    static protected final int RESPONSE_DATA_FILE_ID_1 = 4;
    static protected final int RESPONSE_DATA_FILE_ID_2 = 5;
    static protected final int RESPONSE_DATA_FILE_TYPE = 6;
    static protected final int RESPONSE_DATA_RFU_3 = 7;
    static protected final int RESPONSE_DATA_ACCESS_CONDITION_1 = 8;
    static protected final int RESPONSE_DATA_ACCESS_CONDITION_2 = 9;
    static protected final int RESPONSE_DATA_ACCESS_CONDITION_3 = 10;
    static protected final int RESPONSE_DATA_FILE_STATUS = 11;
    static protected final int RESPONSE_DATA_LENGTH = 12;
    static protected final int RESPONSE_DATA_STRUCTURE = 13;
    static protected final int RESPONSE_DATA_RECORD_LENGTH = 14;


    //***** Events

    /** Finished retrieving size of transparent EF; start loading. */
    static protected final int EVENT_GET_BINARY_SIZE_DONE = 4;
    /** Finished loading contents of transparent EF; post result. */
    static protected final int EVENT_READ_BINARY_DONE = 5;
    /** Finished retrieving size of records for linear-fixed EF; now load. */
    static protected final int EVENT_GET_RECORD_SIZE_DONE = 6;
    /** Finished loading single record from a linear-fixed EF; post result. */
    static protected final int EVENT_READ_RECORD_DONE = 7;
    /** Finished retrieving record size; post result. */
    static protected final int EVENT_GET_EF_LINEAR_RECORD_SIZE_DONE = 8;
    /** Finished retrieving image instance record; post result. */
    static protected final int EVENT_READ_IMG_DONE = 9;
    /** Finished retrieving icon data; post result. */
    static protected final int EVENT_READ_ICON_DONE = 10;
    /** Finished retrieving size of record for EFimg now. */
    static protected final int EVENT_GET_RECORD_SIZE_IMG_DONE = 11;

     // member variables
    protected final CommandsInterface mCi;
    protected final UiccCardApplication mParentApp;
    protected final String mAid;

    static class LoadLinearFixedContext {

        int mEfid;
        int mRecordNum, mRecordSize, mCountRecords;
        boolean mLoadAll;
        String mPath;

        Message mOnLoaded;

        ArrayList<byte[]> results;

        LoadLinearFixedContext(int efid, int recordNum, Message onLoaded) {
            mEfid = efid;
            mRecordNum = recordNum;
            mOnLoaded = onLoaded;
            mLoadAll = false;
            mPath = null;
        }

        LoadLinearFixedContext(int efid, int recordNum, String path, Message onLoaded) {
            mEfid = efid;
            mRecordNum = recordNum;
            mOnLoaded = onLoaded;
            mLoadAll = false;
            mPath = path;
        }

        LoadLinearFixedContext(int efid, String path, Message onLoaded) {
            mEfid = efid;
            mRecordNum = 1;
            mLoadAll = true;
            mOnLoaded = onLoaded;
            mPath = path;
        }

        LoadLinearFixedContext(int efid, Message onLoaded) {
            mEfid = efid;
            mRecordNum = 1;
            mLoadAll = true;
            mOnLoaded = onLoaded;
            mPath = null;
        }
    }

    /**
     * Default constructor
     */
    protected IccFileHandler(UiccCardApplication app, String aid, CommandsInterface ci) {
        mParentApp = app;
        mAid = aid;
        mCi = ci;
    }

    public void dispose() {
    }

    //***** Public Methods

    /**
     * Load a record from a SIM Linear Fixed EF
     *
     * @param fileid EF id
     * @param path Path of the EF on the card
     * @param recordNum 1-based (not 0-based) record number
     * @param onLoaded
     *
     * ((AsyncResult)(onLoaded.obj)).result is the byte[]
     *
     */
    public void loadEFLinearFixed(int fileid, String path, int recordNum, Message onLoaded) {
        String efPath = (path == null) ? getEFPath(fileid) : path;
        Message response
                = obtainMessage(EVENT_GET_RECORD_SIZE_DONE,
                        new LoadLinearFixedContext(fileid, recordNum, efPath, onLoaded));

        mCi.iccIOForApp(COMMAND_GET_RESPONSE, fileid, efPath,
                        0, 0, GET_RESPONSE_EF_SIZE_BYTES, null, null, mAid, response);
    }

    /**
     * Load a record from a SIM Linear Fixed EF
     *
     * @param fileid EF id
     * @param recordNum 1-based (not 0-based) record number
     * @param onLoaded
     *
     * ((AsyncResult)(onLoaded.obj)).result is the byte[]
     *
     */
    public void loadEFLinearFixed(int fileid, int recordNum, Message onLoaded) {
        loadEFLinearFixed(fileid, getEFPath(fileid), recordNum, onLoaded);
    }

    /**
     * Load a image instance record from a SIM Linear Fixed EF-IMG
     *
     * @param recordNum 1-based (not 0-based) record number
     * @param onLoaded
     *
     * ((AsyncResult)(onLoaded.obj)).result is the byte[]
     *
     */
    public void loadEFImgLinearFixed(int recordNum, Message onLoaded) {
        Message response = obtainMessage(EVENT_GET_RECORD_SIZE_IMG_DONE,
                new LoadLinearFixedContext(IccConstants.EF_IMG, recordNum,
                        onLoaded));

        mCi.iccIOForApp(COMMAND_GET_RESPONSE, IccConstants.EF_IMG,
                    getEFPath(IccConstants.EF_IMG), recordNum,
                    READ_RECORD_MODE_ABSOLUTE, GET_RESPONSE_EF_IMG_SIZE_BYTES,
                    null, null, mAid, response);
    }

    /**
     * get record size for a linear fixed EF
     *
     * @param fileid EF id
     * @param path Path of the EF on the card
     * @param onLoaded ((AsnyncResult)(onLoaded.obj)).result is the recordSize[]
     *        int[0] is the record length int[1] is the total length of the EF
     *        file int[3] is the number of records in the EF file So int[0] *
     *        int[3] = int[1]
     */
    public void getEFLinearRecordSize(int fileid, String path, Message onLoaded) {
        String efPath = (path == null) ? getEFPath(fileid) : path;
        Message response
                = obtainMessage(EVENT_GET_EF_LINEAR_RECORD_SIZE_DONE,
                        new LoadLinearFixedContext(fileid, efPath, onLoaded));
        mCi.iccIOForApp(COMMAND_GET_RESPONSE, fileid, efPath,
                    0, 0, GET_RESPONSE_EF_SIZE_BYTES, null, null, mAid, response);
    }

    /**
     * get record size for a linear fixed EF
     *
     * @param fileid EF id
     * @param onLoaded ((AsnyncResult)(onLoaded.obj)).result is the recordSize[]
     *        int[0] is the record length int[1] is the total length of the EF
     *        file int[3] is the number of records in the EF file So int[0] *
     *        int[3] = int[1]
     */
    public void getEFLinearRecordSize(int fileid, Message onLoaded) {
        getEFLinearRecordSize(fileid, getEFPath(fileid), onLoaded);
    }

    /**
     * Load all records from a SIM Linear Fixed EF
     *
     * @param fileid EF id
     * @param path Path of the EF on the card
     * @param onLoaded
     *
     * ((AsyncResult)(onLoaded.obj)).result is an ArrayList<byte[]>
     *
     */
    public void loadEFLinearFixedAll(int fileid, String path, Message onLoaded) {
        String efPath = (path == null) ? getEFPath(fileid) : path;
        Message response = obtainMessage(EVENT_GET_RECORD_SIZE_DONE,
                        new LoadLinearFixedContext(fileid, efPath, onLoaded));

        mCi.iccIOForApp(COMMAND_GET_RESPONSE, fileid, efPath,
                        0, 0, GET_RESPONSE_EF_SIZE_BYTES, null, null, mAid, response);
    }

    /**
     * Load all records from a SIM Linear Fixed EF
     *
     * @param fileid EF id
     * @param onLoaded
     *
     * ((AsyncResult)(onLoaded.obj)).result is an ArrayList<byte[]>
     *
     */
    public void loadEFLinearFixedAll(int fileid, Message onLoaded) {
        loadEFLinearFixedAll(fileid, getEFPath(fileid), onLoaded);
    }

    /**
     * Load a SIM Transparent EF
     *
     * @param fileid EF id
     * @param onLoaded
     *
     * ((AsyncResult)(onLoaded.obj)).result is the byte[]
     *
     */

    public void loadEFTransparent(int fileid, Message onLoaded) {
        Message response = obtainMessage(EVENT_GET_BINARY_SIZE_DONE,
                        fileid, 0, onLoaded);

        mCi.iccIOForApp(COMMAND_GET_RESPONSE, fileid, getEFPath(fileid),
                        0, 0, GET_RESPONSE_EF_SIZE_BYTES, null, null, mAid, response);
    }

    /**
     * Load first @size bytes from SIM Transparent EF
     *
     * @param fileid EF id
     * @param size
     * @param onLoaded
     *
     * ((AsyncResult)(onLoaded.obj)).result is the byte[]
     *
     */
    public void loadEFTransparent(int fileid, int size, Message onLoaded) {
        Message response = obtainMessage(EVENT_READ_BINARY_DONE,
                        fileid, 0, onLoaded);

        mCi.iccIOForApp(COMMAND_READ_BINARY, fileid, getEFPath(fileid),
                        0, 0, size, null, null, mAid, response);
    }

    /**
     * Load a SIM Transparent EF-IMG. Used right after loadEFImgLinearFixed to
     * retrive STK's icon data.
     *
     * @param fileid EF id
     * @param onLoaded
     *
     * ((AsyncResult)(onLoaded.obj)).result is the byte[]
     *
     */
    public void loadEFImgTransparent(int fileid, int highOffset, int lowOffset,
            int length, Message onLoaded) {
        Message response = obtainMessage(EVENT_READ_ICON_DONE, fileid, 0,
                onLoaded);

        logd("IccFileHandler: loadEFImgTransparent fileid = " + fileid
                + " filePath = " + getEFPath(EF_IMG) + " highOffset = " + highOffset
                + " lowOffset = " + lowOffset + " length = " + length);

        /* Per TS 31.102, for displaying of Icon, under
         * DF Telecom and DF Graphics , EF instance(s) (4FXX,transparent files)
         * are present. The possible image file identifiers (EF instance) for
         * EF img ( 4F20, linear fixed file) are : 4F01 ... 4F05.
         * It should be MF_SIM + DF_TELECOM + DF_GRAPHICS, same path as EF IMG
         */
        mCi.iccIOForApp(COMMAND_READ_BINARY, fileid, getEFPath(EF_IMG),
                highOffset, lowOffset, length, null, null, mAid, response);
    }

    /**
     * Update a record in a linear fixed EF
     * @param fileid EF id
     * @param path Path of the EF on the card
     * @param recordNum 1-based (not 0-based) record number
     * @param data must be exactly as long as the record in the EF
     * @param pin2 for CHV2 operations, otherwist must be null
     * @param onComplete onComplete.obj will be an AsyncResult
     *                   onComplete.obj.userObj will be a IccIoResult on success
     */
    public void updateEFLinearFixed(int fileid, String path, int recordNum, byte[] data,
            String pin2, Message onComplete) {
        String efPath = (path == null) ? getEFPath(fileid) : path;
        mCi.iccIOForApp(COMMAND_UPDATE_RECORD, fileid, efPath,
                        recordNum, READ_RECORD_MODE_ABSOLUTE, data.length,
                        IccUtils.bytesToHexString(data), pin2, mAid, onComplete);
    }

    /**
     * Update a record in a linear fixed EF
     * @param fileid EF id
     * @param recordNum 1-based (not 0-based) record number
     * @param data must be exactly as long as the record in the EF
     * @param pin2 for CHV2 operations, otherwist must be null
     * @param onComplete onComplete.obj will be an AsyncResult
     *                   onComplete.obj.userObj will be a IccIoResult on success
     */
    public void updateEFLinearFixed(int fileid, int recordNum, byte[] data,
            String pin2, Message onComplete) {
        mCi.iccIOForApp(COMMAND_UPDATE_RECORD, fileid, getEFPath(fileid),
                        recordNum, READ_RECORD_MODE_ABSOLUTE, data.length,
                        IccUtils.bytesToHexString(data), pin2, mAid, onComplete);
    }

    /**
     * Update a transparent EF
     * @param fileid EF id
     * @param data must be exactly as long as the EF
     */
    public void updateEFTransparent(int fileid, byte[] data, Message onComplete) {
        mCi.iccIOForApp(COMMAND_UPDATE_BINARY, fileid, getEFPath(fileid),
                        0, 0, data.length,
                        IccUtils.bytesToHexString(data), null, mAid, onComplete);
    }


    //***** Abstract Methods


    //***** Private Methods

    private void sendResult(Message response, Object result, Throwable ex) {
        if (response == null) {
            return;
        }

        AsyncResult.forMessage(response, result, ex);

        response.sendToTarget();
    }

    private boolean processException(Message response, AsyncResult ar) {
        IccException iccException;
        boolean flag = false;
        IccIoResult result = (IccIoResult) ar.result;
        if (ar.exception != null) {
            sendResult(response, null, ar.exception);
            flag = true;
        } else {
            iccException = result.getException();
            if (iccException != null) {
                sendResult(response, null, iccException);
                flag = true;
            }
        }
        return flag;
    }

    //***** Overridden from Handler

    @Override
    public void handleMessage(Message msg) {
        AsyncResult ar;
        IccIoResult result;
        Message response = null;
        String str;
        LoadLinearFixedContext lc;

        byte data[];
        int size;
        int fileid;
        int recordSize[];
        String path = null;

        try {
            switch (msg.what) {
            case EVENT_GET_EF_LINEAR_RECORD_SIZE_DONE:
                ar = (AsyncResult)msg.obj;
                lc = (LoadLinearFixedContext) ar.userObj;
                result = (IccIoResult) ar.result;
                response = lc.mOnLoaded;

                if (processException(response, (AsyncResult) msg.obj)) {
                    break;
                }

                data = result.payload;

                if (TYPE_EF != data[RESPONSE_DATA_FILE_TYPE] ||
                    EF_TYPE_LINEAR_FIXED != data[RESPONSE_DATA_STRUCTURE]) {
                    throw new IccFileTypeMismatch();
                }

                recordSize = new int[3];
                recordSize[0] = data[RESPONSE_DATA_RECORD_LENGTH] & 0xFF;
                recordSize[1] = ((data[RESPONSE_DATA_FILE_SIZE_1] & 0xff) << 8)
                       + (data[RESPONSE_DATA_FILE_SIZE_2] & 0xff);
                recordSize[2] = recordSize[1] / recordSize[0];

                sendResult(response, recordSize, null);
                break;

             case EVENT_GET_RECORD_SIZE_IMG_DONE:
             case EVENT_GET_RECORD_SIZE_DONE:
                ar = (AsyncResult)msg.obj;
                lc = (LoadLinearFixedContext) ar.userObj;
                result = (IccIoResult) ar.result;
                response = lc.mOnLoaded;

                if (processException(response, (AsyncResult) msg.obj)) {
                    break;
                }

                data = result.payload;
                path = lc.mPath;

                if (TYPE_EF != data[RESPONSE_DATA_FILE_TYPE]) {
                    throw new IccFileTypeMismatch();
                }

                if (EF_TYPE_LINEAR_FIXED != data[RESPONSE_DATA_STRUCTURE]) {
                    throw new IccFileTypeMismatch();
                }

                lc.mRecordSize = data[RESPONSE_DATA_RECORD_LENGTH] & 0xFF;

                size = ((data[RESPONSE_DATA_FILE_SIZE_1] & 0xff) << 8)
                       + (data[RESPONSE_DATA_FILE_SIZE_2] & 0xff);

                lc.mCountRecords = size / lc.mRecordSize;

                 if (lc.mLoadAll) {
                     lc.results = new ArrayList<byte[]>(lc.mCountRecords);
                 }

                 if (path == null) {
                     path = getEFPath(lc.mEfid);
                 }
                 mCi.iccIOForApp(COMMAND_READ_RECORD, lc.mEfid, path,
                         lc.mRecordNum,
                         READ_RECORD_MODE_ABSOLUTE,
                         lc.mRecordSize, null, null, mAid,
                         obtainMessage(EVENT_READ_RECORD_DONE, lc));
                 break;
            case EVENT_GET_BINARY_SIZE_DONE:
                ar = (AsyncResult)msg.obj;
                response = (Message) ar.userObj;
                result = (IccIoResult) ar.result;

                if (processException(response, (AsyncResult) msg.obj)) {
                    break;
                }

                data = result.payload;

                fileid = msg.arg1;

                if (TYPE_EF != data[RESPONSE_DATA_FILE_TYPE]) {
                    throw new IccFileTypeMismatch();
                }

                if (EF_TYPE_TRANSPARENT != data[RESPONSE_DATA_STRUCTURE]) {
                    throw new IccFileTypeMismatch();
                }

                size = ((data[RESPONSE_DATA_FILE_SIZE_1] & 0xff) << 8)
                       + (data[RESPONSE_DATA_FILE_SIZE_2] & 0xff);

                mCi.iccIOForApp(COMMAND_READ_BINARY, fileid, getEFPath(fileid),
                                0, 0, size, null, null, mAid,
                                obtainMessage(EVENT_READ_BINARY_DONE,
                                              fileid, 0, response));
            break;

            case EVENT_READ_IMG_DONE:
            case EVENT_READ_RECORD_DONE:

                ar = (AsyncResult)msg.obj;
                lc = (LoadLinearFixedContext) ar.userObj;
                result = (IccIoResult) ar.result;
                response = lc.mOnLoaded;
                path = lc.mPath;

                if (processException(response, (AsyncResult) msg.obj)) {
                    break;
                }

                if (!lc.mLoadAll) {
                    sendResult(response, result.payload, null);
                } else {
                    lc.results.add(result.payload);

                    lc.mRecordNum++;

                    if (lc.mRecordNum > lc.mCountRecords) {
                        sendResult(response, lc.results, null);
                    } else {
                        if (path == null) {
                            path = getEFPath(lc.mEfid);
                        }

                        mCi.iccIOForApp(COMMAND_READ_RECORD, lc.mEfid, path,
                                    lc.mRecordNum,
                                    READ_RECORD_MODE_ABSOLUTE,
                                    lc.mRecordSize, null, null, mAid,
                                    obtainMessage(EVENT_READ_RECORD_DONE, lc));
                    }
                }

            break;

            case EVENT_READ_BINARY_DONE:
            case EVENT_READ_ICON_DONE:
                ar = (AsyncResult)msg.obj;
                response = (Message) ar.userObj;
                result = (IccIoResult) ar.result;

                if (processException(response, (AsyncResult) msg.obj)) {
                    break;
                }

                sendResult(response, result.payload, null);
            break;

        }} catch (Exception exc) {
            if (response != null) {
                sendResult(response, null, exc);
            } else {
                loge("uncaught exception" + exc);
            }
        }
    }

    /**
     * Returns the root path of the EF file.
     * i.e returns MasterFile + DFfile as a string.
     * Ex: For EF_ADN on a SIM, it will return "3F007F10"
     * This function handles only EFids that are common to
     * RUIM, SIM, USIM and other types of Icc cards.
     *
     * @param efid of path to retrieve
     * @return root path of the file.
     */
    protected String getCommonIccEFPath(int efid) {
        switch(efid) {
        case EF_ADN:
        case EF_FDN:
        case EF_MSISDN:
        case EF_SDN:
        case EF_EXT1:
        case EF_EXT2:
        case EF_EXT3:
        case EF_PSI:
            return MF_SIM + DF_TELECOM;

        case EF_ICCID:
        case EF_PL:
            return MF_SIM;
        case EF_PBR:
            // we only support global phonebook.
            return MF_SIM + DF_TELECOM + DF_PHONEBOOK;
        case EF_IMG:
            return MF_SIM + DF_TELECOM + DF_GRAPHICS;
        }
        return null;
    }

    protected abstract String getEFPath(int efid);
    protected abstract void logd(String s);

    protected abstract void loge(String s);

}
