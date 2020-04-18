package com.android.server.wifi.anqp;

import com.android.server.wifi.SIMAccessor;
import com.android.server.wifi.hotspot2.AuthMatch;
import com.android.server.wifi.hotspot2.Utils;
import com.android.server.wifi.hotspot2.pps.Credential;
import com.android.server.wifi.hotspot2.pps.HomeSP;

import java.net.ProtocolException;
import java.nio.ByteBuffer;
import java.util.ArrayList;
import java.util.Collections;
import java.util.List;

import static com.android.server.wifi.anqp.Constants.BYTES_IN_SHORT;
import static com.android.server.wifi.anqp.Constants.SHORT_MASK;

/**
 * The NAI Realm ANQP Element, IEEE802.11-2012 section 8.4.4.10
 */
public class NAIRealmElement extends ANQPElement {
    private final List<NAIRealmData> mRealmData;

    public NAIRealmElement(Constants.ANQPElementType infoID, ByteBuffer payload)
            throws ProtocolException {
        super(infoID);

        if (!payload.hasRemaining()) {
            mRealmData = Collections.emptyList();
            return;
        }

        if (payload.remaining() < BYTES_IN_SHORT) {
            throw new ProtocolException("Runt NAI Realm: " + payload.remaining());
        }

        int count = payload.getShort() & SHORT_MASK;
        mRealmData = new ArrayList<>(count);
        while (count > 0) {
            mRealmData.add(new NAIRealmData(payload));
            count--;
        }
    }

    public List<NAIRealmData> getRealmData() {
        return Collections.unmodifiableList(mRealmData);
    }

    public int match(Credential credential) {
        if (mRealmData.isEmpty())
            return AuthMatch.Indeterminate;

        List<String> credLabels = Utils.splitDomain(credential.getRealm());
        int best = AuthMatch.None;
        for (NAIRealmData realmData : mRealmData) {
            int match = realmData.match(credLabels, credential);
            if (match > best) {
                best = match;
                if (best == AuthMatch.Exact) {
                    return best;
                }
            }
        }
        return best;
    }

    @Override
    public String toString() {
        StringBuilder sb = new StringBuilder();
        sb.append("NAI Realm:\n");
        for (NAIRealmData data : mRealmData) {
            sb.append(data);
        }
        return sb.toString();
    }
}
