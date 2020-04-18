package android.bordeaux.services;

import android.os.Parcel;
import android.os.Parcelable;

public final class StringFloat implements Parcelable {
    public String key;
    public float value;

    public static final Parcelable.Creator<StringFloat>
                        CREATOR = new Parcelable.Creator<StringFloat>() {
        public StringFloat createFromParcel(Parcel in) {
            return new StringFloat(in);
        }

        public StringFloat[] newArray(int size) {
            return new StringFloat[size];
        }
    };

    public StringFloat() {
    }

    public StringFloat(String newKey, float newValue) {
        key = newKey;
        value = newValue;
    }

    private StringFloat(Parcel in) {
        readFromParcel(in);
    }

    public int describeContents() {
        return 0;
    }

    public void writeToParcel(Parcel out, int flags) {
        out.writeString(key);
        out.writeFloat(value);
    }

    public void readFromParcel(Parcel in) {
        key  = in.readString();
        value = in.readFloat();
    }
}
