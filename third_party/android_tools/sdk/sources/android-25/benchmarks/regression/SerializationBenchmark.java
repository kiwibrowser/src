/*
 * Copyright (C) 2010 Google Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package benchmarks.regression;

import java.io.ByteArrayInputStream;
import java.io.ByteArrayOutputStream;
import java.io.ObjectInputStream;
import java.io.ObjectOutputStream;
import java.io.ObjectStreamClass;
import java.io.Serializable;
import java.util.ArrayList;

public class SerializationBenchmark {
    private static byte[] bytes(Object o) throws Exception {
        ByteArrayOutputStream baos = new ByteArrayOutputStream(1024);
        ObjectOutputStream out = new ObjectOutputStream(baos);
        out.writeObject(o);
        out.close();
        return baos.toByteArray();
    }

    public void timeReadIntArray(int reps) throws Exception {
        int[] intArray = new int[256];
        readSingleObject(reps, intArray);
    }

    public void timeWriteIntArray(int reps) throws Exception {
        int[] intArray = new int[256];
        writeSingleObject(reps, intArray);
    }
    public void timeReadArrayListInteger(int reps) throws Exception {
        ArrayList<Integer> object = new ArrayList<Integer>();
        for (int i = 0; i < 256; ++i) {
            object.add(i);
        }
        readSingleObject(reps, object);
    }

    public void timeWriteArrayListInteger(int reps) throws Exception {
        ArrayList<Integer> object = new ArrayList<Integer>();
        for (int i = 0; i < 256; ++i) {
            object.add(i);
        }
        writeSingleObject(reps, object);
    }

    public void timeReadString(int reps) throws Exception {
        readSingleObject(reps, "hello");
    }

    public void timeReadObjectStreamClass(int reps) throws Exception {
        // A special case because serialization itself requires this class.
        // (This should really be a unit test.)
        ObjectStreamClass osc = ObjectStreamClass.lookup(String.class);
        readSingleObject(reps, osc);
    }

    public void timeWriteString(int reps) throws Exception {
        // String is a special case that avoids JNI.
        writeSingleObject(reps, "hello");
    }

    public void timeWriteObjectStreamClass(int reps) throws Exception {
        // A special case because serialization itself requires this class.
        // (This should really be a unit test.)
        ObjectStreamClass osc = ObjectStreamClass.lookup(String.class);
        writeSingleObject(reps, osc);
    }

    // This is a baseline for the others.
    public void timeWriteNoObjects(int reps) throws Exception {
        ByteArrayOutputStream baos = new ByteArrayOutputStream(1024);
        ObjectOutputStream out = new ObjectOutputStream(baos);
        for (int rep = 0; rep < reps; ++rep) {
            out.reset();
            baos.reset();
        }
        out.close();
    }

    private void readSingleObject(int reps, Object object) throws Exception {
        byte[] bytes = bytes(object);
        ByteArrayInputStream bais = new ByteArrayInputStream(bytes);
        for (int rep = 0; rep < reps; ++rep) {
            ObjectInputStream in = new ObjectInputStream(bais);
            in.readObject();
            in.close();
            bais.reset();
        }
    }

    private void writeSingleObject(int reps, Object o) throws Exception {
        ByteArrayOutputStream baos = new ByteArrayOutputStream(1024);
        ObjectOutputStream out = new ObjectOutputStream(baos);
        for (int rep = 0; rep < reps; ++rep) {
            out.writeObject(o);
            out.reset();
            baos.reset();
        }
        out.close();
    }

    public void timeWriteEveryKindOfField(int reps) throws Exception {
        writeSingleObject(reps, new LittleBitOfEverything());
    }
    public void timeWriteSerializableBoolean(int reps) throws Exception {
        writeSingleObject(reps, new SerializableBoolean());
    }
    public void timeWriteSerializableByte(int reps) throws Exception {
        writeSingleObject(reps, new SerializableByte());
    }
    public void timeWriteSerializableChar(int reps) throws Exception {
        writeSingleObject(reps, new SerializableChar());
    }
    public void timeWriteSerializableDouble(int reps) throws Exception {
        writeSingleObject(reps, new SerializableDouble());
    }
    public void timeWriteSerializableFloat(int reps) throws Exception {
        writeSingleObject(reps, new SerializableFloat());
    }
    public void timeWriteSerializableInt(int reps) throws Exception {
        writeSingleObject(reps, new SerializableInt());
    }
    public void timeWriteSerializableLong(int reps) throws Exception {
        writeSingleObject(reps, new SerializableLong());
    }
    public void timeWriteSerializableShort(int reps) throws Exception {
        writeSingleObject(reps, new SerializableShort());
    }
    public void timeWriteSerializableReference(int reps) throws Exception {
        writeSingleObject(reps, new SerializableReference());
    }

    public void timeReadEveryKindOfField(int reps) throws Exception {
        readSingleObject(reps, new LittleBitOfEverything());
    }
    public void timeReadSerializableBoolean(int reps) throws Exception {
        readSingleObject(reps, new SerializableBoolean());
    }
    public void timeReadSerializableByte(int reps) throws Exception {
        readSingleObject(reps, new SerializableByte());
    }
    public void timeReadSerializableChar(int reps) throws Exception {
        readSingleObject(reps, new SerializableChar());
    }
    public void timeReadSerializableDouble(int reps) throws Exception {
        readSingleObject(reps, new SerializableDouble());
    }
    public void timeReadSerializableFloat(int reps) throws Exception {
        readSingleObject(reps, new SerializableFloat());
    }
    public void timeReadSerializableInt(int reps) throws Exception {
        readSingleObject(reps, new SerializableInt());
    }
    public void timeReadSerializableLong(int reps) throws Exception {
        readSingleObject(reps, new SerializableLong());
    }
    public void timeReadSerializableShort(int reps) throws Exception {
        readSingleObject(reps, new SerializableShort());
    }
    public void timeReadSerializableReference(int reps) throws Exception {
        readSingleObject(reps, new SerializableReference());
    }

    public static class SerializableBoolean implements Serializable {
        boolean z;
    }
    public static class SerializableByte implements Serializable {
        byte b;
    }
    public static class SerializableChar implements Serializable {
        char c;
    }
    public static class SerializableDouble implements Serializable {
        double d;
    }
    public static class SerializableFloat implements Serializable {
        float f;
    }
    public static class SerializableInt implements Serializable {
        int i;
    }
    public static class SerializableLong implements Serializable {
        long j;
    }
    public static class SerializableShort implements Serializable {
        short s;
    }
    public static class SerializableReference implements Serializable {
        Object l;
    }

    public static class LittleBitOfEverything implements Serializable {
        boolean z;
        byte b;
        char c;
        double d;
        float f;
        int i;
        long j;
        short s;
        Object l;
    }
}
