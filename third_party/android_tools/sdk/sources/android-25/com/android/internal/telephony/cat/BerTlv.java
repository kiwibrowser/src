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

package com.android.internal.telephony.cat;

import java.util.List;

/**
 * Class for representing BER-TLV objects.
 *
 * @see "ETSI TS 102 223 Annex C" for more information.
 *
 * {@hide}
 */
class BerTlv {
    private int mTag = BER_UNKNOWN_TAG;
    private List<ComprehensionTlv> mCompTlvs = null;
    private boolean mLengthValid = true;

    public static final int BER_UNKNOWN_TAG             = 0x00;
    public static final int BER_PROACTIVE_COMMAND_TAG   = 0xd0;
    public static final int BER_MENU_SELECTION_TAG      = 0xd3;
    public static final int BER_EVENT_DOWNLOAD_TAG      = 0xd6;

    private BerTlv(int tag, List<ComprehensionTlv> ctlvs, boolean lengthValid) {
        mTag = tag;
        mCompTlvs = ctlvs;
        mLengthValid = lengthValid;
    }

    /**
     * Gets a list of ComprehensionTlv objects contained in this BER-TLV object.
     *
     * @return A list of COMPREHENSION-TLV object
     */
    public List<ComprehensionTlv> getComprehensionTlvs() {
        return mCompTlvs;
    }

    /**
     * Gets a tag id of the BER-TLV object.
     *
     * @return A tag integer.
     */
    public int getTag() {
        return mTag;
    }

    /**
     * Gets if the length of the BER-TLV object is valid
     *
     * @return if length valid
     */
     public boolean isLengthValid() {
         return mLengthValid;
     }

    /**
     * Decodes a BER-TLV object from a byte array.
     *
     * @param data A byte array to decode from
     * @return A BER-TLV object decoded
     * @throws ResultException
     */
    public static BerTlv decode(byte[] data) throws ResultException {
        int curIndex = 0;
        int endIndex = data.length;
        int tag, length = 0;
        boolean isLengthValid = true;

        try {
            /* tag */
            tag = data[curIndex++] & 0xff;
            if (tag == BER_PROACTIVE_COMMAND_TAG) {
                /* length */
                int temp = data[curIndex++] & 0xff;
                if (temp < 0x80) {
                    length = temp;
                } else if (temp == 0x81) {
                    temp = data[curIndex++] & 0xff;
                    if (temp < 0x80) {
                        throw new ResultException(
                                ResultCode.CMD_DATA_NOT_UNDERSTOOD,
                                "length < 0x80 length=" + Integer.toHexString(length) +
                                " curIndex=" + curIndex + " endIndex=" + endIndex);

                    }
                    length = temp;
                } else {
                    throw new ResultException(
                            ResultCode.CMD_DATA_NOT_UNDERSTOOD,
                            "Expected first byte to be length or a length tag and < 0x81" +
                            " byte= " + Integer.toHexString(temp) + " curIndex=" + curIndex +
                            " endIndex=" + endIndex);
                }
            } else {
                if (ComprehensionTlvTag.COMMAND_DETAILS.value() == (tag & ~0x80)) {
                    tag = BER_UNKNOWN_TAG;
                    curIndex = 0;
                }
            }
        } catch (IndexOutOfBoundsException e) {
            throw new ResultException(ResultCode.REQUIRED_VALUES_MISSING,
                    "IndexOutOfBoundsException " +
                    " curIndex=" + curIndex + " endIndex=" + endIndex);
        } catch (ResultException e) {
            throw new ResultException(ResultCode.CMD_DATA_NOT_UNDERSTOOD, e.explanation());
        }

        /* COMPREHENSION-TLVs */
        if (endIndex - curIndex < length) {
            throw new ResultException(ResultCode.CMD_DATA_NOT_UNDERSTOOD,
                    "Command had extra data endIndex=" + endIndex + " curIndex=" + curIndex +
                    " length=" + length);
        }

        List<ComprehensionTlv> ctlvs = ComprehensionTlv.decodeMany(data,
                curIndex);

        if (tag == BER_PROACTIVE_COMMAND_TAG) {
            int totalLength = 0;
            for (ComprehensionTlv item : ctlvs) {
                int itemLength = item.getLength();
                if (itemLength >= 0x80 && itemLength <= 0xFF) {
                    totalLength += itemLength + 3; //3: 'tag'(1 byte) and 'length'(2 bytes).
                } else if (itemLength >= 0 && itemLength < 0x80) {
                    totalLength += itemLength + 2; //2: 'tag'(1 byte) and 'length'(1 byte).
                } else {
                    isLengthValid = false;
                    break;
                }
            }

            // According to 3gpp11.14, chapter 6.10.6 "Length errors",

            // If the total lengths of the SIMPLE-TLV data objects are not
            // consistent with the length given in the BER-TLV data object,
            // then the whole BER-TLV data object shall be rejected. The
            // result field in the TERMINAL RESPONSE shall have the error
            // condition "Command data not understood by ME".
            if (length != totalLength) {
                isLengthValid = false;
            }
        }

        return new BerTlv(tag, ctlvs, isLengthValid);
    }
}
