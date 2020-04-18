package android.bordeaux.services;

import android.os.Parcel;
import android.os.Parcelable;

public final class IntFloat implements Parcelable {
    public int index;
    public float value;

    public static final Parcelable.Creator<IntFloat> CREATOR = new Parcelable.Creator<IntFloat>() {
        public IntFloat createFromParcel(Parcel in) {
            return new IntFloat(in);
        }

        public IntFloat[] newArray(int size) {
            return new IntFloat[size];
        }
    };

    public IntFloat() {
    }

    private IntFloat(Parcel in) {
        readFromParcel(in);
    }

    public int describeContents() {
        return 0;
    }

    public void writeToParcel(Parcel out, int flags) {
        out.writeInt(index);
        out.writeFloat(value);
    }

    public void readFromParcel(Parcel in) {
        index = in.readInt();
        value = in.readFloat();
    }
}
