package com.android.server.wifi.hotspot2.pps;

import android.util.Log;

import com.android.server.wifi.SIMAccessor;
import com.android.server.wifi.anqp.ANQPElement;
import com.android.server.wifi.anqp.CellularNetwork;
import com.android.server.wifi.anqp.Constants;
import com.android.server.wifi.anqp.DomainNameElement;
import com.android.server.wifi.anqp.NAIRealmElement;
import com.android.server.wifi.anqp.RoamingConsortiumElement;
import com.android.server.wifi.anqp.ThreeGPPNetworkElement;
import com.android.server.wifi.hotspot2.AuthMatch;
import com.android.server.wifi.hotspot2.NetworkDetail;
import com.android.server.wifi.hotspot2.PasspointMatch;
import com.android.server.wifi.hotspot2.Utils;

import java.util.ArrayList;
import java.util.Collection;
import java.util.Collections;
import java.util.HashSet;
import java.util.List;
import java.util.Map;
import java.util.Set;

import static com.android.server.wifi.anqp.Constants.ANQPElementType;

/**
 * This object describes the Home SP sub tree in the "PerProviderSubscription MO" described in
 * the Hotspot 2.0 specification, section 9.1.
 * As a convenience, the object also refers to the other sub-parts of the full
 * PerProviderSubscription tree.
 */
public class HomeSP {
    private final Map<String, Long> mSSIDs;        // SSID, HESSID, [0,N]
    private final String mFQDN;
    private final DomainMatcher mDomainMatcher;
    private final Set<String> mOtherHomePartners;
    private final HashSet<Long> mRoamingConsortiums;    // [0,N]
    private final Set<Long> mMatchAnyOIs;           // [0,N]
    private final List<Long> mMatchAllOIs;          // [0,N]

    private final Credential mCredential;

    // Informational:
    private final String mFriendlyName;             // [1]
    private final String mIconURL;                  // [0,1]

    private final Policy mPolicy;
    private final int mCredentialPriority;
    private final Map<String, String> mAAATrustRoots;
    private final UpdateInfo mSubscriptionUpdate;
    private final SubscriptionParameters mSubscriptionParameters;
    private final int mUpdateIdentifier;

    @Deprecated
    public HomeSP(Map<String, Long> ssidMap,
                   /*@NotNull*/ String fqdn,
                   /*@NotNull*/ HashSet<Long> roamingConsortiums,
                   /*@NotNull*/ Set<String> otherHomePartners,
                   /*@NotNull*/ Set<Long> matchAnyOIs,
                   /*@NotNull*/ List<Long> matchAllOIs,
                   String friendlyName,
                   String iconURL,
                   Credential credential) {

        mSSIDs = ssidMap;
        List<List<String>> otherPartners = new ArrayList<>(otherHomePartners.size());
        for (String otherPartner : otherHomePartners) {
            otherPartners.add(Utils.splitDomain(otherPartner));
        }
        mOtherHomePartners = otherHomePartners;
        mFQDN = fqdn;
        mDomainMatcher = new DomainMatcher(Utils.splitDomain(fqdn), otherPartners);
        mRoamingConsortiums = roamingConsortiums;
        mMatchAnyOIs = matchAnyOIs;
        mMatchAllOIs = matchAllOIs;
        mFriendlyName = friendlyName;
        mIconURL = iconURL;
        mCredential = credential;

        mPolicy = null;
        mCredentialPriority = -1;
        mAAATrustRoots = null;
        mSubscriptionUpdate = null;
        mSubscriptionParameters = null;
        mUpdateIdentifier = -1;
    }

    public HomeSP(Map<String, Long> ssidMap,
                   /*@NotNull*/ String fqdn,
                   /*@NotNull*/ HashSet<Long> roamingConsortiums,
                   /*@NotNull*/ Set<String> otherHomePartners,
                   /*@NotNull*/ Set<Long> matchAnyOIs,
                   /*@NotNull*/ List<Long> matchAllOIs,
                   String friendlyName,
                   String iconURL,
                   Credential credential,

                  Policy policy,
                  int credentialPriority,
                  Map<String, String> AAATrustRoots,
                  UpdateInfo subscriptionUpdate,
                  SubscriptionParameters subscriptionParameters,
                  int updateIdentifier) {

        mSSIDs = ssidMap;
        List<List<String>> otherPartners = new ArrayList<>(otherHomePartners.size());
        for (String otherPartner : otherHomePartners) {
            otherPartners.add(Utils.splitDomain(otherPartner));
        }
        mOtherHomePartners = otherHomePartners;
        mFQDN = fqdn;
        mDomainMatcher = new DomainMatcher(Utils.splitDomain(fqdn), otherPartners);
        mRoamingConsortiums = roamingConsortiums;
        mMatchAnyOIs = matchAnyOIs;
        mMatchAllOIs = matchAllOIs;
        mFriendlyName = friendlyName;
        mIconURL = iconURL;
        mCredential = credential;

        mPolicy = policy;
        mCredentialPriority = credentialPriority;
        mAAATrustRoots = AAATrustRoots;
        mSubscriptionUpdate = subscriptionUpdate;
        mSubscriptionParameters = subscriptionParameters;
        mUpdateIdentifier = updateIdentifier;
    }

