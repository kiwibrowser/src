package com.android.server.wifi.anqp;

import java.net.ProtocolException;
import java.nio.ByteBuffer;
import java.util.ArrayList;
import java.util.Collections;
import java.util.EnumMap;
import java.util.List;
import java.util.Map;

/**
 * The Venue Name ANQP Element, IEEE802.11-2012 section 8.4.4.4
 */
public class VenueNameElement extends ANQPElement {
    private final VenueGroup mGroup;
    private final VenueType mType;
    private final List<I18Name> mNames;

    private static final Map<VenueGroup, Integer> sGroupBases =
            new EnumMap<VenueGroup, Integer>(VenueGroup.class);

    public VenueNameElement(Constants.ANQPElementType infoID, ByteBuffer payload)
            throws ProtocolException {
        super(infoID);

        if (payload.remaining() < 2)
            throw new ProtocolException("Runt Venue Name");

        int group = payload.get() & Constants.BYTE_MASK;
        int type = payload.get() & Constants.BYTE_MASK;

        if (group >= VenueGroup.Reserved.ordinal()) {
            mGroup = VenueGroup.Reserved;
            mType = VenueType.Reserved;
        } else {
            mGroup = VenueGroup.values()[group];
            type += sGroupBases.get(mGroup);
            if (type >= VenueType.Reserved.ordinal()) {
                mType = VenueType.Reserved;
            } else {
                mType = VenueType.values()[type];
            }
        }

        mNames = new ArrayList<I18Name>();
        while (payload.hasRemaining()) {
            mNames.add(new I18Name(payload));
        }
    }

    public VenueGroup getGroup() {
        return mGroup;
    }

    public VenueType getType() {
        return mType;
    }

    public List<I18Name> getNames() {
        return Collections.unmodifiableList(mNames);
    }

    @Override
    public String toString() {
        return "VenueName{" +
                "m_group=" + mGroup +
                ", m_type=" + mType +
                ", m_names=" + mNames +
                '}';
    }

    public enum VenueGroup {
        Unspecified,
        Assembly,
        Business,
        Educational,
        FactoryIndustrial,
        Institutional,
        Mercantile,
        Residential,
        Storage,
        UtilityMiscellaneous,
        Vehicular,
        Outdoor,
        Reserved  // Note: this must be the last enum constant
    }

    public enum VenueType {
        Unspecified,

        UnspecifiedAssembly,
        Arena,
        Stadium,
        PassengerTerminal,
        Amphitheater,
        AmusementPark,
        PlaceOfWorship,
        ConventionCenter,
        Library,
        Museum,
        Restaurant,
        Theater,
        Bar,
        CoffeeShop,
        ZooOrAquarium,
        EmergencyCoordinationCenter,

        UnspecifiedBusiness,
        DoctorDentistoffice,
        Bank,
        FireStation,
        PoliceStation,
        PostOffice,
        ProfessionalOffice,
        ResearchDevelopmentFacility,
        AttorneyOffice,

        UnspecifiedEducational,
        SchoolPrimary,
        SchoolSecondary,
        UniversityCollege,

        UnspecifiedFactoryIndustrial,
        Factory,

        UnspecifiedInstitutional,
        Hospital,
        LongTermCareFacility,
        AlcoholAndDrugRehabilitationCenter,
        GroupHome,
        PrisonJail,

        UnspecifiedMercantile,
        RetailStore,
        GroceryMarket,
        AutomotiveServiceStation,
        ShoppingMall,
        GasStation,

        UnspecifiedResidential,
        PrivateResidence,
        HotelMotel,
        Dormitory,
        BoardingHouse,

        UnspecifiedStorage,

        UnspecifiedUtilityMiscellaneous,

        UnspecifiedVehicular,
        AutomobileOrTruck,
        Airplane,
        Bus,
        Ferry,
        ShipOrBoat,
        Train,
        MotorBike,

        UnspecifiedOutdoor,
        MuniMeshNetwork,
        CityPark,
        RestArea,
        TrafficControl,
        BusStop,
        Kiosk,

        Reserved  // Note: this must be the last enum constant
    }

    private static final VenueType[] PerGroup =
            {
                    VenueType.Unspecified,
                    VenueType.UnspecifiedAssembly,
                    VenueType.UnspecifiedBusiness,
                    VenueType.UnspecifiedEducational,
                    VenueType.UnspecifiedFactoryIndustrial,
                    VenueType.UnspecifiedInstitutional,
                    VenueType.UnspecifiedMercantile,
                    VenueType.UnspecifiedResidential,
                    VenueType.UnspecifiedStorage,
                    VenueType.UnspecifiedUtilityMiscellaneous,
                    VenueType.UnspecifiedVehicular,
                    VenueType.UnspecifiedOutdoor
            };

    static {
        int index = 0;
        for (VenueType venue : PerGroup) {
            sGroupBases.put(VenueGroup.values()[index++], venue.ordinal());
        }
    }
}
