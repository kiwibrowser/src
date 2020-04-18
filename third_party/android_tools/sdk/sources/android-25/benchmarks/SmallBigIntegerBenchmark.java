/*
 * Copyright (C) 2015 Google Inc.
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

package benchmarks;

import java.math.BigInteger;
import java.util.Random;

/**
 * This pretends to measure performance of operations on small BigIntegers.
 * Given our current implementation, this is really a way to measure performance of
 * finalization and JNI.
 * We manually determine the number of iterations so that it should cause total memory
 * allocation on the order of a few hundred megabytes.  Due to BigInteger's reliance on
 * finalization, these may unfortunately all be kept around at once.
 */
public class SmallBigIntegerBenchmark {
    // We allocate about 2 1/3 BigIntegers per iteration.
    // Assuming 100 bytes/BigInteger, this gives us around 500MB total.
    static final int NITERS = 2 * 1000 * 1000;
    static final BigInteger BIG_THREE = BigInteger.valueOf(3);
    static final BigInteger BIG_FOUR = BigInteger.valueOf(4);

    public static void main(String args[]) {
        final Random r = new Random();
        BigInteger x = new BigInteger(20, r);
        final long startNanos = System.nanoTime();
        long intermediateNanos = 0;
        for (int i = 0; i < NITERS; ++i) {
            if (i == NITERS / 100) {
                intermediateNanos = System.nanoTime();
            }
            // We know this converges, but the compiler doesn't.
            if (x.and(BigInteger.ONE).equals(BigInteger.ONE)) {
                x = x.multiply(BIG_THREE).add(BigInteger.ONE);
            } else {
                x = x.shiftRight(1);
            }
        }
        if (x.signum() < 0 || x.compareTo(BIG_FOUR) > 0) {
            throw new AssertionError("Something went horribly wrong.");
        }
        final long finalNanos = System.nanoTime();
        double firstFewTime = ((double) intermediateNanos - (double) startNanos) / (NITERS / 100);
        double restTime = ((double) finalNanos - (double) intermediateNanos) / (99 * NITERS / 100);
        System.out.println("First Few: " + firstFewTime
                + " nanoseconds per iteration (2.33 BigInteger ops/iter)");
        System.out.println("Remainder: " + restTime + " nanoseconds per iteration");
    }
}

