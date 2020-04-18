package com.android.server.wifi.hotspot2;

import static com.android.server.wifi.anqp.Constants.BYTES_IN_EUI48;
import static com.android.server.wifi.anqp.Constants.BYTE_MASK;

import android.net.wifi.ScanResult;
import android.util.Log;

import com.android.server.wifi.anqp.ANQPElement;
import com.android.server.wifi.anqp.Constants;
import com.android.server.wifi.anqp.RawByteElement;
import com.android.server.wifi.anqp.VenueNameElement;
import com.android.server.wifi.util.InformationElementUtil;

import java.nio.BufferUnderflowException;
import java.nio.ByteBuffer;
import java.nio.CharBuffer;
import java.nio.charset.CharacterCodingException;
import java.nio.charset.CharsetDecoder;
import java.nio.charset.StandardCharsets;
import java.util.ArrayList;
import java.util.List;
import java.util.Map;

public class NetworkDetail {

    //turn off when SHIP
    private static final boolean DBG = true;
    private static final boolean VDBG = false;

    private static final String TAG = "NetworkDetail:";

    public enum Ant {
        Private,
        PrivateWithGuest,
        ChargeablePublic,
        FreePublic,
        Personal,
        EmergencyOnly,
        Resvd6,
        Resvd7,
        Resvd8,
        Resvd9,
        Resvd10,
        Resvd11,
        Resvd12,
        Resvd13,
        TestOrExperimental,
        Wildcard
    }

    public enum HSRelease {
        R1,
        R2,
        Unknown
    }

    // General identifiers:
    private final String mSSID;
    private final long mHESSID;
    private final long mBSSID;
    // True if the SSID is potentially from a hidden network
    private final boolean mIsHiddenSsid;

    // BSS Load element:
    private final int mStationCount;
    private final int mChannelUtilization;
    private final int mCapacity;

    //channel detailed information
   /*
    * 0 -- 20 MHz
    * 1 -- 40 MHz
    * 2 -- 80 MHz
    * 3 -- 160 MHz
    * 4 -- 80 + 80 MHz
    */
    private final int mChannelWidth;
    private final int mPrimaryFreq;
    private final int mCenterfreq0;
    private final int mCenterfreq1;

    /*
     * 802.11 Standard (calculated from Capabilities and Supported Rates)
     * 0 -- Unknown
     * 1 -- 802.11a
     * 2 -- 802.11b
     * 3 -- 802.11g
     * 4 -- 802.11n
     * 7 -- 802.11ac
     */
    private final int mWifiMode;
    private final int mMaxRate;

    /*
     * From Interworking element:
     * mAnt non null indicates the presence of Interworking, i.e. 802.11u
     * mVenueGroup and mVenueType may be null if not present in the Interworking element.
     */
    private final Ant mAnt;
    private final boolean mInternet;
    private final VenueNameElement.VenueGroup mVenueGroup;
    private final VenueNameElement.VenueType mVenueType;

    /*
     * From HS20 Indication element:
     * mHSRelease is null only if the HS20 Indication element was not present.
     * mAnqpDomainID is set to -1 if not present in the element.
     */
    private final HSRelease mHSRelease;
    private final int mAnqpDomainID;

    /*
     * From beacon:
     * mAnqpOICount is how many additional OIs are available through ANQP.
     * mRoamingConsortiums is either null, if the element was not present, or is an array of
     * 1, 2 or 3 longs in which the roaming consortium values occupy the LSBs.
     */
    private final int mAnqpOICount;
    private final long[] mRoamingConsortiums;
    private int mDtimInterval = -1;

    private final InformationElementUtil.ExtendedCapabilities mExtendedCapabilities;

    private final Map<Constants.ANQPElementType, ANQPElement> mANQPElements;

