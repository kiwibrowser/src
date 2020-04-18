/*
 * Copyright (C) 2015 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
package android.databinding.testapp.vo;

import android.databinding.ObservableBoolean;
import android.databinding.ObservableByte;
import android.databinding.ObservableChar;
import android.databinding.ObservableDouble;
import android.databinding.ObservableField;
import android.databinding.ObservableFloat;
import android.databinding.ObservableInt;
import android.databinding.ObservableLong;
import android.databinding.ObservableParcelable;
import android.databinding.ObservableShort;
import android.os.Parcel;
import android.os.Parcelable;

public class ObservableFieldBindingObject {
    public final ObservableBoolean bField = new ObservableBoolean();
    public final ObservableByte tField = new ObservableByte();
    public final ObservableShort sField = new ObservableShort();
    public final ObservableChar cField = new ObservableChar();
    public final ObservableInt iField = new ObservableInt();
    public final ObservableLong lField = new ObservableLong();
    public final ObservableFloat fField = new ObservableFloat();
    public final ObservableDouble dField = new ObservableDouble();
    public final ObservableParcelable<MyParcelable> pField;
    public final ObservableField<String> oField = new ObservableField<>();

    public ObservableFieldBindingObject() {
        oField.set("Hello");
        MyParcelable myParcelable = new MyParcelable(3, "abc");
        pField = new ObservableParcelable(myParcelable);
    }

    public static class MyParcelable implements Parcelable {
        int x;
        String y;

        public MyParcelable(int x, String y) {
            this.x = x;
            this.y = y;
        }

        @Override
        public int describeContents() {
            return 0;
        }

        @Override
        public void writeToParcel(Parcel dest, int flags) {
            dest.writeInt(x);
            dest.writeString(y);
        }

        @Override
        public boolean equals(Object o) {
            if (this == o) {
                return true;
            }
            if (o == null || getClass() != o.getClass()) {
                return false;
            }

            MyParcelable that = (MyParcelable) o;

            if (x != that.x) {
                return false;
            }
            if (y != null ? !y.equals(that.y) : that.y != null) {
                return false;
            }

            return true;
        }

        public int getX() {
            return x;
        }

        public String getY() {
            return y;
        }

        @Override
        public int hashCode() {
            int result = x;
            result = 31 * result + (y != null ? y.hashCode() : 0);
            return result;
        }

        public static final Parcelable.Creator<MyParcelable> CREATOR
                = new Parcelable.Creator<MyParcelable>() {

            @Override
            public MyParcelable createFromParcel(Parcel source) {
                return new MyParcelable(source.readInt(), source.readString());
            }

            @Override
            public MyParcelable[] newArray(int size) {
                return new MyParcelable[size];
            }
        };
    }
}