    public int getUpdateIdentifier() {
        return mUpdateIdentifier;
    }

    public UpdateInfo getSubscriptionUpdate() {
        return mSubscriptionUpdate;
    }

    public Policy getPolicy() {
        return mPolicy;
    }

    public PasspointMatch match(NetworkDetail networkDetail,
                                Map<ANQPElementType, ANQPElement> anqpElementMap,
                                SIMAccessor simAccessor) {

        List<String> imsis = simAccessor.getMatchingImsis(mCredential.getImsi());

        PasspointMatch spMatch = matchSP(networkDetail, anqpElementMap, imsis);

        if (spMatch == PasspointMatch.Incomplete || spMatch == PasspointMatch.Declined) {
            return spMatch;
        }

        if (imsiMatch(imsis, (ThreeGPPNetworkElement)
                anqpElementMap.get(ANQPElementType.ANQP3GPPNetwork)) != null) {
            // PLMN match, promote sp match to roaming if necessary.
            return spMatch == PasspointMatch.None ? PasspointMatch.RoamingProvider : spMatch;
        }

        NAIRealmElement naiRealmElement =
                (NAIRealmElement) anqpElementMap.get(ANQPElementType.ANQPNAIRealm);

        int authMatch = naiRealmElement != null ?
                naiRealmElement.match(mCredential) :
                AuthMatch.Indeterminate;
        
        Log.d(Utils.hs2LogTag(getClass()), networkDetail.toKeyString() + " match on " + mFQDN +
                ": " + spMatch + ", auth " + AuthMatch.toString(authMatch));

        if (authMatch == AuthMatch.None) {
            // Distinct auth mismatch, demote authentication.
            return PasspointMatch.None;
        }
        else if ((authMatch & AuthMatch.Realm) == 0) {
            // No realm match, return sp match as is.
            return spMatch;
        }
        else {
            // Realm match, promote sp match to roaming if necessary.
            return spMatch == PasspointMatch.None ? PasspointMatch.RoamingProvider : spMatch;
        }
    }

    public PasspointMatch matchSP(NetworkDetail networkDetail,
                                Map<ANQPElementType, ANQPElement> anqpElementMap,
                                List<String> imsis) {

        if (mSSIDs.containsKey(networkDetail.getSSID())) {
            Long hessid = mSSIDs.get(networkDetail.getSSID());
            if (hessid == null || networkDetail.getHESSID() == hessid) {
                Log.d(Utils.hs2LogTag(getClass()), "match SSID");
                return PasspointMatch.HomeProvider;
            }
        }

        Set<Long> anOIs = new HashSet<>();

        if (networkDetail.getRoamingConsortiums() != null) {
            for (long oi : networkDetail.getRoamingConsortiums()) {
                anOIs.add(oi);
            }
        }

        boolean validANQP = anqpElementMap != null &&
                Constants.hasBaseANQPElements(anqpElementMap.keySet());

        RoamingConsortiumElement rcElement = validANQP ?
                (RoamingConsortiumElement) anqpElementMap.get(ANQPElementType.ANQPRoamingConsortium)
                : null;
        if (rcElement != null) {
            anOIs.addAll(rcElement.getOIs());
        }

        // It may seem reasonable to check for home provider match prior to checking for roaming
        // relationship, but it is possible to avoid an ANQP query if it turns out that the
        // "match all" rule fails based only on beacon info only.
        boolean roamingMatch = false;

        if (!mMatchAllOIs.isEmpty()) {
            boolean matchesAll = true;

            for (long spOI : mMatchAllOIs) {
                if (!anOIs.contains(spOI)) {
                    matchesAll = false;
                    break;
                }
            }
            if (matchesAll) {
                roamingMatch = true;
            }
            else {
                if (validANQP || networkDetail.getAnqpOICount() == 0) {
                    return PasspointMatch.Declined;
                }
                else {
                    return PasspointMatch.Incomplete;
                }
            }
        }

        if (!roamingMatch &&
                (!Collections.disjoint(mMatchAnyOIs, anOIs) ||
                        !Collections.disjoint(mRoamingConsortiums, anOIs))) {
            roamingMatch = true;
        }

        if (!validANQP) {
            return PasspointMatch.Incomplete;
        }

        DomainNameElement domainNameElement =
                (DomainNameElement) anqpElementMap.get(ANQPElementType.ANQPDomName);

        if (domainNameElement != null) {
            for (String domain : domainNameElement.getDomains()) {
                List<String> anLabels = Utils.splitDomain(domain);
                DomainMatcher.Match match = mDomainMatcher.isSubDomain(anLabels);
                if (match != DomainMatcher.Match.None) {
                    return PasspointMatch.HomeProvider;
                }

                if (imsiMatch(imsis, anLabels) != null) {
                    return PasspointMatch.HomeProvider;
                }
            }
        }

        return roamingMatch ? PasspointMatch.RoamingProvider : PasspointMatch.None;
    }