    public NetworkDetail(String bssid, ScanResult.InformationElement[] infoElements,
            List<String> anqpLines, int freq) {
        if (infoElements == null) {
            throw new IllegalArgumentException("Null information elements");
        }

        mBSSID = Utils.parseMac(bssid);

        String ssid = null;
        boolean isHiddenSsid = false;
        byte[] ssidOctets = null;

        InformationElementUtil.BssLoad bssLoad = new InformationElementUtil.BssLoad();

        InformationElementUtil.Interworking interworking =
                new InformationElementUtil.Interworking();

        InformationElementUtil.RoamingConsortium roamingConsortium =
                new InformationElementUtil.RoamingConsortium();

        InformationElementUtil.Vsa vsa = new InformationElementUtil.Vsa();

        InformationElementUtil.HtOperation htOperation = new InformationElementUtil.HtOperation();
        InformationElementUtil.VhtOperation vhtOperation =
                new InformationElementUtil.VhtOperation();

        InformationElementUtil.ExtendedCapabilities extendedCapabilities =
                new InformationElementUtil.ExtendedCapabilities();

        InformationElementUtil.TrafficIndicationMap trafficIndicationMap =
                new InformationElementUtil.TrafficIndicationMap();

        InformationElementUtil.SupportedRates supportedRates =
                new InformationElementUtil.SupportedRates();
        InformationElementUtil.SupportedRates extendedSupportedRates =
                new InformationElementUtil.SupportedRates();

        RuntimeException exception = null;

        ArrayList<Integer> iesFound = new ArrayList<Integer>();
        try {
            for (ScanResult.InformationElement ie : infoElements) {
                iesFound.add(ie.id);
                switch (ie.id) {
                    case ScanResult.InformationElement.EID_SSID:
                        ssidOctets = ie.bytes;
                        break;
                    case ScanResult.InformationElement.EID_BSS_LOAD:
                        bssLoad.from(ie);
                        break;
                    case ScanResult.InformationElement.EID_HT_OPERATION:
                        htOperation.from(ie);
                        break;
                    case ScanResult.InformationElement.EID_VHT_OPERATION:
                        vhtOperation.from(ie);
                        break;
                    case ScanResult.InformationElement.EID_INTERWORKING:
                        interworking.from(ie);
                        break;
                    case ScanResult.InformationElement.EID_ROAMING_CONSORTIUM:
                        roamingConsortium.from(ie);
                        break;
                    case ScanResult.InformationElement.EID_VSA:
                        vsa.from(ie);
                        break;
                    case ScanResult.InformationElement.EID_EXTENDED_CAPS:
                        extendedCapabilities.from(ie);
                        break;
                    case ScanResult.InformationElement.EID_TIM:
                        trafficIndicationMap.from(ie);
                        break;
                    case ScanResult.InformationElement.EID_SUPPORTED_RATES:
                        supportedRates.from(ie);
                        break;
                    case ScanResult.InformationElement.EID_EXTENDED_SUPPORTED_RATES:
                        extendedSupportedRates.from(ie);
                        break;
                    default:
                        break;
                }
            }
        }
        catch (IllegalArgumentException | BufferUnderflowException | ArrayIndexOutOfBoundsException e) {
            Log.d(Utils.hs2LogTag(getClass()), "Caught " + e);
            if (ssidOctets == null) {
                throw new IllegalArgumentException("Malformed IE string (no SSID)", e);
            }
            exception = e;
        }
        if (ssidOctets != null) {
            /*
             * Strict use of the "UTF-8 SSID" bit by APs appears to be spotty at best even if the
             * encoding truly is in UTF-8. An unconditional attempt to decode the SSID as UTF-8 is
             * therefore always made with a fall back to 8859-1 under normal circumstances.
             * If, however, a previous exception was detected and the UTF-8 bit is set, failure to
             * decode the SSID will be used as an indication that the whole frame is malformed and
             * an exception will be triggered.
             */
            CharsetDecoder decoder = StandardCharsets.UTF_8.newDecoder();
            try {
                CharBuffer decoded = decoder.decode(ByteBuffer.wrap(ssidOctets));
                ssid = decoded.toString();
            }
            catch (CharacterCodingException cce) {
                ssid = null;
            }

            if (ssid == null) {
                if (extendedCapabilities.isStrictUtf8() && exception != null) {
                    throw new IllegalArgumentException("Failed to decode SSID in dubious IE string");
                }
                else {
                    ssid = new String(ssidOctets, StandardCharsets.ISO_8859_1);
                }
            }
            isHiddenSsid = true;
            for (byte byteVal : ssidOctets) {
                if (byteVal != 0) {
                    isHiddenSsid = false;
                    break;
                }
            }
        }

        mSSID = ssid;
        mHESSID = interworking.hessid;
        mIsHiddenSsid = isHiddenSsid;
        mStationCount = bssLoad.stationCount;
        mChannelUtilization = bssLoad.channelUtilization;
        mCapacity = bssLoad.capacity;
        mAnt = interworking.ant;
        mInternet = interworking.internet;
        mVenueGroup = interworking.venueGroup;
        mVenueType = interworking.venueType;
        mHSRelease = vsa.hsRelease;
        mAnqpDomainID = vsa.anqpDomainID;
        mAnqpOICount = roamingConsortium.anqpOICount;
        mRoamingConsortiums = roamingConsortium.roamingConsortiums;
        mExtendedCapabilities = extendedCapabilities;
        mANQPElements = SupplicantBridge.parseANQPLines(anqpLines);
        //set up channel info
        mPrimaryFreq = freq;

        if (vhtOperation.isValid()) {
            // 80 or 160 MHz
            mChannelWidth = vhtOperation.getChannelWidth();
            mCenterfreq0 = vhtOperation.getCenterFreq0();
            mCenterfreq1 = vhtOperation.getCenterFreq1();
        } else {
            mChannelWidth = htOperation.getChannelWidth();
            mCenterfreq0 = htOperation.getCenterFreq0(mPrimaryFreq);
            mCenterfreq1  = 0;
        }

        // If trafficIndicationMap is not valid, mDtimPeriod will be negative
        if (trafficIndicationMap.isValid()) {
            mDtimInterval = trafficIndicationMap.mDtimPeriod;
        }

        int maxRateA = 0;
        int maxRateB = 0;
        // If we got some Extended supported rates, consider them, if not default to 0
        if (extendedSupportedRates.isValid()) {
            // rates are sorted from smallest to largest in InformationElement
            maxRateB = extendedSupportedRates.mRates.get(extendedSupportedRates.mRates.size() - 1);
        }
        // Only process the determination logic if we got a 'SupportedRates'
        if (supportedRates.isValid()) {
            maxRateA = supportedRates.mRates.get(supportedRates.mRates.size() - 1);
            mMaxRate = maxRateA > maxRateB ? maxRateA : maxRateB;
            mWifiMode = InformationElementUtil.WifiMode.determineMode(mPrimaryFreq, mMaxRate,
                    vhtOperation.isValid(),
                    iesFound.contains(ScanResult.InformationElement.EID_HT_OPERATION),
                    iesFound.contains(ScanResult.InformationElement.EID_ERP));
        } else {
            mWifiMode = 0;
            mMaxRate = 0;
            Log.w("WifiMode", mSSID + ", Invalid SupportedRates!!!");
        }
        if (VDBG) {
            Log.d(TAG, mSSID + "ChannelWidth is: " + mChannelWidth + " PrimaryFreq: " + mPrimaryFreq
                    + " mCenterfreq0: " + mCenterfreq0 + " mCenterfreq1: " + mCenterfreq1
                    + (extendedCapabilities.is80211McRTTResponder ? "Support RTT reponder"
                    : "Do not support RTT responder"));
            Log.v("WifiMode", mSSID
                    + ", WifiMode: " + InformationElementUtil.WifiMode.toString(mWifiMode)
                    + ", Freq: " + mPrimaryFreq
                    + ", mMaxRate: " + mMaxRate
                    + ", VHT: " + String.valueOf(vhtOperation.isValid())
                    + ", HT: " + String.valueOf(
                    iesFound.contains(ScanResult.InformationElement.EID_HT_OPERATION))
                    + ", ERP: " + String.valueOf(
                    iesFound.contains(ScanResult.InformationElement.EID_ERP))
                    + ", SupportedRates: " + supportedRates.toString()
                    + " ExtendedSupportedRates: " + extendedSupportedRates.toString());
        }
    }

