/*
 * Copyright (C) 2014 The Android Open Source Project
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

package benchmarks.regression;

import com.google.caliper.Param;
import java.util.ArrayList;
import java.util.Collections;
import java.util.Comparator;
import java.util.List;
import java.util.Random;
import java.util.Vector;

public class CollectionsBenchmark {
    @Param({"4", "16", "64", "256", "1024"})
    private int arrayListLength;

    public static Comparator<Integer> REVERSE = new Comparator<Integer>() {
        @Override
        public int compare(Integer lhs, Integer rhs) {
            int lhsAsInt = lhs.intValue();
            int rhsAsInt = rhs.intValue();
            return rhsAsInt < lhsAsInt ? -1 : (lhsAsInt == rhsAsInt ? 0 : 1);
        }
    };


    public void timeSort_arrayList(int nreps) throws Exception {
        List<Integer> input = buildList(arrayListLength, ArrayList.class);
        for (int i = 0; i < nreps; ++i) {
            Collections.sort(input);
        }
    }

    public void timeSortWithComparator_arrayList(int nreps) throws Exception {
        List<Integer> input = buildList(arrayListLength, ArrayList.class);
        for (int i = 0; i < nreps; ++i) {
            Collections.sort(input, REVERSE);
        }
    }

    public void timeSort_vector(int nreps) throws Exception {
        List<Integer> input = buildList(arrayListLength, Vector.class);
        for (int i = 0; i < nreps; ++i) {
            Collections.sort(input);
        }
    }

    public void timeSortWithComparator_vector(int nreps) throws Exception {
        List<Integer> input = buildList(arrayListLength, Vector.class);
        for (int i = 0; i < nreps; ++i) {
            Collections.sort(input, REVERSE);
        }
    }

    private static <T extends List<Integer>> List<Integer> buildList(
            int arrayListLength, Class<T> listClass) throws Exception {
        Random random = new Random();
        random.setSeed(0);
        List<Integer> list = listClass.newInstance();
        for (int i = 0; i < arrayListLength; ++i) {
            list.add(random.nextInt());
        }
        return list;
    }
}
