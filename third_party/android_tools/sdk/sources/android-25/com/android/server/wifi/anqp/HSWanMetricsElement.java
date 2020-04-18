package com.android.server.wifi.anqp;

import java.net.ProtocolException;
import java.nio.ByteBuffer;

import static com.android.server.wifi.anqp.Constants.BYTE_MASK;
import static com.android.server.wifi.anqp.Constants.INT_MASK;
import static com.android.server.wifi.anqp.Constants.SHORT_MASK;

/**
 * The WAN Metrics vendor specific ANQP Element,
 * Wi-Fi Alliance Hotspot 2.0 (Release 2) Technical Specification - Version 5.00,
 * section 4.4
 */
public class HSWanMetricsElement extends ANQPElement {

    public enum LinkStatus {Reserved, Up, Down, Test}

    private final LinkStatus mStatus;
    private final boolean mSymmetric;
    private final boolean mCapped;
    private final long mDlSpeed;
    private final long mUlSpeed;
    private final int mDlLoad;
    private final int mUlLoad;
    private final int mLMD;

    public HSWanMetricsElement(Constants.ANQPElementType infoID, ByteBuffer payload)
            throws ProtocolException {
        super(infoID);

        if (payload.remaining() != 13) {
            throw new ProtocolException("Bad WAN metrics length: " + payload.remaining());
        }

        int status = payload.get() & BYTE_MASK;
        mStatus = LinkStatus.values()[status & 0x03];
        mSymmetric = (status & 0x04) != 0;
        mCapped = (status & 0x08) != 0;
        mDlSpeed = payload.getInt() & INT_MASK;
        mUlSpeed = payload.getInt() & INT_MASK;
        mDlLoad = payload.get() & BYTE_MASK;
        mUlLoad = payload.get() & BYTE_MASK;
        mLMD = payload.getShort() & SHORT_MASK;
    }

    public LinkStatus getStatus() {
        return mStatus;
    }

    public boolean isSymmetric() {
        return mSymmetric;
    }

    public boolean isCapped() {
        return mCapped;
    }

    public long getDlSpeed() {
        return mDlSpeed;
    }

    public long getUlSpeed() {
        return mUlSpeed;
    }

    public int getDlLoad() {
        return mDlLoad;
    }

    public int getUlLoad() {
        return mUlLoad;
    }

    public int getLMD() {
        return mLMD;
    }

    @Override
    public String toString() {
        return String.format("HSWanMetrics{mStatus=%s, mSymmetric=%s, mCapped=%s, " +
                "mDlSpeed=%d, mUlSpeed=%d, mDlLoad=%f, mUlLoad=%f, mLMD=%d}",
                mStatus, mSymmetric, mCapped,
                mDlSpeed, mUlSpeed,
                (double)mDlLoad * 100.0 / 256.0,
                (double)mUlLoad * 100.0 / 256.0,
                mLMD);
    }
}