    private static ByteBuffer getAndAdvancePayload(ByteBuffer data, int plLength) {
        ByteBuffer payload = data.duplicate().order(data.order());
        payload.limit(payload.position() + plLength);
        data.position(data.position() + plLength);
        return payload;
    }

    private NetworkDetail(NetworkDetail base, Map<Constants.ANQPElementType, ANQPElement> anqpElements) {
        mSSID = base.mSSID;
        mIsHiddenSsid = base.mIsHiddenSsid;
        mBSSID = base.mBSSID;
        mHESSID = base.mHESSID;
        mStationCount = base.mStationCount;
        mChannelUtilization = base.mChannelUtilization;
        mCapacity = base.mCapacity;
        mAnt = base.mAnt;
        mInternet = base.mInternet;
        mVenueGroup = base.mVenueGroup;
        mVenueType = base.mVenueType;
        mHSRelease = base.mHSRelease;
        mAnqpDomainID = base.mAnqpDomainID;
        mAnqpOICount = base.mAnqpOICount;
        mRoamingConsortiums = base.mRoamingConsortiums;
        mExtendedCapabilities =
                new InformationElementUtil.ExtendedCapabilities(base.mExtendedCapabilities);
        mANQPElements = anqpElements;
        mChannelWidth = base.mChannelWidth;
        mPrimaryFreq = base.mPrimaryFreq;
        mCenterfreq0 = base.mCenterfreq0;
        mCenterfreq1 = base.mCenterfreq1;
        mDtimInterval = base.mDtimInterval;
        mWifiMode = base.mWifiMode;
        mMaxRate = base.mMaxRate;
    }

