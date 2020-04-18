package com.android.server.wifi.anqp;

import java.nio.ByteBuffer;

/**
 * An object holding the raw octets of an ANQP element as provided by the wpa_supplicant.
 */
public class RawByteElement extends ANQPElement {
    private final byte[] mPayload;

    public RawByteElement(Constants.ANQPElementType infoID, ByteBuffer payload) {
        super(infoID);
        mPayload = new byte[payload.remaining()];
        payload.get(mPayload);
    }

    public byte[] getPayload() {
        return mPayload;
    }
}
