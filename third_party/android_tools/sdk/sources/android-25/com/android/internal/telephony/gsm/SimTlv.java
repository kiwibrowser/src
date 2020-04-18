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

package com.android.internal.telephony.gsm;

/**
 * SIM Tag-Length-Value record
 * TS 102 223 Annex C
 *
 * {@hide}
 *
 */
public class SimTlv
{
    //***** Private Instance Variables

    byte mRecord[];
    int mTlvOffset;
    int mTlvLength;
    int mCurOffset;
    int mCurDataOffset;
    int mCurDataLength;
    boolean mHasValidTlvObject;

    public SimTlv(byte[] record, int offset, int length) {
        mRecord = record;

        mTlvOffset = offset;
        mTlvLength = length;
        mCurOffset = offset;

        mHasValidTlvObject = parseCurrentTlvObject();
    }

    public boolean nextObject() {
        if (!mHasValidTlvObject) return false;
        mCurOffset = mCurDataOffset + mCurDataLength;
        mHasValidTlvObject = parseCurrentTlvObject();
        return mHasValidTlvObject;
    }

    public boolean isValidObject() {
        return mHasValidTlvObject;
    }

    /**
     * Returns the tag for the current TLV object
     * Return 0 if !isValidObject()
     * 0 and 0xff are invalid tag values
     * valid tags range from 1 - 0xfe
     */
    public int getTag() {
        if (!mHasValidTlvObject) return 0;
        return mRecord[mCurOffset] & 0xff;
    }

    /**
     * Returns data associated with current TLV object
     * returns null if !isValidObject()
     */

    public byte[] getData() {
        if (!mHasValidTlvObject) return null;

        byte[] ret = new byte[mCurDataLength];
        System.arraycopy(mRecord, mCurDataOffset, ret, 0, mCurDataLength);
        return ret;
    }

    /**
     * Updates curDataLength and curDataOffset
     * @return false on invalid record, true on valid record
     */

    private boolean parseCurrentTlvObject() {
        // 0x00 and 0xff are invalid tag values

        try {
            if (mRecord[mCurOffset] == 0 || (mRecord[mCurOffset] & 0xff) == 0xff) {
                return false;
            }

            if ((mRecord[mCurOffset + 1] & 0xff) < 0x80) {
                // one byte length 0 - 0x7f
                mCurDataLength = mRecord[mCurOffset + 1] & 0xff;
                mCurDataOffset = mCurOffset + 2;
            } else if ((mRecord[mCurOffset + 1] & 0xff) == 0x81) {
                // two byte length 0x80 - 0xff
                mCurDataLength = mRecord[mCurOffset + 2] & 0xff;
                mCurDataOffset = mCurOffset + 3;
            } else {
                return false;
            }
        } catch (ArrayIndexOutOfBoundsException ex) {
            return false;
        }

        if (mCurDataLength + mCurDataOffset > mTlvOffset + mTlvLength) {
            return false;
        }

        return true;
    }

}