    public NetworkDetail complete(Map<Constants.ANQPElementType, ANQPElement> anqpElements) {
        return new NetworkDetail(this, anqpElements);
    }

    public boolean queriable(List<Constants.ANQPElementType> queryElements) {
        return mAnt != null &&
                (Constants.hasBaseANQPElements(queryElements) ||
                 Constants.hasR2Elements(queryElements) && mHSRelease == HSRelease.R2);
    }

    public boolean has80211uInfo() {
        return mAnt != null || mRoamingConsortiums != null || mHSRelease != null;
    }

    public boolean hasInterworking() {
        return mAnt != null;
    }

    public String getSSID() {
        return mSSID;
    }

    public String getTrimmedSSID() {
        for (int n = 0; n < mSSID.length(); n++) {
            if (mSSID.charAt(n) != 0) {
                return mSSID;
            }
        }
        return "";
    }

    public long getHESSID() {
        return mHESSID;
    }

    public long getBSSID() {
        return mBSSID;
    }

    public int getStationCount() {
        return mStationCount;
    }

    public int getChannelUtilization() {
        return mChannelUtilization;
    }

    public int getCapacity() {
        return mCapacity;
    }

    public boolean isInterworking() {
        return mAnt != null;
    }

    public Ant getAnt() {
        return mAnt;
    }

    public boolean isInternet() {
        return mInternet;
    }

    public VenueNameElement.VenueGroup getVenueGroup() {
        return mVenueGroup;
    }

    public VenueNameElement.VenueType getVenueType() {
        return mVenueType;
    }

    public HSRelease getHSRelease() {
        return mHSRelease;
    }

    public int getAnqpDomainID() {
        return mAnqpDomainID;
    }

