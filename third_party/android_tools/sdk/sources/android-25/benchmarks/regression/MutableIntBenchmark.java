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

import com.google.caliper.Param;
import java.util.concurrent.atomic.AtomicInteger;

public final class MutableIntBenchmark {

    enum Kind {
        ARRAY() {
            int[] value = new int[1];

            @Override void timeCreate(int reps) {
                for (int i = 0; i < reps; i++) {
                    value = new int[] { 5 };
                }
            }
            @Override void timeIncrement(int reps) {
                for (int i = 0; i < reps; i++) {
                    value[0]++;
                }
            }
            @Override int timeGet(int reps) {
                int sum = 0;
                for (int i = 0; i < reps; i++) {
                    sum += value[0];
                }
                return sum;
            }
        },
        ATOMIC() {
            AtomicInteger value = new AtomicInteger();

            @Override void timeCreate(int reps) {
                for (int i = 0; i < reps; i++) {
                    value = new AtomicInteger(5);
                }
            }
            @Override void timeIncrement(int reps) {
                for (int i = 0; i < reps; i++) {
                    value.incrementAndGet();
                }
            }
            @Override int timeGet(int reps) {
                int sum = 0;
                for (int i = 0; i < reps; i++) {
                    sum += value.intValue();
                }
                return sum;
            }
        };

        abstract void timeCreate(int reps);
        abstract void timeIncrement(int reps);
        abstract int timeGet(int reps);
    }

    @Param Kind kind;

    public void timeCreate(int reps) {
        kind.timeCreate(reps);
    }

    public void timeIncrement(int reps) {
        kind.timeIncrement(reps);
    }

    public void timeGet(int reps) {
        kind.timeGet(reps);
    }
}
