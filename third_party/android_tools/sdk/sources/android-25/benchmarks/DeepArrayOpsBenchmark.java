/*
 * Copyright (C) 2013 The Android Open Source Project
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
 * limitations under the License
 */

package benchmarks;

import com.google.caliper.Param;
import java.lang.reflect.Array;
import java.lang.reflect.Constructor;
import java.util.Arrays;

public class DeepArrayOpsBenchmark {
    @Param({"1", "4", "16", "256", "2048"}) int arrayLength;

    private Object[] array;
    private Object[] array2;

    private Object[] array3;
    private Object[] array4;

    protected void setUp() throws Exception {
        array = new Object[arrayLength * 13];
        array2 = new Object[arrayLength * 13];
        for (int i = 0; i < arrayLength; i += 13) {
            array[i] = new IntWrapper(i);
            array2[i] = new IntWrapper(i);

            array[i + 1] = new16ElementObjectarray();
            array2[i + 1] = new16ElementObjectarray();

            array[i + 2] = new boolean[16];
            array2[i + 2] = new boolean[16];

            array[i + 3] = new byte[16];
            array2[i + 3] = new byte[16];

            array[i + 4] = new char[16];
            array2[i + 4] = new char[16];

            array[i + 5] = new short[16];
            array2[i + 5] = new short[16];

            array[i + 6] = new float[16];
            array2[i + 6] = new float[16];

            array[i + 7] = new long[16];
            array2[i + 7] = new long[16];

            array[i + 8] = new int[16];
            array2[i + 8] = new int[16];

            array[i + 9] = new double[16];
            array2[i + 9] = new double[16];

            // Subarray types are concrete objects.
            array[i + 10] = new16ElementArray(String.class, String.class);
            array2[i + 10] = new16ElementArray(String.class, String.class);

            array[i + 11] = new16ElementArray(Integer.class, Integer.class);
            array2[i + 11] = new16ElementArray(Integer.class, Integer.class);

            // Subarray types is an interface.
            array[i + 12] = new16ElementArray(CharSequence.class, String.class);
            array2[i + 12] = new16ElementArray(CharSequence.class, String.class);
        }
    }

    public void timeDeepHashCode(int reps) {
        for (int i = 0; i < reps; ++i) {
            Arrays.deepHashCode(array);
        }
    }

    public void timeEquals(int reps) {
        for (int i = 0; i < reps; ++i) {
            Arrays.deepEquals(array, array2);
        }
    }

    private static final Object[] new16ElementObjectarray() {
        Object[] array = new Object[16];
        for (int i = 0; i < 16; ++i) {
            array[i] = new IntWrapper(i);
        }

        return array;
    }

    @SuppressWarnings("unchecked")
    private static final <T, V> T[] new16ElementArray(Class<T> arrayType, Class<V> type)
            throws Exception {
        T[] array = (T []) Array.newInstance(type, 16);
        if (!arrayType.isAssignableFrom(type)) {
            throw new IllegalArgumentException(arrayType + " is not assignable from " + type);
        }

        Constructor<V> constructor = type.getDeclaredConstructor(String.class);
        for (int i = 0; i < 16; ++i) {
            array[i] = (T) constructor.newInstance(String.valueOf(i + 1000));
        }

        return array;
    }

    /**
     * A class that provides very basic equals() and hashCode() operations
     * and doesn't resort to memoization tricks like {@link java.lang.Integer}.
     *
     * Useful for providing equal objects that aren't the same (a.equals(b) but
     * a != b).
     */
    public static final class IntWrapper {
        private final int wrapped;

        public IntWrapper(int wrap) {
            wrapped = wrap;
        }

        @Override
        public int hashCode() {
            return wrapped;
        }

        @Override
        public boolean equals(Object o) {
            if (!(o instanceof IntWrapper)) {
                return false;
            }

            return ((IntWrapper) o).wrapped == this.wrapped;
        }
    }
}

