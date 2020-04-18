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

package android.databinding;

import android.os.Parcel;
import android.os.Parcelable;
import android.test.AndroidTestCase;

import java.io.ByteArrayInputStream;
import java.io.ByteArrayOutputStream;
import java.io.Closeable;
import java.io.IOException;
import java.io.ObjectInputStream;
import java.io.ObjectOutputStream;
import java.io.Serializable;
import java.util.UUID;

public class ObservableParcelTest extends AndroidTestCase {
    public void testParcelInt() {
        ObservableInt observableInt = new ObservableInt();
        observableInt.set(Integer.MAX_VALUE - 1);
        ObservableInt read = parcelAndUnparcel(observableInt, ObservableInt.class);
        assertEquals(Integer.MAX_VALUE - 1, read.get());
    }

    public void testParcelBoolean() {
        ObservableBoolean obj = new ObservableBoolean(false);
        ObservableBoolean read = parcelAndUnparcel(obj, ObservableBoolean.class);
        assertFalse(read.get());

        ObservableBoolean obj2 = new ObservableBoolean(true);
        ObservableBoolean read2 = parcelAndUnparcel(obj2, ObservableBoolean.class);
        assertTrue(read2.get());
    }

    public void testParcelByte() {
        ObservableByte obj = new ObservableByte((byte) 7);
        ObservableByte read = parcelAndUnparcel(obj, ObservableByte.class);
        assertEquals((byte) 7, read.get());
    }

    public void testParcelChar() {
        ObservableChar obj = new ObservableChar('y');
        ObservableChar read = parcelAndUnparcel(obj, ObservableChar.class);
        assertEquals('y', read.get());
    }

    public void testParcelDouble() {
        ObservableDouble obj = new ObservableDouble(Double.MAX_VALUE);
        ObservableDouble read = parcelAndUnparcel(obj, ObservableDouble.class);
        assertEquals(Double.MAX_VALUE, read.get());
    }

    public void testParcelFloat() {
        ObservableFloat obj = new ObservableFloat(Float.MIN_VALUE);
        ObservableFloat read = parcelAndUnparcel(obj, ObservableFloat.class);
        assertEquals(Float.MIN_VALUE, read.get());
    }

    public void testParcelParcel() {
        MyParcelable myParcelable = new MyParcelable(5, "foo");
        ObservableParcelable<MyParcelable> obj = new ObservableParcelable<>(myParcelable);
        ObservableParcelable read = parcelAndUnparcel(obj,
                ObservableParcelable.class);
        assertEquals(myParcelable, read.get());
    }

    public void testParcelLong() {
        ObservableLong obj = new ObservableLong(Long.MAX_VALUE - 1);
        ObservableLong read = parcelAndUnparcel(obj, ObservableLong.class);
        assertEquals(Long.MAX_VALUE - 1, read.get());
    }

    public void testParcelShort() {
        ObservableShort obj = new ObservableShort(Short.MIN_VALUE);
        ObservableShort read = parcelAndUnparcel(obj, ObservableShort.class);
        assertEquals(Short.MIN_VALUE, read.get());
    }

    public void testSerializeInt() throws IOException, ClassNotFoundException {
        ObservableInt observableInt = new ObservableInt();
        observableInt.set(Integer.MAX_VALUE - 1);
        ObservableInt read = serializeAndDeserialize(observableInt, ObservableInt.class);
        assertEquals(Integer.MAX_VALUE - 1, read.get());
    }

    public void testSerializeBoolean() throws IOException, ClassNotFoundException {
        ObservableBoolean obj = new ObservableBoolean(false);
        ObservableBoolean read = serializeAndDeserialize(obj, ObservableBoolean.class);
        assertFalse(read.get());
        ObservableBoolean obj2 = new ObservableBoolean(true);
        ObservableBoolean read2 = serializeAndDeserialize(obj2, ObservableBoolean.class);
        assertTrue(read2.get());
    }

    public void testSerializeByte() throws IOException, ClassNotFoundException {
        ObservableByte obj = new ObservableByte((byte) 7);
        ObservableByte read = serializeAndDeserialize(obj, ObservableByte.class);
        assertEquals((byte) 7, read.get());
    }

