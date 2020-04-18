package com.android.server.wifi.anqp;

import java.net.ProtocolException;
import java.nio.ByteBuffer;
import java.util.ArrayList;
import java.util.Collections;
import java.util.List;

import static com.android.server.wifi.anqp.Constants.BYTE_MASK;


/**
 * The 3GPP Cellular Network ANQP Element, IEEE802.11-2012 section 8.4.4.11
 */
public class ThreeGPPNetworkElement extends ANQPElement {
    private final int mUserData;
    private final List<CellularNetwork> mPlmns;

    public ThreeGPPNetworkElement(Constants.ANQPElementType infoID, ByteBuffer payload)
            throws ProtocolException {
        super(infoID);

        mPlmns = new ArrayList<CellularNetwork>();
        mUserData = payload.get() & BYTE_MASK;
        int length = payload.get() & BYTE_MASK;
        if (length > payload.remaining()) {
            throw new ProtocolException("Runt payload");
        }

        while (payload.hasRemaining()) {
            CellularNetwork network = CellularNetwork.buildCellularNetwork(payload);
            if (network != null) {
                mPlmns.add(network);
            }
        }
    }

    public int getUserData() {
        return mUserData;
    }

    public List<CellularNetwork> getPlmns() {
        return Collections.unmodifiableList(mPlmns);
    }

    @Override
    public String toString() {
        return "ThreeGPPNetwork{" +
                "mUserData=" + mUserData +
                ", mPlmns=" + mPlmns +
                '}';
    }
}
