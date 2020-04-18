package android.bordeaux.services;

import android.os.Parcel;
import android.os.Parcelable;

public final class StringString implements Parcelable {
    public String key;
    public String value;

    public static final Parcelable.Creator<StringString>
                        CREATOR = new Parcelable.Creator<StringString>() {
        public StringString createFromParcel(Parcel in) {
            return new StringString(in);
        }

        public StringString[] newArray(int size) {
            return new StringString[size];
        }
    };

    public StringString() {
    }

    private StringString(Parcel in) {
        readFromParcel(in);
    }

    public int describeContents() {
        return 0;
    }

    public void writeToParcel(Parcel out, int flags) {
        out.writeString(key);
        out.writeString(value);
    }

    public void readFromParcel(Parcel in) {
        key  = in.readString();
        value = in.readString();
    }
}
