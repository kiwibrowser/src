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

/**
 * Static library that provides all multiplication of {@link BigInteger} methods.
 */
class Multiplication {

    /** Just to denote that this class can't be instantiated. */
    private Multiplication() {}

    // BEGIN android-removed
    // /**
    //  * Break point in digits (number of {@code int} elements)
    //  * between Karatsuba and Pencil and Paper multiply.
    //  */
    // static final int whenUseKaratsuba = 63; // an heuristic value
    // END android-removed

    /**
     * An array with powers of ten that fit in the type {@code int}.
     * ({@code 10^0,10^1,...,10^9})
     */
    static final int[] tenPows = {
        1, 10, 100, 1000, 10000, 100000, 1000000, 10000000, 100000000, 1000000000
    };

    /**
     * An array with powers of five that fit in the type {@code int}.
     * ({@code 5^0,5^1,...,5^13})
     */
    static final int[] fivePows = {
        1, 5, 25, 125, 625, 3125, 15625, 78125, 390625,
        1953125, 9765625, 48828125, 244140625, 1220703125
    };

    /**
     * An array with the first powers of ten in {@code BigInteger} version.
     * ({@code 10^0,10^1,...,10^31})
     */
    static final BigInteger[] bigTenPows = new BigInteger[32];

    /**
     * An array with the first powers of five in {@code BigInteger} version.
     * ({@code 5^0,5^1,...,5^31})
     */
    static final BigInteger bigFivePows[] = new BigInteger[32];



    static {
        int i;
        long fivePow = 1L;

        for (i = 0; i <= 18; i++) {
            bigFivePows[i] = BigInteger.valueOf(fivePow);
            bigTenPows[i] = BigInteger.valueOf(fivePow << i);
            fivePow *= 5;
        }
        for (; i < bigTenPows.length; i++) {
            bigFivePows[i] = bigFivePows[i - 1].multiply(bigFivePows[1]);
            bigTenPows[i] = bigTenPows[i - 1].multiply(BigInteger.TEN);
        }
    }

    // BEGIN android-note: multiply has been removed in favor of using OpenSSL BIGNUM
    // END android-note

    /**
     * Multiplies a number by a positive integer.
     * @param val an arbitrary {@code BigInteger}
     * @param factor a positive {@code int} number
     * @return {@code val * factor}
     */
    static BigInteger multiplyByPositiveInt(BigInteger val, int factor) {
        BigInt bi = val.getBigInt().copy();
        bi.multiplyByPositiveInt(factor);
        return new BigInteger(bi);
    }

    /**
     * Multiplies a number by a power of ten.
     * This method is used in {@code BigDecimal} class.
     * @param val the number to be multiplied
     * @param exp a positive {@code long} exponent
     * @return {@code val * 10<sup>exp</sup>}
     */
    static BigInteger multiplyByTenPow(BigInteger val, long exp) {
        // PRE: exp >= 0
        return ((exp < tenPows.length)
        ? multiplyByPositiveInt(val, tenPows[(int)exp])
        : val.multiply(powerOf10(exp)));
    }

    /**
     * It calculates a power of ten, which exponent could be out of 32-bit range.
     * Note that internally this method will be used in the worst case with
     * an exponent equals to: {@code Integer.MAX_VALUE - Integer.MIN_VALUE}.
     * @param exp the exponent of power of ten, it must be positive.
     * @return a {@code BigInteger} with value {@code 10<sup>exp</sup>}.
     */
    static BigInteger powerOf10(long exp) {
        // PRE: exp >= 0
        int intExp = (int)exp;
        // "SMALL POWERS"
        if (exp < bigTenPows.length) {
            // The largest power that fit in 'long' type
            return bigTenPows[intExp];
        } else if (exp <= 50) {
            // To calculate:    10^exp
            return BigInteger.TEN.pow(intExp);
        }

        BigInteger res = null;
        try {
            // "LARGE POWERS"
            if (exp <= Integer.MAX_VALUE) {
                // To calculate:    5^exp * 2^exp
                res = bigFivePows[1].pow(intExp).shiftLeft(intExp);
            } else {
                /*
                 * "HUGE POWERS"
                 *
                 * This branch probably won't be executed since the power of ten is too
                 * big.
                 */
                // To calculate:    5^exp
                BigInteger powerOfFive = bigFivePows[1].pow(Integer.MAX_VALUE);
                res = powerOfFive;
                long longExp = exp - Integer.MAX_VALUE;

                intExp = (int) (exp % Integer.MAX_VALUE);
                while (longExp > Integer.MAX_VALUE) {
                    res = res.multiply(powerOfFive);
                    longExp -= Integer.MAX_VALUE;
                }
                res = res.multiply(bigFivePows[1].pow(intExp));
                // To calculate:    5^exp << exp
                res = res.shiftLeft(Integer.MAX_VALUE);
                longExp = exp - Integer.MAX_VALUE;
                while (longExp > Integer.MAX_VALUE) {
                    res = res.shiftLeft(Integer.MAX_VALUE);
                    longExp -= Integer.MAX_VALUE;
                }
                res = res.shiftLeft(intExp);
            }
        } catch (OutOfMemoryError error) {
            throw new ArithmeticException(error.getMessage());
        }

        return res;
    }

    /**
     * Multiplies a number by a power of five.
     * This method is used in {@code BigDecimal} class.
     * @param val the number to be multiplied
     * @param exp a positive {@code int} exponent
     * @return {@code val * 5<sup>exp</sup>}
     */
    static BigInteger multiplyByFivePow(BigInteger val, int exp) {
        // PRE: exp >= 0
        if (exp < fivePows.length) {
            return multiplyByPositiveInt(val, fivePows[exp]);
        } else if (exp < bigFivePows.length) {
            return val.multiply(bigFivePows[exp]);
        } else {// Large powers of five
            return val.multiply(bigFivePows[1].pow(exp));
        }
    }
}
