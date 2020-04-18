package com.android.server.wifi.hotspot2.omadm;

public class NodeAttribute {
    private final String mName;
    private final String mType;
    private final String mValue;

    public NodeAttribute(String name, String type, String value) {
        mName = name;
        mType = type;
        mValue = value;
    }

    public String getName() {
        return mName;
    }

    public String getValue() {
        return mValue;
    }

    public String getType() {
        return mType;
    }

    @Override
    public boolean equals(Object thatObject) {
        if (this == thatObject) {
            return true;
        }
        if (thatObject == null || getClass() != thatObject.getClass()) {
            return false;
        }

        NodeAttribute that = (NodeAttribute) thatObject;
        return mName.equals(that.mName) && mType.equals(that.mType) && mValue.equals(that.mValue);
    }

    @Override
    public String toString() {
        return String.format("%s (%s) = '%s'", mName, mType, mValue);
    }
}