    public void testSerializeChar() throws IOException, ClassNotFoundException {
        ObservableChar obj = new ObservableChar('y');
        ObservableChar read = serializeAndDeserialize(obj, ObservableChar.class);
        assertEquals('y', read.get());
    }

    public void testSerializeDouble() throws IOException, ClassNotFoundException {
        ObservableDouble obj = new ObservableDouble(Double.MAX_VALUE);
        ObservableDouble read = serializeAndDeserialize(obj, ObservableDouble.class);
        assertEquals(Double.MAX_VALUE, read.get());
    }

    public void testSerializeFloat() throws IOException, ClassNotFoundException {
        ObservableFloat obj = new ObservableFloat(Float.MIN_VALUE);
        ObservableFloat read = serializeAndDeserialize(obj, ObservableFloat.class);
        assertEquals(Float.MIN_VALUE, read.get());
    }

    public void testSerializeParcel() throws IOException, ClassNotFoundException {
        MyParcelable myParcelable = new MyParcelable(5, "foo");
        ObservableParcelable<MyParcelable> obj = new ObservableParcelable<>(myParcelable);
        ObservableParcelable read = serializeAndDeserialize(obj,
                ObservableParcelable.class);
        assertEquals(myParcelable, read.get());
    }

    public void testSerializeField() throws IOException, ClassNotFoundException {
        MyParcelable myParcelable = new MyParcelable(5, "foo");
        ObservableField<MyParcelable> obj = new ObservableField<>(myParcelable);
        ObservableField read = serializeAndDeserialize(obj, ObservableField.class);
        assertEquals(myParcelable, read.get());
    }

    public void testSerializeLong() throws IOException, ClassNotFoundException {
        ObservableLong obj = new ObservableLong(Long.MAX_VALUE - 1);
        ObservableLong read = serializeAndDeserialize(obj, ObservableLong.class);
        assertEquals(Long.MAX_VALUE - 1, read.get());
    }

    public void testSerializeShort() throws IOException, ClassNotFoundException {
        ObservableShort obj = new ObservableShort(Short.MIN_VALUE);
        ObservableShort read = serializeAndDeserialize(obj, ObservableShort.class);
        assertEquals(Short.MIN_VALUE, read.get());
    }

    private <T extends Parcelable> T parcelAndUnparcel(T t, Class<T> klass) {
        Parcel parcel = Parcel.obtain();
        parcel.writeParcelable(t, 0);
        // we append a suffix to the parcelable to test out of bounds
        String parcelSuffix = UUID.randomUUID().toString();
        parcel.writeString(parcelSuffix);
        // get ready to read
        parcel.setDataPosition(0);
        Parcelable parcelable = parcel.readParcelable(getClass().getClassLoader());
        assertNotNull(parcelable);
        assertEquals(klass, parcelable.getClass());
        assertEquals(parcelSuffix, parcel.readString());
        return (T) parcelable;
    }

    private <T> T serializeAndDeserialize(T t, Class<T> klass)
            throws IOException, ClassNotFoundException {
        ObjectOutputStream oos = null;
        ByteArrayOutputStream bos = null;
        String suffix = UUID.randomUUID().toString();
        try {
            bos = new ByteArrayOutputStream();
            oos = new ObjectOutputStream(bos);
            oos.writeObject(t);
            oos.writeObject(suffix);
        } finally {
            closeQuietly(bos);
            closeQuietly(oos);
        }
        ByteArrayInputStream bis = null;
        ObjectInputStream ois = null;
        try {
            bis = new ByteArrayInputStream(bos.toByteArray());
            ois = new ObjectInputStream(bis);
            Object o = ois.readObject();
            assertEquals(klass, o.getClass());
            assertEquals(suffix, ois.readObject());
            return (T) o;
        } finally {
            closeQuietly(bis);
            closeQuietly(ois);
        }
    }

    private static void closeQuietly(Closeable closeable) {
        try {
            if (closeable != null) {
                closeable.close();
            }
        } catch (IOException ignored) {
        }
    }

    public static class MyParcelable implements Parcelable, Serializable {
        int x;
        String y;

        public MyParcelable() {
        }

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
