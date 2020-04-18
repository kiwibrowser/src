package com.android.server.wifi.anqp;

import java.net.ProtocolException;
import java.nio.ByteBuffer;
import java.util.ArrayList;
import java.util.Iterator;
import java.util.List;

import static com.android.server.wifi.anqp.Constants.BYTE_MASK;

public class CellularNetwork implements Iterable<String> {
    private static final int PLMNListType = 0;

    private final List<String> mMccMnc;

    private CellularNetwork(int plmnCount, ByteBuffer payload) throws ProtocolException {
        mMccMnc = new ArrayList<>(plmnCount);

        while (plmnCount > 0) {
            if (payload.remaining() < 3) {
                throw new ProtocolException("Truncated PLMN info");
            }
            byte[] plmn = new byte[3];
            payload.get(plmn);

            int mcc = ((plmn[0] << 8) & 0xf00) |
                    (plmn[0] & 0x0f0) |
                    (plmn[1] & 0x00f);

            int mnc = ((plmn[2] << 4) & 0xf0) |
                    ((plmn[2] >> 4) & 0x0f);

            int n2 = (plmn[1] >> 4) & 0x0f;
            String mccMnc = n2 != 0xf ?
                    String.format("%03x%03x", mcc, (mnc << 4) | n2) :
                    String.format("%03x%02x", mcc, mnc);

            mMccMnc.add(mccMnc);
            plmnCount--;
        }
    }

    public static CellularNetwork buildCellularNetwork(ByteBuffer payload)
            throws ProtocolException {
        int iei = payload.get() & BYTE_MASK;
        int plmnLen = payload.get() & 0x7f;

        if (iei != PLMNListType) {
            payload.position(payload.position() + plmnLen);
            return null;
        }

        int plmnCount = payload.get() & BYTE_MASK;
        return new CellularNetwork(plmnCount, payload);
    }

    @Override
    public Iterator<String> iterator() {
        return mMccMnc.iterator();
    }

    @Override
    public String toString() {
        StringBuilder sb = new StringBuilder("PLMN:");
        for (String mccMnc : mMccMnc) {
            sb.append(' ').append(mccMnc);
        }
        return sb.toString();
    }
}