    public byte[] getOsuProviders() {
        if (mANQPElements == null) {
            return null;
        }
        ANQPElement osuProviders = mANQPElements.get(Constants.ANQPElementType.HSOSUProviders);
        return osuProviders != null ? ((RawByteElement) osuProviders).getPayload() : null;
    }

    public int getAnqpOICount() {
        return mAnqpOICount;
    }

    public long[] getRoamingConsortiums() {
        return mRoamingConsortiums;
    }

    public Long getExtendedCapabilities() {
        return mExtendedCapabilities.extendedCapabilities;
    }

    public Map<Constants.ANQPElementType, ANQPElement> getANQPElements() {
        return mANQPElements;
    }

    public int getChannelWidth() {
        return mChannelWidth;
    }

    public int getCenterfreq0() {
        return mCenterfreq0;
    }

    public int getCenterfreq1() {
        return mCenterfreq1;
    }

    public int getWifiMode() {
        return mWifiMode;
    }

    public int getDtimInterval() {
        return mDtimInterval;
    }

    public boolean is80211McResponderSupport() {
        return mExtendedCapabilities.is80211McRTTResponder;
    }

    public boolean isSSID_UTF8() {
        return mExtendedCapabilities.isStrictUtf8();
    }

    @Override
    public boolean equals(Object thatObject) {
        if (this == thatObject) {
            return true;
        }
        if (thatObject == null || getClass() != thatObject.getClass()) {
            return false;
        }

        NetworkDetail that = (NetworkDetail)thatObject;

        return getSSID().equals(that.getSSID()) && getBSSID() == that.getBSSID();
    }

    @Override
    public int hashCode() {
        return ((mSSID.hashCode() * 31) + (int)(mBSSID >>> 32)) * 31 + (int)mBSSID;
    }

    @Override
    public String toString() {
        return String.format("NetworkInfo{SSID='%s', HESSID=%x, BSSID=%x, StationCount=%d, " +
                "ChannelUtilization=%d, Capacity=%d, Ant=%s, Internet=%s, " +
                "VenueGroup=%s, VenueType=%s, HSRelease=%s, AnqpDomainID=%d, " +
                "AnqpOICount=%d, RoamingConsortiums=%s}",
                mSSID, mHESSID, mBSSID, mStationCount,
                mChannelUtilization, mCapacity, mAnt, mInternet,
                mVenueGroup, mVenueType, mHSRelease, mAnqpDomainID,
                mAnqpOICount, Utils.roamingConsortiumsToString(mRoamingConsortiums));
    }

    public String toKeyString() {
        return mHESSID != 0 ?
            String.format("'%s':%012x (%012x)", mSSID, mBSSID, mHESSID) :
            String.format("'%s':%012x", mSSID, mBSSID);
    }

    public String getBSSIDString() {
        return toMACString(mBSSID);
    }

    /**
     * Evaluates the ScanResult this NetworkDetail is built from
     * returns true if built from a Beacon Frame
     * returns false if built from a Probe Response
     */
    public boolean isBeaconFrame() {
        // Beacon frames have a 'Traffic Indication Map' Information element
        // Probe Responses do not. This is indicated by a DTIM period > 0
        return mDtimInterval > 0;
    }

    /**
     * Evaluates the ScanResult this NetworkDetail is built from
     * returns true if built from a hidden Beacon Frame
     * returns false if not hidden or not a Beacon
     */
    public boolean isHiddenBeaconFrame() {
        // Hidden networks are not 80211 standard, but it is common for a hidden network beacon
        // frame to either send zero-value bytes as the SSID, or to send no bytes at all.
        return isBeaconFrame() && mIsHiddenSsid;
    }

    public static String toMACString(long mac) {
        StringBuilder sb = new StringBuilder();
        boolean first = true;
        for (int n = BYTES_IN_EUI48 - 1; n >= 0; n--) {
            if (first) {
                first = false;
            } else {
                sb.append(':');
            }
            sb.append(String.format("%02x", (mac >>> (n * Byte.SIZE)) & BYTE_MASK));
        }
        return sb.toString();
    }
}
