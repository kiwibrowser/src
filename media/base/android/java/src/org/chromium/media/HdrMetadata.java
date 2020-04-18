// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.media;

import android.annotation.TargetApi;
import android.media.MediaFormat;
import android.os.Build;
import android.support.annotation.VisibleForTesting;

import org.chromium.base.Log;
import org.chromium.base.annotations.CalledByNative;
import org.chromium.base.annotations.JNINamespace;
import org.chromium.base.annotations.MainDex;

import java.nio.ByteBuffer;
import java.nio.ByteOrder;

@JNINamespace("media")
@MainDex
class HdrMetadata {
    private static final String TAG = "HdrMetadata";
    private static final int MAX_CHROMATICITY = 50000; // Defined in CTA-861.3.

    private long mNativeJniHdrMetadata;
    private final Object mLock = new Object();

    @CalledByNative
    private static HdrMetadata create(long nativeRef) {
        return new HdrMetadata(nativeRef);
    }

    @VisibleForTesting
    HdrMetadata() {
        // Used for testing only.
        mNativeJniHdrMetadata = 0;
    }

    private HdrMetadata(long nativeRef) {
        assert nativeRef != 0;
        mNativeJniHdrMetadata = nativeRef;
    }

    @CalledByNative
    private void close() {
        synchronized (mLock) {
            mNativeJniHdrMetadata = 0;
        }
    }

    @TargetApi(Build.VERSION_CODES.N)
    public void addMetadataToFormat(MediaFormat format) {
        synchronized (mLock) {
            assert mNativeJniHdrMetadata != 0;
            if (Build.VERSION.SDK_INT < Build.VERSION_CODES.N) {
                Log.e(TAG, "HDR not supported before Android N");
                return;
            }

            // TODO(sandv): Use color space matrix when android has support for it.
            int colorStandard = getColorStandard();
            if (colorStandard != -1)
                format.setInteger(MediaFormat.KEY_COLOR_STANDARD, colorStandard);

            int colorTransfer = getColorTransfer();
            if (colorTransfer != -1)
                format.setInteger(MediaFormat.KEY_COLOR_TRANSFER, colorTransfer);

            int colorRange = getColorRange();
            if (colorRange != -1) format.setInteger(MediaFormat.KEY_COLOR_RANGE, colorRange);

            ByteBuffer hdrStaticInfo = ByteBuffer.wrap(new byte[25]);
            hdrStaticInfo.order(ByteOrder.LITTLE_ENDIAN);
            hdrStaticInfo.put((byte) 0); // Type.
            hdrStaticInfo.putShort((short) ((primaryRChromaticityX() * MAX_CHROMATICITY) + 0.5f));
            hdrStaticInfo.putShort((short) ((primaryRChromaticityY() * MAX_CHROMATICITY) + 0.5f));
            hdrStaticInfo.putShort((short) ((primaryGChromaticityX() * MAX_CHROMATICITY) + 0.5f));
            hdrStaticInfo.putShort((short) ((primaryGChromaticityY() * MAX_CHROMATICITY) + 0.5f));
            hdrStaticInfo.putShort((short) ((primaryBChromaticityX() * MAX_CHROMATICITY) + 0.5f));
            hdrStaticInfo.putShort((short) ((primaryBChromaticityY() * MAX_CHROMATICITY) + 0.5f));
            hdrStaticInfo.putShort((short) ((whitePointChromaticityX() * MAX_CHROMATICITY) + 0.5f));
            hdrStaticInfo.putShort((short) ((whitePointChromaticityY() * MAX_CHROMATICITY) + 0.5f));
            hdrStaticInfo.putShort((short) (maxMasteringLuminance() + 0.5f));
            hdrStaticInfo.putShort((short) (minMasteringLuminance() + 0.5f));
            hdrStaticInfo.putShort((short) maxContentLuminance());
            hdrStaticInfo.putShort((short) maxFrameAverageLuminance());

            hdrStaticInfo.rewind();
            format.setByteBuffer(MediaFormat.KEY_HDR_STATIC_INFO, hdrStaticInfo);
        }
    }

