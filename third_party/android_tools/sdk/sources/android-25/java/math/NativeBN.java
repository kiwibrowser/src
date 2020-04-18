/*
 * Copyright (C) 2008 The Android Open Source Project
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

package java.math;

final class NativeBN {

    public static native long BN_new();
    // BIGNUM *BN_new(void);

    public static native void BN_free(long a);
    // void BN_free(BIGNUM *a);

    public static native int BN_cmp(long a, long b);
    // int BN_cmp(const BIGNUM *a, const BIGNUM *b);

    public static native void BN_copy(long to, long from);
    // BIGNUM *BN_copy(BIGNUM *to, const BIGNUM *from);

    public static native void putLongInt(long a, long dw);
    public static native void putULongInt(long a, long dw, boolean neg);

    public static native int BN_dec2bn(long a, String str);
    // int BN_dec2bn(BIGNUM **a, const char *str);

    public static native int BN_hex2bn(long a, String str);
    // int BN_hex2bn(BIGNUM **a, const char *str);

    public static native void BN_bin2bn(byte[] s, int len, boolean neg, long ret);
    // BIGNUM * BN_bin2bn(const unsigned char *s, int len, BIGNUM *ret);
    // BN-Docu: s is taken as unsigned big endian;
    // Additional parameter: neg.

    public static native void litEndInts2bn(int[] ints, int len, boolean neg, long ret);

    public static native void twosComp2bn(byte[] s, int len, long ret);


    public static native long longInt(long a);
    // unsigned long BN_get_word(BIGNUM *a);

    public static native String BN_bn2dec(long a);
    // char * BN_bn2dec(const BIGNUM *a);

    public static native String BN_bn2hex(long a);
    // char * BN_bn2hex(const BIGNUM *a);

    public static native byte[] BN_bn2bin(long a);
    // Returns result byte[] AND NOT length.
    // int BN_bn2bin(const BIGNUM *a, unsigned char *to);

    public static native int[] bn2litEndInts(long a);

    public static native int sign(long a);
    // Returns -1, 0, 1 AND NOT boolean.
    // #define BN_is_negative(a) ((a)->neg != 0)

    public static native void BN_set_negative(long b, int n);
    // void BN_set_negative(BIGNUM *b, int n);

    public static native int bitLength(long a);

    public static native boolean BN_is_bit_set(long a, int n);
    // int BN_is_bit_set(const BIGNUM *a, int n);

    public static native void BN_shift(long r, long a, int n);
    // int BN_shift(BIGNUM *r, const BIGNUM *a, int n);

    public static native void BN_add_word(long a, int w);
    // ATTENTION: w is treated as unsigned.
    // int BN_add_word(BIGNUM *a, BN_ULONG w);

    public static native void BN_mul_word(long a, int w);
    // ATTENTION: w is treated as unsigned.
    // int BN_mul_word(BIGNUM *a, BN_ULONG w);

    public static native int BN_mod_word(long a, int w);
    // ATTENTION: w is treated as unsigned.
    // BN_ULONG BN_mod_word(BIGNUM *a, BN_ULONG w);

    public static native void BN_add(long r, long a, long b);
    // int BN_add(BIGNUM *r, const BIGNUM *a, const BIGNUM *b);

    public static native void BN_sub(long r, long a, long b);
    // int BN_sub(BIGNUM *r, const BIGNUM *a, const BIGNUM *b);

    public static native void BN_gcd(long r, long a, long b);
    // int BN_gcd(BIGNUM *r, const BIGNUM *a, const BIGNUM *b, BN_CTX *ctx);

    public static native void BN_mul(long r, long a, long b);
    // int BN_mul(BIGNUM *r, const BIGNUM *a, const BIGNUM *b, BN_CTX *ctx);

    public static native void BN_exp(long r, long a, long p);
    // int BN_exp(BIGNUM *r, const BIGNUM *a, const BIGNUM *p, BN_CTX *ctx);

    public static native void BN_div(long dv, long rem, long m, long d);
    // int BN_div(BIGNUM *dv, BIGNUM *rem, const BIGNUM *m, const BIGNUM *d, BN_CTX *ctx);

    public static native void BN_nnmod(long r, long a, long m);
    // int BN_nnmod(BIGNUM *r, const BIGNUM *a, const BIGNUM *m, BN_CTX *ctx);

    public static native void BN_mod_exp(long r, long a, long p, long m);
    // int BN_mod_exp(BIGNUM *r, const BIGNUM *a, const BIGNUM *p, const BIGNUM *m, BN_CTX *ctx);

    public static native void BN_mod_inverse(long ret, long a, long n);
    // BIGNUM * BN_mod_inverse(BIGNUM *ret, const BIGNUM *a, const BIGNUM *n, BN_CTX *ctx);


    public static native void BN_generate_prime_ex(long ret, int bits, boolean safe,
                                                   long add, long rem, long cb);
    // int BN_generate_prime_ex(BIGNUM *ret, int bits, int safe,
    //         const BIGNUM *add, const BIGNUM *rem, BN_GENCB *cb);

    public static native boolean BN_is_prime_ex(long p, int nchecks, long cb);
    // int BN_is_prime_ex(const BIGNUM *p, int nchecks, BN_CTX *ctx, BN_GENCB *cb);

    public static native long getNativeFinalizer();
    // &BN_free

    /** Returns the expected size of the native allocation for a BIGNUM.
     */
    public static long size() {
        // 36 bytes is an empirically determined approximation.
        return 36;
    }
}