    private String imsiMatch(List<String> imsis, ThreeGPPNetworkElement plmnElement) {
        if (imsis == null || plmnElement == null || plmnElement.getPlmns().isEmpty()) {
            return null;
        }
        for (CellularNetwork network : plmnElement.getPlmns()) {
            for (String mccMnc : network) {
                String imsi = imsiMatch(imsis, mccMnc);
                if (imsi != null) {
                    return imsi;
                }
            }
        }
        return null;
    }

    private String imsiMatch(List<String> imsis, List<String> fqdn) {
        if (imsis == null) {
            return null;
        }
        String mccMnc = Utils.getMccMnc(fqdn);
        return mccMnc != null ? imsiMatch(imsis, mccMnc) : null;
    }

    private String imsiMatch(List<String> imsis, String mccMnc) {
        if (mCredential.getImsi().matchesMccMnc(mccMnc)) {
            for (String imsi : imsis) {
                if (imsi.startsWith(mccMnc)) {
                    return imsi;
                }
            }
        }
        return null;
    }

    public String getFQDN() { return mFQDN; }
    public String getFriendlyName() { return mFriendlyName; }
    public HashSet<Long> getRoamingConsortiums() { return mRoamingConsortiums; }
    public Credential getCredential() { return mCredential; }

    public Map<String, Long> getSSIDs() {
        return mSSIDs;
    }

    public Collection<String> getOtherHomePartners() {
        return mOtherHomePartners;
    }

    public Set<Long> getMatchAnyOIs() {
        return mMatchAnyOIs;
    }

    public List<Long> getMatchAllOIs() {
        return mMatchAllOIs;
    }

    public String getIconURL() {
        return mIconURL;
    }

    public boolean deepEquals(HomeSP other) {
        return mFQDN.equals(other.mFQDN) &&
                mSSIDs.equals(other.mSSIDs) &&
                mOtherHomePartners.equals(other.mOtherHomePartners) &&
                mRoamingConsortiums.equals(other.mRoamingConsortiums) &&
                mMatchAnyOIs.equals(other.mMatchAnyOIs) &&
                mMatchAllOIs.equals(other.mMatchAllOIs) &&
                mFriendlyName.equals(other.mFriendlyName) &&
                Utils.compare(mIconURL, other.mIconURL) == 0 &&
                mCredential.equals(other.mCredential);
    }

    @Override
    public boolean equals(Object thatObject) {
        if (this == thatObject) {
            return true;
        } else if (thatObject == null || getClass() != thatObject.getClass()) {
            return false;
        }

        HomeSP that = (HomeSP) thatObject;
        return mFQDN.equals(that.mFQDN);
    }

    @Override
    public int hashCode() {
        return mFQDN.hashCode();
    }

    @Override
    public String toString() {
        return "HomeSP{" +
                "SSIDs=" + mSSIDs +
                ", FQDN='" + mFQDN + '\'' +
                ", DomainMatcher=" + mDomainMatcher +
                ", RoamingConsortiums={" + Utils.roamingConsortiumsToString(mRoamingConsortiums) +
                '}' +
                ", MatchAnyOIs={" + Utils.roamingConsortiumsToString(mMatchAnyOIs) + '}' +
                ", MatchAllOIs={" + Utils.roamingConsortiumsToString(mMatchAllOIs) + '}' +
                ", Credential=" + mCredential +
                ", FriendlyName='" + mFriendlyName + '\'' +
                ", IconURL='" + mIconURL + '\'' +
                '}';
    }
}
