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

package com.android.org.bouncycastle.crypto.digests;

import junit.framework.TestCase;
import com.android.org.bouncycastle.crypto.Digest;
import com.android.org.bouncycastle.crypto.ExtendedDigest;
import tests.util.SummaryStatistics;

/**
 * Implements unit tests for our JNI wrapper around OpenSSL. We use the
 * existing Bouncy Castle implementation as our test oracle.
 */
public class DigestTest extends TestCase {

    /**
     * Processes the two given message digests for the same data and checks
     * the results. Requirement is that the results must be equal, the digest
     * implementations must have the same properties, and the new implementation
     * must be faster than the old one.
     *
     * @param oldDigest The old digest implementation, provided by Bouncy Castle
     * @param newDigest The new digest implementation, provided by OpenSSL
     */
    public void doTestMessageDigest(Digest oldDigest, Digest newDigest) {
        final int WARMUP = 10;
        final int ITERATIONS = 100;

        byte[] data = new byte[1024];

        byte[] oldHash = new byte[oldDigest.getDigestSize()];
        byte[] newHash = new byte[newDigest.getDigestSize()];

        assertEquals("Hash names must be equal",
                     oldDigest.getAlgorithmName(), newDigest.getAlgorithmName());
        assertEquals("Hash sizes must be equal",
                     oldHash.length, newHash.length);
        assertEquals("Hash block sizes must be equal",
                     ((ExtendedDigest)oldDigest).getByteLength(),
                     ((ExtendedDigest)newDigest).getByteLength());
        for (int i = 0; i < data.length; i++) {
            data[i] = (byte)i;
        }

        SummaryStatistics oldTime = new SummaryStatistics();
        SummaryStatistics newTime = new SummaryStatistics();

        for (int j = 0; j < ITERATIONS + WARMUP; j++) {
            long t0 = System.nanoTime();
            for (int i = 0; i < 4; i++) {
                oldDigest.update(data, 0, data.length);
            }
            int oldLength = oldDigest.doFinal(oldHash, 0);
            long t1 = System.nanoTime();

            if (j >= WARMUP) {
                oldTime.add(t1 - t0);
            }

            long t2 = System.nanoTime();
            for (int i = 0; i < 4; i++) {
                newDigest.update(data, 0, data.length);
            }
            int newLength = newDigest.doFinal(newHash, 0);
            long t3 = System.nanoTime();

            if (j >= WARMUP) {
              newTime.add(t3 - t2);
            }

            assertEquals("Hash sizes must be equal", oldLength, newLength);

            for (int i = 0; i < oldLength; i++) {
                assertEquals("Hashes[" + i + "] must be equal", oldHash[i], newHash[i]);
            }
        }

        System.out.println("Time for " + ITERATIONS + " x old hash processing: "
                + oldTime.toString());
        System.out.println("Time for " + ITERATIONS + " x new hash processing: "
                + newTime.toString());
    }

    /**
     * Tests the MD5 implementation.
     */
    public void testMD5() {
        Digest oldDigest = new MD5Digest();
        Digest newDigest = new OpenSSLDigest.MD5();
        doTestMessageDigest(oldDigest, newDigest);
    }

    /**
     * Tests the SHA-1 implementation.
     */
    public void testSHA1() {
        Digest oldDigest = new SHA1Digest();
        Digest newDigest = new OpenSSLDigest.SHA1();
        doTestMessageDigest(oldDigest, newDigest);
    }

    /**
     * Tests the SHA-256 implementation.
     */
    public void testSHA256() {
        Digest oldDigest = new SHA256Digest();
        Digest newDigest = new OpenSSLDigest.SHA256();
        doTestMessageDigest(oldDigest, newDigest);
    }

    /**
     * Tests the SHA-384 implementation.
     */
    public void testSHA384() {
        Digest oldDigest = new SHA384Digest();
        Digest newDigest = new OpenSSLDigest.SHA384();
        doTestMessageDigest(oldDigest, newDigest);
    }

    /**
     * Tests the SHA-512 implementation.
     */
    public void testSHA512() {
        Digest oldDigest = new SHA512Digest();
        Digest newDigest = new OpenSSLDigest.SHA512();
        doTestMessageDigest(oldDigest, newDigest);
    }
}