    private native int nativePrimaries(long nativeJniHdrMetadata);
    private int getColorStandard() {
        // media/base/video_color_space.h
        switch (nativePrimaries(mNativeJniHdrMetadata)) {
            case 1:
                return MediaFormat.COLOR_STANDARD_BT709;
            case 4: // BT.470M.
            case 5: // BT.470BG.
            case 6: // SMPTE 170M.
            case 7: // SMPTE 240M.
                return MediaFormat.COLOR_STANDARD_BT601_NTSC;
            case 9:
                return MediaFormat.COLOR_STANDARD_BT2020;
            default:
                return -1;
        }
    }

    private native int nativeColorTransfer(long nativeJniHdrMetadata);
    private int getColorTransfer() {
        // media/base/video_color_space.h
        switch (nativeColorTransfer(mNativeJniHdrMetadata)) {
            case 1: // BT.709.
            case 6: // SMPTE 170M.
            case 7: // SMPTE 240M.
                return MediaFormat.COLOR_TRANSFER_SDR_VIDEO;
            case 8:
                return MediaFormat.COLOR_TRANSFER_LINEAR;
            case 16:
                return MediaFormat.COLOR_TRANSFER_ST2084;
            case 18:
                return MediaFormat.COLOR_TRANSFER_HLG;
            default:
                return -1;
        }
    }

    private native int nativeRange(long nativeJniHdrMetadata);
    private int getColorRange() {
        // media/base/video_color_space.h
        switch (nativeRange(mNativeJniHdrMetadata)) {
            case 1:
                return MediaFormat.COLOR_RANGE_LIMITED;
            case 2:
                return MediaFormat.COLOR_RANGE_FULL;
            default:
                return -1;
        }
    }

    private native float nativePrimaryRChromaticityX(long nativeJniHdrMetadata);
    private float primaryRChromaticityX() {
        return nativePrimaryRChromaticityX(mNativeJniHdrMetadata);
    }

    private native float nativePrimaryRChromaticityY(long nativeJniHdrMetadata);
    private float primaryRChromaticityY() {
        return nativePrimaryRChromaticityY(mNativeJniHdrMetadata);
    }

    private native float nativePrimaryGChromaticityX(long nativeJniHdrMetadata);
    private float primaryGChromaticityX() {
        return nativePrimaryGChromaticityX(mNativeJniHdrMetadata);
    }

    private native float nativePrimaryGChromaticityY(long nativeJniHdrMetadata);
    private float primaryGChromaticityY() {
        return nativePrimaryGChromaticityY(mNativeJniHdrMetadata);
    }

    private native float nativePrimaryBChromaticityX(long nativeJniHdrMetadata);
    private float primaryBChromaticityX() {
        return nativePrimaryBChromaticityX(mNativeJniHdrMetadata);
    }

    private native float nativePrimaryBChromaticityY(long nativeJniHdrMetadata);
    private float primaryBChromaticityY() {
        return nativePrimaryBChromaticityY(mNativeJniHdrMetadata);
    }

    private native float nativeWhitePointChromaticityX(long nativeJniHdrMetadata);
    private float whitePointChromaticityX() {
        return nativeWhitePointChromaticityX(mNativeJniHdrMetadata);
    }

    private native float nativeWhitePointChromaticityY(long nativeJniHdrMetadata);
    private float whitePointChromaticityY() {
        return nativeWhitePointChromaticityY(mNativeJniHdrMetadata);
    }

    private native float nativeMaxMasteringLuminance(long nativeJniHdrMetadata);
    private float maxMasteringLuminance() {
        return nativeMaxMasteringLuminance(mNativeJniHdrMetadata);
    }

    private native float nativeMinMasteringLuminance(long nativeJniHdrMetadata);
    private float minMasteringLuminance() {
        return nativeMinMasteringLuminance(mNativeJniHdrMetadata);
    }

    private native int nativeMaxContentLuminance(long nativeJniHdrMetadata);
    private int maxContentLuminance() {
        return nativeMaxContentLuminance(mNativeJniHdrMetadata);
    }

    private native int nativeMaxFrameAverageLuminance(long nativeJniHdrMetadata);
    private int maxFrameAverageLuminance() {
        return nativeMaxFrameAverageLuminance(mNativeJniHdrMetadata);
    }
}
