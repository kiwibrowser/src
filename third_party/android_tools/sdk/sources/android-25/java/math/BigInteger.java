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

import java.io.IOException;
import java.io.ObjectInputStream;
import java.io.ObjectOutputStream;
import java.io.Serializable;
import java.util.Random;

/**
 * An immutable arbitrary-precision signed integer.
 *
 * <h3>Fast Cryptography</h3>
 * This implementation is efficient for operations traditionally used in
 * cryptography, such as the generation of large prime numbers and computation
 * of the modular inverse.
 *
 * <h3>Slow Two's Complement Bitwise Operations</h3>
 * This API includes operations for bitwise operations in two's complement
 * representation. Two's complement is not the internal representation used by
 * this implementation, so such methods may be inefficient. Use {@link
 * java.util.BitSet} for high-performance bitwise operations on
 * arbitrarily-large sequences of bits.
 */
public class BigInteger extends Number
        implements Comparable<BigInteger>, Serializable {

    /** This is the serialVersionUID used by the sun implementation. */
    private static final long serialVersionUID = -8287574255936472291L;

    private transient BigInt bigInt;

    private transient boolean nativeIsValid = false;

    private transient boolean javaIsValid = false;

    /** The magnitude of this in the little-endian representation. */
    transient int[] digits;

    /**
     * The length of this in measured in ints. Can be less than
     * digits.length().
     */
    transient int numberLength;

    /** The sign of this. */
    transient int sign;

    /** The {@code BigInteger} constant 0. */
    public static final BigInteger ZERO = new BigInteger(0, 0);

    /** The {@code BigInteger} constant 1. */
    public static final BigInteger ONE = new BigInteger(1, 1);

    /** The {@code BigInteger} constant 10. */
    public static final BigInteger TEN = new BigInteger(1, 10);

    /** The {@code BigInteger} constant -1. */
    static final BigInteger MINUS_ONE = new BigInteger(-1, 1);

    /** All the {@code BigInteger} numbers in the range [0,10] are cached. */
    static final BigInteger[] SMALL_VALUES = { ZERO, ONE, new BigInteger(1, 2),
            new BigInteger(1, 3), new BigInteger(1, 4), new BigInteger(1, 5),
            new BigInteger(1, 6), new BigInteger(1, 7), new BigInteger(1, 8),
            new BigInteger(1, 9), TEN };

    private transient int firstNonzeroDigit = -2;

    /** sign field, used for serialization. */
    private int signum;

    /** absolute value field, used for serialization */
    private byte[] magnitude;

    /** Cache for the hash code. */
    private transient int hashCode = 0;

    BigInteger(BigInt bigInt) {
        if (bigInt == null || bigInt.getNativeBIGNUM() == 0) {
            throw new AssertionError();
        }
        setBigInt(bigInt);
    }

    BigInteger(int sign, long value) {
        BigInt bigInt = new BigInt();
        bigInt.putULongInt(value, (sign < 0));
        setBigInt(bigInt);
    }

    /**
     * Constructs a number without creating new space. This construct should be
     * used only if the three fields of representation are known.
     *
     * @param sign the sign of the number.
     * @param numberLength the length of the internal array.
     * @param digits a reference of some array created before.
     */
    BigInteger(int sign, int numberLength, int[] digits) {
        setJavaRepresentation(sign, numberLength, digits);
    }

    /**
     * Constructs a random non-negative {@code BigInteger} instance in the range
     * {@code [0, pow(2, numBits)-1]}.
     *
     * @param numBits maximum length of the new {@code BigInteger} in bits.
     * @param random is the random number generator to be used.
     * @throws IllegalArgumentException if {@code numBits} < 0.
     */
    public BigInteger(int numBits, Random random) {
        if (numBits < 0) {
            throw new IllegalArgumentException("numBits < 0: " + numBits);
        }
        if (numBits == 0) {
            setJavaRepresentation(0, 1, new int[] { 0 });
        } else {
            int sign = 1;
            int numberLength = (numBits + 31) >> 5;
            int[] digits = new int[numberLength];
            for (int i = 0; i < numberLength; i++) {
                digits[i] = random.nextInt();
            }
            // Clear any extra bits.
            digits[numberLength - 1] >>>= (-numBits) & 31;
            setJavaRepresentation(sign, numberLength, digits);
        }
        javaIsValid = true;
    }

    /**
     * Constructs a random {@code BigInteger} instance in the range {@code [0,
     * pow(2, bitLength)-1]} which is probably prime. The probability that the
     * returned {@code BigInteger} is prime is greater than
     * {@code 1 - 1/2<sup>certainty</sup>)}.
     *
     * <p><b>Note:</b> the {@code Random} argument is ignored if
     * {@code bitLength >= 16}, where this implementation will use OpenSSL's
     * {@code BN_generate_prime_ex} as a source of cryptographically strong pseudo-random numbers.
     *
     * @param bitLength length of the new {@code BigInteger} in bits.
     * @param certainty tolerated primality uncertainty.
     * @throws ArithmeticException if {@code bitLength < 2}.
     * @see <a href="http://www.openssl.org/docs/crypto/BN_rand.html">
     *      Specification of random generator used from OpenSSL library</a>
     */
    public BigInteger(int bitLength, int certainty, Random random) {
        if (bitLength < 2) {
            throw new ArithmeticException("bitLength < 2: " + bitLength);
        }
        if (bitLength < 16) {
            // We have to generate short primes ourselves, because OpenSSL bottoms out at 16 bits.
            int candidate;
            do {
                candidate = random.nextInt() & ((1 << bitLength) - 1);
                candidate |= (1 << (bitLength - 1)); // Set top bit.
                if (bitLength > 2) {
                    candidate |= 1; // Any prime longer than 2 bits must have the bottom bit set.
                }
            } while (!isSmallPrime(candidate));
            BigInt prime = new BigInt();
            prime.putULongInt(candidate, false);
            setBigInt(prime);
        } else {
            // We need a loop here to work around an OpenSSL bug; http://b/8588028.
            do {
                setBigInt(BigInt.generatePrimeDefault(bitLength));
            } while (bitLength() != bitLength);
        }
    }

    private static boolean isSmallPrime(int x) {
        if (x == 2) {
            return true;
        }
        if ((x % 2) == 0) {
            return false;
        }
        final int max = (int) Math.sqrt(x);
        for (int i = 3; i <= max; i += 2) {
            if ((x % i) == 0) {
                return false;
            }
        }
        return true;
    }

    /**
     * Constructs a new {@code BigInteger} by parsing {@code value}. The string
     * representation consists of an optional plus or minus sign followed by a
     * non-empty sequence of decimal digits. Digits are interpreted as if by
     * {@code Character.digit(char,10)}.
     *
     * @param value string representation of the new {@code BigInteger}.
     * @throws NullPointerException if {@code value == null}.
     * @throws NumberFormatException if {@code value} is not a valid
     *     representation of a {@code BigInteger}.
     */
    public BigInteger(String value) {
        BigInt bigInt = new BigInt();
        bigInt.putDecString(value);
        setBigInt(bigInt);
    }

    /**
     * Constructs a new {@code BigInteger} instance by parsing {@code value}.
     * The string representation consists of an optional plus or minus sign
     * followed by a non-empty sequence of digits in the specified radix. Digits
     * are interpreted as if by {@code Character.digit(char, radix)}.
     *
     * @param value string representation of the new {@code BigInteger}.
     * @param radix the base to be used for the conversion.
     * @throws NullPointerException if {@code value == null}.
     * @throws NumberFormatException if {@code value} is not a valid
     *     representation of a {@code BigInteger} or if {@code radix <
     *     Character.MIN_RADIX} or {@code radix > Character.MAX_RADIX}.
     */
    public BigInteger(String value, int radix) {
        if (value == null) {
            throw new NullPointerException("value == null");
        }
        if (radix == 10) {
            BigInt bigInt = new BigInt();
            bigInt.putDecString(value);
            setBigInt(bigInt);
        } else if (radix == 16) {
            BigInt bigInt = new BigInt();
            bigInt.putHexString(value);
            setBigInt(bigInt);
        } else {
            if (radix < Character.MIN_RADIX || radix > Character.MAX_RADIX) {
                throw new NumberFormatException("Invalid radix: " + radix);
            }
            if (value.isEmpty()) {
                throw new NumberFormatException("value.isEmpty()");
            }
            BigInteger.parseFromString(this, value, radix);
        }
    }

    /**
     * Constructs a new {@code BigInteger} instance with the given sign and
     * magnitude.
     *
     * @param signum sign of the new {@code BigInteger} (-1 for negative, 0 for
     *     zero, 1 for positive).
     * @param magnitude magnitude of the new {@code BigInteger} with the most
     *     significant byte first.
     * @throws NullPointerException if {@code magnitude == null}.
     * @throws NumberFormatException if the sign is not one of -1, 0, 1 or if
     *     the sign is zero and the magnitude contains non-zero entries.
     */
    public BigInteger(int signum, byte[] magnitude) {
        if (magnitude == null) {
            throw new NullPointerException("magnitude == null");
        }
        if (signum < -1 || signum > 1) {
            throw new NumberFormatException("Invalid signum: " + signum);
        }
        if (signum == 0) {
            for (byte element : magnitude) {
                if (element != 0) {
                    throw new NumberFormatException("signum-magnitude mismatch");
                }
            }
        }
        BigInt bigInt = new BigInt();
        bigInt.putBigEndian(magnitude, signum < 0);
        setBigInt(bigInt);
    }

    /**
     * Constructs a new {@code BigInteger} from the given two's complement
     * representation. The most significant byte is the entry at index 0. The
     * most significant bit of this entry determines the sign of the new {@code
     * BigInteger} instance. The array must be nonempty.
     *
     * @param value two's complement representation of the new {@code
     *     BigInteger}.
     * @throws NullPointerException if {@code value == null}.
     * @throws NumberFormatException if the length of {@code value} is zero.
     */
    public BigInteger(byte[] value) {
        if (value.length == 0) {
            throw new NumberFormatException("value.length == 0");
        }
        BigInt bigInt = new BigInt();
        bigInt.putBigEndianTwosComplement(value);
        setBigInt(bigInt);
    }

    /**
     * Returns the internal native representation of this big integer, computing
     * it if necessary.
     */
    BigInt getBigInt() {
        if (nativeIsValid) {
            return bigInt;
        }

        synchronized (this) {
            if (nativeIsValid) {
                return bigInt;
            }
            BigInt bigInt = new BigInt();
            bigInt.putLittleEndianInts(digits, (sign < 0));
            setBigInt(bigInt);
            return bigInt;
        }
    }

    private void setBigInt(BigInt bigInt) {
        this.bigInt = bigInt;
        this.nativeIsValid = true;
    }

    private void setJavaRepresentation(int sign, int numberLength, int[] digits) {
        // decrement numberLength to drop leading zeroes...
        while (numberLength > 0 && digits[--numberLength] == 0) {
            ;
        }
        // ... and then increment it back because we always drop one too many
        if (digits[numberLength++] == 0) {
            sign = 0;
        }
        this.sign = sign;
        this.digits = digits;
        this.numberLength = numberLength;
        this.javaIsValid = true;
    }

    void prepareJavaRepresentation() {
        if (javaIsValid) {
            return;
        }

        synchronized (this) {
            if (javaIsValid) {
                return;
            }
            int sign = bigInt.sign();
            int[] digits = (sign != 0) ? bigInt.littleEndianIntsMagnitude() : new int[] { 0 };
            setJavaRepresentation(sign, digits.length, digits);
        }
    }

    /** Returns a {@code BigInteger} whose value is equal to {@code value}. */
    public static BigInteger valueOf(long value) {
        if (value < 0) {
            if (value != -1) {
                return new BigInteger(-1, -value);
            }
            return MINUS_ONE;
        } else if (value < SMALL_VALUES.length) {
            return SMALL_VALUES[(int) value];
        } else {// (value > 10)
            return new BigInteger(1, value);
        }
    }

    /**
     * Returns the two's complement representation of this {@code BigInteger} in
     * a byte array.
     */
    public byte[] toByteArray() {
        return twosComplement();
    }

    /**
     * Returns a {@code BigInteger} whose value is the absolute value of {@code
     * this}.
     */
    public BigInteger abs() {
        BigInt bigInt = getBigInt();
        if (bigInt.sign() >= 0) {
            return this;
        }
        BigInt a = bigInt.copy();
        a.setSign(1);
        return new BigInteger(a);
    }

    /**
     * Returns a {@code BigInteger} whose value is the {@code -this}.
     */
    public BigInteger negate() {
        BigInt bigInt = getBigInt();
        int sign = bigInt.sign();
        if (sign == 0) {
            return this;
        }
        BigInt a = bigInt.copy();
        a.setSign(-sign);
        return new BigInteger(a);
    }

    /**
     * Returns a {@code BigInteger} whose value is {@code this + value}.
     */
    public BigInteger add(BigInteger value) {
        BigInt lhs = getBigInt();
        BigInt rhs = value.getBigInt();
        if (rhs.sign() == 0) {
            return this;
        }
        if (lhs.sign() == 0) {
            return value;
        }
        return new BigInteger(BigInt.addition(lhs, rhs));
    }

    /**
     * Returns a {@code BigInteger} whose value is {@code this - value}.
     */
    public BigInteger subtract(BigInteger value) {
        BigInt lhs = getBigInt();
        BigInt rhs = value.getBigInt();
        if (rhs.sign() == 0) {
            return this;
        }
        return new BigInteger(BigInt.subtraction(lhs, rhs));
    }

    /**
     * Returns the sign of this {@code BigInteger}.
     *
     * @return {@code -1} if {@code this < 0}, {@code 0} if {@code this == 0},
     *     {@code 1} if {@code this > 0}.
     */
    public int signum() {
        if (javaIsValid) {
            return sign;
        }
        return getBigInt().sign();
    }

    /**
     * Returns a {@code BigInteger} whose value is {@code this >> n}. For
     * negative arguments, the result is also negative. The shift distance may
     * be negative which means that {@code this} is shifted left.
     *
     * <p><b>Implementation Note:</b> Usage of this method on negative values is
     * not recommended as the current implementation is not efficient.
     *
     * @param n shift distance
     * @return {@code this >> n} if {@code n >= 0}; {@code this << (-n)}
     *     otherwise
     */
    public BigInteger shiftRight(int n) {
        return shiftLeft(-n);
    }

    /**
     * Returns a {@code BigInteger} whose value is {@code this << n}. The
     * result is equivalent to {@code this * pow(2, n)} if n >= 0. The shift
     * distance may be negative which means that {@code this} is shifted right.
     * The result then corresponds to {@code floor(this / pow(2, -n))}.
     *
     * <p><b>Implementation Note:</b> Usage of this method on negative values is
     * not recommended as the current implementation is not efficient.
     *
     * @param n shift distance.
     * @return {@code this << n} if {@code n >= 0}; {@code this >> (-n)}.
     *     otherwise
     */
    public BigInteger shiftLeft(int n) {
        if (n == 0) {
            return this;
        }
        int sign = signum();
        if (sign == 0) {
            return this;
        }
        if ((sign > 0) || (n >= 0)) {
            return new BigInteger(BigInt.shift(getBigInt(), n));
        } else {
            // Negative numbers faking 2's complement:
            // Not worth optimizing this:
            // Sticking to Harmony Java implementation.
            return BitLevel.shiftRight(this, -n);
        }
    }

    BigInteger shiftLeftOneBit() {
        return (signum() == 0) ? this : BitLevel.shiftLeftOneBit(this);
    }

    /**
     * Returns the length of the value's two's complement representation without
     * leading zeros for positive numbers / without leading ones for negative
     * values.
     *
     * <p>The two's complement representation of {@code this} will be at least
     * {@code bitLength() + 1} bits long.
     *
     * <p>The value will fit into an {@code int} if {@code bitLength() < 32} or
     * into a {@code long} if {@code bitLength() < 64}.
     *
     * @return the length of the minimal two's complement representation for
     *     {@code this} without the sign bit.
     */
    public int bitLength() {
        // Optimization to avoid unnecessary duplicate representation:
        if (!nativeIsValid && javaIsValid) {
            return BitLevel.bitLength(this);
        }
        return getBigInt().bitLength();
    }

    /**
     * Tests whether the bit at position n in {@code this} is set. The result is
     * equivalent to {@code this & pow(2, n) != 0}.
     *
     * <p><b>Implementation Note:</b> Usage of this method is not recommended as
     * the current implementation is not efficient.
     *
     * @param n position where the bit in {@code this} has to be inspected.
     * @throws ArithmeticException if {@code n < 0}.
     */
    public boolean testBit(int n) {
        if (n < 0) {
            throw new ArithmeticException("n < 0: " + n);
        }
        int sign = signum();
        if (sign > 0 && nativeIsValid && !javaIsValid) {
            return getBigInt().isBitSet(n);
        } else {
            // Negative numbers faking 2's complement:
            // Not worth optimizing this:
            // Sticking to Harmony Java implementation.
            prepareJavaRepresentation();
            if (n == 0) {
                return ((digits[0] & 1) != 0);
            }
            int intCount = n >> 5;
            if (intCount >= numberLength) {
                return (sign < 0);
            }
            int digit = digits[intCount];
            n = (1 << (n & 31)); // int with 1 set to the needed position
            if (sign < 0) {
                int firstNonZeroDigit = getFirstNonzeroDigit();
                if (intCount < firstNonZeroDigit) {
                    return false;
                } else if (firstNonZeroDigit == intCount) {
                    digit = -digit;
                } else {
                    digit = ~digit;
                }
            }
            return ((digit & n) != 0);
        }
    }

    /**
     * Returns a {@code BigInteger} which has the same binary representation
     * as {@code this} but with the bit at position n set. The result is
     * equivalent to {@code this | pow(2, n)}.
     *
     * <p><b>Implementation Note:</b> Usage of this method is not recommended as
     * the current implementation is not efficient.
     *
     * @param n position where the bit in {@code this} has to be set.
     * @throws ArithmeticException if {@code n < 0}.
     */
    public BigInteger setBit(int n) {
        prepareJavaRepresentation();
        if (!testBit(n)) {
            return BitLevel.flipBit(this, n);
        } else {
            return this;
        }
    }

    /**
     * Returns a {@code BigInteger} which has the same binary representation
     * as {@code this} but with the bit at position n cleared. The result is
     * equivalent to {@code this & ~pow(2, n)}.
     *
     * <p><b>Implementation Note:</b> Usage of this method is not recommended as
     * the current implementation is not efficient.
     *
     * @param n position where the bit in {@code this} has to be cleared.
     * @throws ArithmeticException if {@code n < 0}.
     */
    public BigInteger clearBit(int n) {
        prepareJavaRepresentation();
        if (testBit(n)) {
            return BitLevel.flipBit(this, n);
        } else {
            return this;
        }
    }

    /**
     * Returns a {@code BigInteger} which has the same binary representation
     * as {@code this} but with the bit at position n flipped. The result is
     * equivalent to {@code this ^ pow(2, n)}.
     *
     * <p><b>Implementation Note:</b> Usage of this method is not recommended as
     * the current implementation is not efficient.
     *
     * @param n position where the bit in {@code this} has to be flipped.
     * @throws ArithmeticException if {@code n < 0}.
     */
    public BigInteger flipBit(int n) {
        prepareJavaRepresentation();
        if (n < 0) {
            throw new ArithmeticException("n < 0: " + n);
        }
        return BitLevel.flipBit(this, n);
    }

    /**
     * Returns the position of the lowest set bit in the two's complement
     * representation of this {@code BigInteger}. If all bits are zero (this==0)
     * then -1 is returned as result.
     *
     * <p><b>Implementation Note:</b> Usage of this method is not recommended as
     * the current implementation is not efficient.
     */
    public int getLowestSetBit() {
        prepareJavaRepresentation();
        if (sign == 0) {
            return -1;
        }
        // (sign != 0) implies that exists some non zero digit
        int i = getFirstNonzeroDigit();
        return ((i << 5) + Integer.numberOfTrailingZeros(digits[i]));
    }

    /**
     * Returns the number of bits in the two's complement representation of
     * {@code this} which differ from the sign bit. If {@code this} is negative,
     * the result is equivalent to the number of bits set in the two's
     * complement representation of {@code -this - 1}.
     *
     * <p>Use {@code bitLength(0)} to find the length of the binary value in
     * bits.
     *
     * <p><b>Implementation Note:</b> Usage of this method is not recommended as
     * the current implementation is not efficient.
     */
    public int bitCount() {
        prepareJavaRepresentation();
        return BitLevel.bitCount(this);
    }

    /**
     * Returns a {@code BigInteger} whose value is {@code ~this}. The result
     * of this operation is {@code -this-1}.
     *
     * <p><b>Implementation Note:</b> Usage of this method is not recommended as
     * the current implementation is not efficient.
     */
    public BigInteger not() {
        this.prepareJavaRepresentation();
        return Logical.not(this);
    }

    /**
     * Returns a {@code BigInteger} whose value is {@code this & value}.
     *
     * <p><b>Implementation Note:</b> Usage of this method is not recommended
     * as the current implementation is not efficient.
     *
     * @param value value to be and'ed with {@code this}.
     * @throws NullPointerException if {@code value == null}.
     */
    public BigInteger and(BigInteger value) {
        this.prepareJavaRepresentation();
        value.prepareJavaRepresentation();
        return Logical.and(this, value);
    }

    /**
     * Returns a {@code BigInteger} whose value is {@code this | value}.
     *
     * <p><b>Implementation Note:</b> Usage of this method is not recommended as
     * the current implementation is not efficient.
     *
     * @param value value to be or'ed with {@code this}.
     * @throws NullPointerException if {@code value == null}.
     */
    public BigInteger or(BigInteger value) {
        this.prepareJavaRepresentation();
        value.prepareJavaRepresentation();
        return Logical.or(this, value);
    }

    /**
     * Returns a {@code BigInteger} whose value is {@code this ^ value}.
     *
     * <p><b>Implementation Note:</b> Usage of this method is not recommended as
     * the current implementation is not efficient.
     *
     * @param value value to be xor'ed with {@code this}
     * @throws NullPointerException if {@code value == null}
     */
    public BigInteger xor(BigInteger value) {
        this.prepareJavaRepresentation();
        value.prepareJavaRepresentation();
        return Logical.xor(this, value);
    }

    /**
     * Returns a {@code BigInteger} whose value is {@code this & ~value}.
     * Evaluating {@code x.andNot(value)} returns the same result as {@code
     * x.and(value.not())}.
     *
     * <p><b>Implementation Note:</b> Usage of this method is not recommended
     * as the current implementation is not efficient.
     *
     * @param value value to be not'ed and then and'ed with {@code this}.
     * @throws NullPointerException if {@code value == null}.
     */
    public BigInteger andNot(BigInteger value) {
        this.prepareJavaRepresentation();
        value.prepareJavaRepresentation();
        return Logical.andNot(this, value);
    }

    /**
     * Returns this {@code BigInteger} as an int value. If {@code this} is too
     * big to be represented as an int, then {@code this % (1 << 32)} is
     * returned.
     */
    @Override
    public int intValue() {
        if (nativeIsValid && bigInt.twosCompFitsIntoBytes(4)) {
            return (int) bigInt.longInt();
        }
        this.prepareJavaRepresentation();
        return (sign * digits[0]);
    }

    /**
     * Returns this {@code BigInteger} as a long value. If {@code this} is too
     * big to be represented as a long, then {@code this % pow(2, 64)} is
     * returned.
     */
    @Override
    public long longValue() {
        if (nativeIsValid && bigInt.twosCompFitsIntoBytes(8)) {
            return bigInt.longInt();
        }
        prepareJavaRepresentation();
        long value = numberLength > 1
                ? ((long) digits[1]) << 32 | digits[0] & 0xFFFFFFFFL
                : digits[0] & 0xFFFFFFFFL;
        return sign * value;
    }

    /**
     * Returns this {@code BigInteger} as a float. If {@code this} is too big to
     * be represented as a float, then {@code Float.POSITIVE_INFINITY} or
     * {@code Float.NEGATIVE_INFINITY} is returned. Note that not all integers
     * in the range {@code [-Float.MAX_VALUE, Float.MAX_VALUE]} can be exactly
     * represented as a float.
     */
    @Override
    public float floatValue() {
        return (float) doubleValue();
    }

    /**
     * Returns this {@code BigInteger} as a double. If {@code this} is too big
     * to be represented as a double, then {@code Double.POSITIVE_INFINITY} or
     * {@code Double.NEGATIVE_INFINITY} is returned. Note that not all integers
     * in the range {@code [-Double.MAX_VALUE, Double.MAX_VALUE]} can be exactly
     * represented as a double.
     */
    @Override
    public double doubleValue() {
        return Conversion.bigInteger2Double(this);
    }

    /**
     * Compares this {@code BigInteger} with {@code value}. Returns {@code -1}
     * if {@code this < value}, {@code 0} if {@code this == value} and {@code 1}
     * if {@code this > value}, .
     *
     * @param value value to be compared with {@code this}.
     * @throws NullPointerException if {@code value == null}.
     */
    public int compareTo(BigInteger value) {
        return BigInt.cmp(getBigInt(), value.getBigInt());
    }

    /**
     * Returns the minimum of this {@code BigInteger} and {@code value}.
     *
     * @param value value to be used to compute the minimum with {@code this}.
     * @throws NullPointerException if {@code value == null}.
     */
    public BigInteger min(BigInteger value) {
        return this.compareTo(value) == -1 ? this : value;
    }

    /**
     * Returns the maximum of this {@code BigInteger} and {@code value}.
     *
     * @param value value to be used to compute the maximum with {@code this}
     * @throws NullPointerException if {@code value == null}
     */
    public BigInteger max(BigInteger value) {
        return this.compareTo(value) == 1 ? this : value;
    }

    @Override
    public int hashCode() {
        if (hashCode == 0) {
            prepareJavaRepresentation();
            int hash = 0;
            for (int i = 0; i < numberLength; ++i) {
                hash = hash * 33 + digits[i];
            }
            hashCode = hash * sign;
        }
        return hashCode;
    }

    @Override
    public boolean equals(Object x) {
        if (this == x) {
            return true;
        }
        if (x instanceof BigInteger) {
            return this.compareTo((BigInteger) x) == 0;
        }
        return false;
    }

    /**
     * Returns a string representation of this {@code BigInteger} in decimal
     * form.
     */
    @Override
    public String toString() {
        return getBigInt().decString();
    }

    /**
     * Returns a string containing a string representation of this {@code
     * BigInteger} with base radix. If {@code radix < Character.MIN_RADIX} or
     * {@code radix > Character.MAX_RADIX} then a decimal representation is
     * returned. The characters of the string representation are generated with
     * method {@code Character.forDigit}.
     *
     * @param radix base to be used for the string representation.
     */
    public String toString(int radix) {
        if (radix == 10) {
            return getBigInt().decString();
        } else {
            prepareJavaRepresentation();
            return Conversion.bigInteger2String(this, radix);
        }
    }

    /**
     * Returns a {@code BigInteger} whose value is greatest common divisor
     * of {@code this} and {@code value}. If {@code this == 0} and {@code
     * value == 0} then zero is returned, otherwise the result is positive.
     *
     * @param value value with which the greatest common divisor is computed.
     * @throws NullPointerException if {@code value == null}.
     */
    public BigInteger gcd(BigInteger value) {
        return new BigInteger(BigInt.gcd(getBigInt(), value.getBigInt()));
    }

    /**
     * Returns a {@code BigInteger} whose value is {@code this * value}.
     *
     * @throws NullPointerException if {@code value == null}.
     */
    public BigInteger multiply(BigInteger value) {
        return new BigInteger(BigInt.product(getBigInt(), value.getBigInt()));
    }

    /**
     * Returns a {@code BigInteger} whose value is {@code pow(this, exp)}.
     *
     * @throws ArithmeticException if {@code exp < 0}.
     */
    public BigInteger pow(int exp) {
        if (exp < 0) {
            throw new ArithmeticException("exp < 0: " + exp);
        }
        return new BigInteger(BigInt.exp(getBigInt(), exp));
    }

    /**
     * Returns a two element {@code BigInteger} array containing
     * {@code this / divisor} at index 0 and {@code this % divisor} at index 1.
     *
     * @param divisor value by which {@code this} is divided.
     * @throws NullPointerException if {@code divisor == null}.
     * @throws ArithmeticException if {@code divisor == 0}.
     * @see #divide
     * @see #remainder
     */
    public BigInteger[] divideAndRemainder(BigInteger divisor) {
        BigInt divisorBigInt = divisor.getBigInt();
        BigInt quotient = new BigInt();
        BigInt remainder = new BigInt();
        BigInt.division(getBigInt(), divisorBigInt, quotient, remainder);
        return new BigInteger[] {new BigInteger(quotient), new BigInteger(remainder) };
    }

    /**
     * Returns a {@code BigInteger} whose value is {@code this / divisor}.
     *
     * @param divisor value by which {@code this} is divided.
     * @return {@code this / divisor}.
     * @throws NullPointerException if {@code divisor == null}.
     * @throws ArithmeticException if {@code divisor == 0}.
     */
    public BigInteger divide(BigInteger divisor) {
        BigInt quotient = new BigInt();
        BigInt.division(getBigInt(), divisor.getBigInt(), quotient, null);
        return new BigInteger(quotient);
    }

    /**
     * Returns a {@code BigInteger} whose value is {@code this % divisor}.
     * Regarding signs this methods has the same behavior as the % operator on
     * ints: the sign of the remainder is the same as the sign of this.
     *
     * @param divisor value by which {@code this} is divided.
     * @throws NullPointerException if {@code divisor == null}.
     * @throws ArithmeticException if {@code divisor == 0}.
     */
    public BigInteger remainder(BigInteger divisor) {
        BigInt remainder = new BigInt();
        BigInt.division(getBigInt(), divisor.getBigInt(), null, remainder);
        return new BigInteger(remainder);
    }

    /**
     * Returns a {@code BigInteger} whose value is {@code 1/this mod m}. The
     * modulus {@code m} must be positive. The result is guaranteed to be in the
     * interval {@code [0, m)} (0 inclusive, m exclusive). If {@code this} is
     * not relatively prime to m, then an exception is thrown.
     *
     * @param m the modulus.
     * @throws NullPointerException if {@code m == null}
     * @throws ArithmeticException if {@code m < 0 or} if {@code this} is not
     *     relatively prime to {@code m}
     */
    public BigInteger modInverse(BigInteger m) {
        if (m.signum() <= 0) {
            throw new ArithmeticException("modulus not positive");
        }
        return new BigInteger(BigInt.modInverse(getBigInt(), m.getBigInt()));
    }

    /**
     * Returns a {@code BigInteger} whose value is {@code
     * pow(this, exponent) mod modulus}. The modulus must be positive. The
     * result is guaranteed to be in the interval {@code [0, modulus)}.
     * If the exponent is negative, then
     * {@code pow(this.modInverse(modulus), -exponent) mod modulus} is computed.
     * The inverse of this only exists if {@code this} is relatively prime to the modulus,
     * otherwise an exception is thrown.
     *
     * @throws NullPointerException if {@code modulus == null} or {@code exponent == null}.
     * @throws ArithmeticException if {@code modulus < 0} or if {@code exponent < 0} and
     *     not relatively prime to {@code modulus}.
     */
    public BigInteger modPow(BigInteger exponent, BigInteger modulus) {
        if (modulus.signum() <= 0) {
            throw new ArithmeticException("modulus.signum() <= 0");
        }
        int exponentSignum = exponent.signum();
        if (exponentSignum == 0) { // OpenSSL gets this case wrong; http://b/8574367.
            return ONE.mod(modulus);
        }
        BigInteger base = exponentSignum < 0 ? modInverse(modulus) : this;
        return new BigInteger(BigInt.modExp(base.getBigInt(), exponent.getBigInt(), modulus.getBigInt()));
    }

    /**
     * Returns a {@code BigInteger} whose value is {@code this mod m}. The
     * modulus {@code m} must be positive. The result is guaranteed to be in the
     * interval {@code [0, m)} (0 inclusive, m exclusive). The behavior of this
     * function is not equivalent to the behavior of the % operator defined for
     * the built-in {@code int}'s.
     *
     * @param m the modulus.
     * @return {@code this mod m}.
     * @throws NullPointerException if {@code m == null}.
     * @throws ArithmeticException if {@code m < 0}.
     */
    public BigInteger mod(BigInteger m) {
        if (m.signum() <= 0) {
            throw new ArithmeticException("m.signum() <= 0");
        }
        return new BigInteger(BigInt.modulus(getBigInt(), m.getBigInt()));
    }

    /**
     * Tests whether this {@code BigInteger} is probably prime. If {@code true}
     * is returned, then this is prime with a probability greater than
     * {@code 1 - 1/2<sup>certainty</sup>)}. If {@code false} is returned, then this
     * is definitely composite. If the argument {@code certainty} <= 0, then
     * this method returns true.
     *
     * @param certainty tolerated primality uncertainty.
     * @return {@code true}, if {@code this} is probably prime, {@code false}
     *     otherwise.
     */
    public boolean isProbablePrime(int certainty) {
        if (certainty <= 0) {
            return true;
        }
        return getBigInt().isPrime(certainty);
    }

    /**
     * Returns the smallest integer x > {@code this} which is probably prime as
     * a {@code BigInteger} instance. The probability that the returned {@code
     * BigInteger} is prime is greater than {@code 1 - 1/2<sup>100</sup>}.
     *
     * @return smallest integer > {@code this} which is probably prime.
     * @throws ArithmeticException if {@code this < 0}.
     */
    public BigInteger nextProbablePrime() {
        if (sign < 0) {
            throw new ArithmeticException("sign < 0");
        }
        return Primality.nextProbablePrime(this);
    }

    /**
     * Returns a random positive {@code BigInteger} instance in the range {@code
     * [0, pow(2, bitLength)-1]} which is probably prime. The probability that
     * the returned {@code BigInteger} is prime is greater than {@code 1 - 1/2<sup>100</sup>)}.
     *
     * @param bitLength length of the new {@code BigInteger} in bits.
     * @return probably prime random {@code BigInteger} instance.
     * @throws IllegalArgumentException if {@code bitLength < 2}.
     */
    public static BigInteger probablePrime(int bitLength, Random random) {
        return new BigInteger(bitLength, 100, random);
    }

    /* Private Methods */

    /**
     * Returns the two's complement representation of this BigInteger in a byte
     * array.
     */
    private byte[] twosComplement() {
        prepareJavaRepresentation();
        if (this.sign == 0) {
            return new byte[] { 0 };
        }
        BigInteger temp = this;
        int bitLen = bitLength();
        int iThis = getFirstNonzeroDigit();
        int bytesLen = (bitLen >> 3) + 1;
        /* Puts the little-endian int array representing the magnitude
         * of this BigInteger into the big-endian byte array. */
        byte[] bytes = new byte[bytesLen];
        int firstByteNumber = 0;
        int highBytes;
        int bytesInInteger = 4;
        int hB;

        if (bytesLen - (numberLength << 2) == 1) {
            bytes[0] = (byte) ((sign < 0) ? -1 : 0);
            highBytes = 4;
            firstByteNumber++;
        } else {
            hB = bytesLen & 3;
            highBytes = (hB == 0) ? 4 : hB;
        }

        int digitIndex = iThis;
        bytesLen -= iThis << 2;

        if (sign < 0) {
            int digit = -temp.digits[digitIndex];
            digitIndex++;
            if (digitIndex == numberLength) {
                bytesInInteger = highBytes;
            }
            for (int i = 0; i < bytesInInteger; i++, digit >>= 8) {
                bytes[--bytesLen] = (byte) digit;
            }
            while (bytesLen > firstByteNumber) {
                digit = ~temp.digits[digitIndex];
                digitIndex++;
                if (digitIndex == numberLength) {
                    bytesInInteger = highBytes;
                }
                for (int i = 0; i < bytesInInteger; i++, digit >>= 8) {
                    bytes[--bytesLen] = (byte) digit;
                }
            }
        } else {
            while (bytesLen > firstByteNumber) {
                int digit = temp.digits[digitIndex];
                digitIndex++;
                if (digitIndex == numberLength) {
                    bytesInInteger = highBytes;
                }
                for (int i = 0; i < bytesInInteger; i++, digit >>= 8) {
                    bytes[--bytesLen] = (byte) digit;
                }
            }
        }
        return bytes;
    }


    static int multiplyByInt(int[] res, int[] a, int aSize, int factor) {
        long carry = 0;

        for (int i = 0; i < aSize; i++) {
            carry += (a[i] & 0xFFFFFFFFL) * (factor & 0xFFFFFFFFL);
            res[i] = (int) carry;
            carry >>>= 32;
        }
        return (int) carry;
    }

    static int inplaceAdd(int[] a, int aSize, int addend) {
        long carry = addend & 0xFFFFFFFFL;

        for (int i = 0; (carry != 0) && (i < aSize); i++) {
            carry += a[i] & 0xFFFFFFFFL;
            a[i] = (int) carry;
            carry >>= 32;
        }
        return (int) carry;
    }

    /** @see BigInteger#BigInteger(String, int) */
    private static void parseFromString(BigInteger bi, String value, int radix) {
        int stringLength = value.length();
        int endChar = stringLength;

        int sign;
        int startChar;
        if (value.charAt(0) == '-') {
            sign = -1;
            startChar = 1;
            stringLength--;
        } else {
            sign = 1;
            startChar = 0;
        }

        /*
         * We use the following algorithm: split a string into portions of n
         * characters and convert each portion to an integer according to the
         * radix. Then convert an pow(radix, n) based number to binary using the
         * multiplication method. See D. Knuth, The Art of Computer Programming,
         * vol. 2.
         */

        int charsPerInt = Conversion.digitFitInInt[radix];
        int bigRadixDigitsLength = stringLength / charsPerInt;
        int topChars = stringLength % charsPerInt;

        if (topChars != 0) {
            bigRadixDigitsLength++;
        }
        int[] digits = new int[bigRadixDigitsLength];
        // Get the maximal power of radix that fits in int
        int bigRadix = Conversion.bigRadices[radix - 2];
        // Parse an input string and accumulate the BigInteger's magnitude
        int digitIndex = 0; // index of digits array
        int substrEnd = startChar + ((topChars == 0) ? charsPerInt : topChars);

        for (int substrStart = startChar; substrStart < endChar;
                substrStart = substrEnd, substrEnd = substrStart + charsPerInt) {
            int bigRadixDigit = Integer.parseInt(value.substring(substrStart, substrEnd), radix);
            int newDigit = multiplyByInt(digits, digits, digitIndex, bigRadix);
            newDigit += inplaceAdd(digits, digitIndex, bigRadixDigit);
            digits[digitIndex++] = newDigit;
        }
        int numberLength = digitIndex;
        bi.setJavaRepresentation(sign, numberLength, digits);
    }

    int getFirstNonzeroDigit() {
        if (firstNonzeroDigit == -2) {
            int i;
            if (this.sign == 0) {
                i = -1;
            } else {
                for (i = 0; digits[i] == 0; i++) {
                    ;
                }
            }
            firstNonzeroDigit = i;
        }
        return firstNonzeroDigit;
    }

    /**
     * Returns a copy of the current instance to achieve immutability
     */
    BigInteger copy() {
        prepareJavaRepresentation();
        int[] copyDigits = new int[numberLength];
        System.arraycopy(digits, 0, copyDigits, 0, numberLength);
        return new BigInteger(sign, numberLength, copyDigits);
    }

    /**
     * Assigns all transient fields upon deserialization of a {@code BigInteger}
     * instance.
     */
    private void readObject(ObjectInputStream in)
            throws IOException, ClassNotFoundException {
        in.defaultReadObject();
        BigInt bigInt = new BigInt();
        bigInt.putBigEndian(magnitude, signum < 0);
        setBigInt(bigInt);
    }

    /**
     * Prepares this {@code BigInteger} for serialization, i.e. the
     * non-transient fields {@code signum} and {@code magnitude} are assigned.
     */
    private void writeObject(ObjectOutputStream out) throws IOException {
        BigInt bigInt = getBigInt();
        signum = bigInt.sign();
        magnitude = bigInt.bigEndianMagnitude();
        out.defaultWriteObject();
    }
}
