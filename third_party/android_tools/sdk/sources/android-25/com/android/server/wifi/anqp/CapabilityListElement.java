package com.android.server.wifi.anqp;

import java.net.ProtocolException;
import java.nio.ByteBuffer;
import java.util.Arrays;

/**
 * The ANQP Capability List element, 802.11-2012 section 8.4.4.3
 */
public class CapabilityListElement extends ANQPElement {
    private final Constants.ANQPElementType[] mCapabilities;

    public CapabilityListElement(Constants.ANQPElementType infoID, ByteBuffer payload)
            throws ProtocolException {
        super(infoID);
        if ((payload.remaining() & 1) == 1)
            throw new ProtocolException("Odd length");
        mCapabilities = new Constants.ANQPElementType[payload.remaining() / Constants.BYTES_IN_SHORT];

        int index = 0;
        while (payload.hasRemaining()) {
            int capID = payload.getShort() & Constants.SHORT_MASK;
            Constants.ANQPElementType capability = Constants.mapANQPElement(capID);
            if (capability == null)
                throw new ProtocolException("Unknown capability: " + capID);
            mCapabilities[index++] = capability;
        }
    }

    public Constants.ANQPElementType[] getCapabilities() {
        return mCapabilities;
    }

    @Override
    public String toString() {
        return "CapabilityList{" +
                "mCapabilities=" + Arrays.toString(mCapabilities) +
                '}';
    }
}
