package com.android.server.wifi.hotspot2;

public class IconEvent {
    private final long mBSSID;
    private final String mFileName;
    private final int mSize;

    public IconEvent(long bssid, String fileName, int size) {
        mBSSID = bssid;
        mFileName = fileName;
        mSize = size;
    }

    public long getBSSID() {
        return mBSSID;
    }

    public String getFileName() {
        return mFileName;
    }

    public int getSize() {
        return mSize;
    }

    @Override
    public String toString() {
        return "IconEvent: " +
                "BSSID=" + String.format("%012x", mBSSID) +
                ", fileName='" + mFileName + '\'' +
                ", size=" + mSize;
    }
}
