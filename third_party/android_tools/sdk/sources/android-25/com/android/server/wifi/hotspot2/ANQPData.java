package com.android.server.wifi.hotspot2;

import com.android.server.wifi.Clock;
import com.android.server.wifi.anqp.ANQPElement;
import com.android.server.wifi.anqp.Constants;

import java.util.ArrayList;
import java.util.Collections;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

public class ANQPData {
    /**
     * The regular cache time for entries with a non-zero domain id.
     */
    private static final long ANQP_QUALIFIED_CACHE_TIMEOUT = 3600000L;
    /**
     * The cache time for entries with a zero domain id. The zero domain id indicates that ANQP
     * data from the AP may change at any time, thus a relatively short cache time is given to
     * such data, but still long enough to avoid excessive querying.
     */
    private static final long ANQP_UNQUALIFIED_CACHE_TIMEOUT = 300000L;
    /**
     * This is the hold off time for pending queries, i.e. the time during which subsequent queries
     * are squelched.
     */
    private static final long ANQP_HOLDOFF_TIME = 10000L;

    /**
     * Max value for the retry counter for unanswered queries. This limits the maximum time-out to
     * ANQP_HOLDOFF_TIME * 2^MAX_RETRY. With current values this results in 640s.
     */
    private static final int MAX_RETRY = 6;

    private final NetworkDetail mNetwork;
    private final Map<Constants.ANQPElementType, ANQPElement> mANQPElements;
    private final long mCtime;
    private final long mExpiry;
    private final int mRetry;
    private final Clock mClock;

    public ANQPData(Clock clock, NetworkDetail network,
                    Map<Constants.ANQPElementType, ANQPElement> anqpElements) {

        mClock = clock;
        mNetwork = network;
        mANQPElements = anqpElements != null ? new HashMap<>(anqpElements) : null;
        mCtime = mClock.currentTimeMillis();
        mRetry = 0;
        if (anqpElements == null) {
            mExpiry = mCtime + ANQP_HOLDOFF_TIME;
        }
        else if (network.getAnqpDomainID() == 0) {
            mExpiry = mCtime + ANQP_UNQUALIFIED_CACHE_TIMEOUT;
        }
        else {
            mExpiry = mCtime + ANQP_QUALIFIED_CACHE_TIMEOUT;
        }
    }

    public ANQPData(Clock clock, NetworkDetail network, ANQPData existing) {
        mClock = clock;
        mNetwork = network;
        mANQPElements = null;
        mCtime = mClock.currentTimeMillis();
        if (existing == null) {
            mRetry = 0;
            mExpiry = mCtime + ANQP_HOLDOFF_TIME;
        }
        else {
            mRetry = Math.max(existing.getRetry() + 1, MAX_RETRY);
            mExpiry = ANQP_HOLDOFF_TIME * (1<<mRetry);
        }
    }

    public List<Constants.ANQPElementType> disjoint(List<Constants.ANQPElementType> querySet) {
        if (mANQPElements == null) {
            // Ignore the query set for pending responses, it has minimal probability to happen
            // and a new query will be reissued on the next round anyway.
            return null;
        }
        else {
            List<Constants.ANQPElementType> additions = new ArrayList<>();
            for (Constants.ANQPElementType element : querySet) {
                if (!mANQPElements.containsKey(element)) {
                    additions.add(element);
                }
            }
            return additions.isEmpty() ? null : additions;
        }
    }

    public Map<Constants.ANQPElementType, ANQPElement> getANQPElements() {
        return Collections.unmodifiableMap(mANQPElements);
    }

    public NetworkDetail getNetwork() {
        return mNetwork;
    }

    public boolean expired() {
        return expired(mClock.currentTimeMillis());
    }

    public boolean expired(long at) {
        return mExpiry <= at;
    }

    protected boolean hasData() {
        return mANQPElements != null;
    }

    protected void merge(Map<Constants.ANQPElementType, ANQPElement> data) {
        if (data != null) {
            mANQPElements.putAll(data);
        }
    }

    protected boolean isValid(NetworkDetail nwk) {
        return mANQPElements != null &&
                nwk.getAnqpDomainID() == mNetwork.getAnqpDomainID() &&
                mExpiry > mClock.currentTimeMillis();
    }

    private int getRetry() {
        return mRetry;
    }

    public String toString(boolean brief) {
        StringBuilder sb = new StringBuilder();
        sb.append(mNetwork.toKeyString()).append(", domid ").append(mNetwork.getAnqpDomainID());
        if (mANQPElements == null) {
            sb.append(", unresolved, ");
        }
        else {
            sb.append(", ").append(mANQPElements.size()).append(" elements, ");
        }
        long now = mClock.currentTimeMillis();
        sb.append(Utils.toHMS(now-mCtime)).append(" old, expires in ").
                append(Utils.toHMS(mExpiry-now)).append(' ');
        if (brief) {
            sb.append(expired(now) ? 'x' : '-');
            sb.append(mANQPElements == null ? 'u' : '-');
        }
        else if (mANQPElements != null) {
            sb.append(" data=").append(mANQPElements);
        }
        return sb.toString();
    }

    @Override
    public String toString() {
        return toString(true);
    }
}
