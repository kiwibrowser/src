/*
 *  Licensed to the Apache Software Foundation (ASF) under one or more
 *  contributor license agreements.  See the NOTICE file distributed with
 *  this work for additional information regarding copyright ownership.
 *  The ASF licenses this file to You under the Apache License, Version 2.0
 *  (the "License"); you may not use this file except in compliance with
 *  the License.  You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 */

package java.math;

import java.util.Arrays;

/**
 * Provides primality probabilistic methods.
 */
class Primality {

    /** Just to denote that this class can't be instantiated. */
    private Primality() {}

    /** All prime numbers with bit length lesser than 10 bits. */
    private static final int[] primes = { 2, 3, 5, 7, 11, 13, 17, 19, 23, 29,
            31, 37, 41, 43, 47, 53, 59, 61, 67, 71, 73, 79, 83, 89, 97, 101,
            103, 107, 109, 113, 127, 131, 137, 139, 149, 151, 157, 163, 167,
            173, 179, 181, 191, 193, 197, 199, 211, 223, 227, 229, 233, 239,
            241, 251, 257, 263, 269, 271, 277, 281, 283, 293, 307, 311, 313,
            317, 331, 337, 347, 349, 353, 359, 367, 373, 379, 383, 389, 397,
            401, 409, 419, 421, 431, 433, 439, 443, 449, 457, 461, 463, 467,
            479, 487, 491, 499, 503, 509, 521, 523, 541, 547, 557, 563, 569,
            571, 577, 587, 593, 599, 601, 607, 613, 617, 619, 631, 641, 643,
            647, 653, 659, 661, 673, 677, 683, 691, 701, 709, 719, 727, 733,
            739, 743, 751, 757, 761, 769, 773, 787, 797, 809, 811, 821, 823,
            827, 829, 839, 853, 857, 859, 863, 877, 881, 883, 887, 907, 911,
            919, 929, 937, 941, 947, 953, 967, 971, 977, 983, 991, 997, 1009,
            1013, 1019, 1021 };

    /** All {@code BigInteger} prime numbers with bit length lesser than 10 bits. */
    private static final BigInteger BIprimes[] = new BigInteger[primes.length];

//    /**
//     * It encodes how many iterations of Miller-Rabin test are need to get an
//     * error bound not greater than {@code 2<sup>(-100)</sup>}. For example:
//     * for a {@code 1000}-bit number we need {@code 4} iterations, since
//     * {@code BITS[3] < 1000 <= BITS[4]}.
//     */
//    private static final int[] BITS = { 0, 0, 1854, 1233, 927, 747, 627, 543,
//            480, 431, 393, 361, 335, 314, 295, 279, 265, 253, 242, 232, 223,
//            216, 181, 169, 158, 150, 145, 140, 136, 132, 127, 123, 119, 114,
//            110, 105, 101, 96, 92, 87, 83, 78, 73, 69, 64, 59, 54, 49, 44, 38,
//            32, 26, 1 };
//
//    /**
//     * It encodes how many i-bit primes there are in the table for
//     * {@code i=2,...,10}. For example {@code offsetPrimes[6]} says that from
//     * index {@code 11} exists {@code 7} consecutive {@code 6}-bit prime
//     * numbers in the array.
//     */
//    private static final int[][] offsetPrimes = { null, null, { 0, 2 },
//            { 2, 2 }, { 4, 2 }, { 6, 5 }, { 11, 7 }, { 18, 13 }, { 31, 23 },
//            { 54, 43 }, { 97, 75 } };

    static {// To initialize the dual table of BigInteger primes
        for (int i = 0; i < primes.length; i++) {
            BIprimes[i] = BigInteger.valueOf(primes[i]);
        }
    }

    /**
     * It uses the sieve of Eratosthenes to discard several composite numbers in
     * some appropriate range (at the moment {@code [this, this + 1024]}). After
     * this process it applies the Miller-Rabin test to the numbers that were
     * not discarded in the sieve.
     *
     * @see BigInteger#nextProbablePrime()
     */
    static BigInteger nextProbablePrime(BigInteger n) {
        // PRE: n >= 0
        int i, j;
//        int certainty;
        int gapSize = 1024; // for searching of the next probable prime number
        int[] modules = new int[primes.length];
        boolean isDivisible[] = new boolean[gapSize];
        BigInt ni = n.getBigInt();
        // If n < "last prime of table" searches next prime in the table
        if (ni.bitLength() <= 10) {
            int l = (int)ni.longInt();
            if (l < primes[primes.length - 1]) {
                for (i = 0; l >= primes[i]; i++) {}
                return BIprimes[i];
            }
        }

        BigInt startPoint = ni.copy();
        BigInt probPrime = new BigInt();

        // Fix startPoint to "next odd number":
        startPoint.addPositiveInt(BigInt.remainderByPositiveInt(ni, 2) + 1);

//        // To set the improved certainty of Miller-Rabin
//        j = startPoint.bitLength();
//        for (certainty = 2; j < BITS[certainty]; certainty++) {
//            ;
//        }

        // To calculate modules: N mod p1, N mod p2, ... for first primes.
        for (i = 0; i < primes.length; i++) {
            modules[i] = BigInt.remainderByPositiveInt(startPoint, primes[i]) - gapSize;
        }
        while (true) {
            // At this point, all numbers in the gap are initialized as
            // probably primes
            Arrays.fill(isDivisible, false);
            // To discard multiples of first primes
            for (i = 0; i < primes.length; i++) {
                modules[i] = (modules[i] + gapSize) % primes[i];
                j = (modules[i] == 0) ? 0 : (primes[i] - modules[i]);
                for (; j < gapSize; j += primes[i]) {
                    isDivisible[j] = true;
                }
            }
            // To execute Miller-Rabin for non-divisible numbers by all first
            // primes
            for (j = 0; j < gapSize; j++) {
                if (!isDivisible[j]) {
                    probPrime.putCopy(startPoint);
                    probPrime.addPositiveInt(j);
                    if (probPrime.isPrime(100)) {
                        return new BigInteger(probPrime);
                    }
                }
            }
            startPoint.addPositiveInt(gapSize);
        }
    }

}
