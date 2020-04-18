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

/**
 * {@hide}
 */
public class ImageDescriptor {
    // members
    int mWidth;
    int mHeight;
    int mCodingScheme;
    int mImageId;
    int mHighOffset;
    int mLowOffset;
    int mLength;

    // constants
    static final int CODING_SCHEME_BASIC = 0x11;
    static final int CODING_SCHEME_COLOUR = 0x21;

    // public static final int ID_LENGTH = 9;
    // ID_LENGTH substituted by IccFileHandlerBase.GET_RESPONSE_EF_IMG_SIZE_BYTES

    ImageDescriptor() {
        mWidth = 0;
        mHeight = 0;
        mCodingScheme = 0;
        mImageId = 0;
        mHighOffset = 0;
        mLowOffset = 0;
        mLength = 0;
    }

    /**
     * Extract descriptor information about image instance.
     *
     * @param rawData
     * @param valueIndex
     * @return ImageDescriptor
     */
    static ImageDescriptor parse(byte[] rawData, int valueIndex) {
        ImageDescriptor d = new ImageDescriptor();
        try {
            d.mWidth = rawData[valueIndex++] & 0xff;
            d.mHeight = rawData[valueIndex++] & 0xff;
            d.mCodingScheme = rawData[valueIndex++] & 0xff;

            // parse image id
            d.mImageId = (rawData[valueIndex++] & 0xff) << 8;
            d.mImageId |= rawData[valueIndex++] & 0xff;
            // parse offset
            d.mHighOffset = (rawData[valueIndex++] & 0xff); // high byte offset
            d.mLowOffset = rawData[valueIndex++] & 0xff; // low byte offset

            d.mLength = ((rawData[valueIndex++] & 0xff) << 8 | (rawData[valueIndex++] & 0xff));
            CatLog.d("ImageDescriptor", "parse; Descriptor : " + d.mWidth + ", " + d.mHeight +
                    ", " + d.mCodingScheme + ", 0x" + Integer.toHexString(d.mImageId) + ", " +
                    d.mHighOffset + ", " + d.mLowOffset + ", " + d.mLength);
        } catch (IndexOutOfBoundsException e) {
            CatLog.d("ImageDescriptor", "parse; failed parsing image descriptor");
            d = null;
        }
        return d;
    }
}
