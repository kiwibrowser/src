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
/**
* @author Yuri A. Kropachev
* @version $Revision$
*/


package org.apache.harmony.security.provider.crypto;

import static org.apache.harmony.security.provider.crypto.SHA1Constants.*;

/**
 * This class contains methods providing SHA-1 functionality to use in classes. <BR>
 * The methods support the algorithm described in "SECURE HASH STANDARD", FIPS PUB 180-2, <BR>
 * "http://csrc.nist.gov/publications/fips/fips180-2/fips180-2.pdf"      <BR>
 * <BR>
 * The class contains two package level access methods, -
 * "void updateHash(int[], byte[], int, int)" and "void computeHash(int[])", -
 * performing the following operations. <BR>
 * <BR>
 * The "updateHash(..)" method appends new bytes to existing ones
 * within limit of a frame of 64 bytes (16 words).
 * Once a length of accumulated bytes reaches the limit
 * the "computeHash(int[])" method is invoked on the frame to compute updated hash,
 * and the number of bytes in the frame is set to 0.
 * Thus, after appending all bytes, the frame contain only those bytes
 * that were not used in computing final hash value yet. <BR>
 * <BR>
 * The "computeHash(..)" method generates a 160 bit hash value using
 * a 512 bit message stored in first 16 words of int[] array argument and
 * current hash value stored in five words, beginning HASH_OFFSET, of the array argument.
 * Computation is done according to SHA-1 algorithm. <BR>
 * <BR>
 * The resulting hash value replaces the previous hash value in the array;
 * original bits of the message are not preserved.
 */
public class SHA1Impl {


    /**
     * The method generates a 160 bit hash value using
     * a 512 bit message stored in first 16 words of int[] array argument and
     * current hash value stored in five words, beginning OFFSET+1, of the array argument.
     * Computation is done according to SHA-1 algorithm.
     *
     * The resulting hash value replaces the previous hash value in the array;
     * original bits of the message are not preserved.
     *
     * No checks on argument supplied, that is,
     * a calling method is responsible for such checks.
     * In case of incorrect array passed to the method
     * either NPE or IndexOutOfBoundException gets thrown by JVM.
     *
     * @params
     *        arrW - integer array; arrW.length >= (BYTES_OFFSET+6); <BR>
     *               only first (BYTES_OFFSET+6) words are used
     */
    static void computeHash(int[] arrW) {

        int  a = arrW[HASH_OFFSET   ];
        int  b = arrW[HASH_OFFSET +1];
        int  c = arrW[HASH_OFFSET +2];
        int  d = arrW[HASH_OFFSET +3];
        int  e = arrW[HASH_OFFSET +4];

        int temp;

        // In this implementation the "d. For t = 0 to 79 do" loop
        // is split into four loops. The following constants:
        //     K = 5A827999   0 <= t <= 19
        //     K = 6ED9EBA1  20 <= t <= 39
        //     K = 8F1BBCDC  40 <= t <= 59
        //     K = CA62C1D6  60 <= t <= 79
        // are hex literals in the loops.

        for ( int t = 16; t < 80 ; t++ ) {

            temp  = arrW[t-3] ^ arrW[t-8] ^ arrW[t-14] ^ arrW[t-16];
            arrW[t] = ( temp<<1 ) | ( temp>>>31 );
        }

        for ( int t = 0 ; t < 20 ; t++ ) {

            temp = ( ( a<<5 ) | ( a>>>27 )   ) +
                   ( ( b & c) | ((~b) & d)   ) +
                   ( e + arrW[t] + 0x5A827999 ) ;
            e = d;
            d = c;
            c = ( b<<30 ) | ( b>>>2 ) ;
            b = a;
            a = temp;
        }
        for ( int t = 20 ; t < 40 ; t++ ) {

            temp = ((( a<<5 ) | ( a>>>27 ))) + (b ^ c ^ d) + (e + arrW[t] + 0x6ED9EBA1) ;
            e = d;
            d = c;
            c = ( b<<30 ) | ( b>>>2 ) ;
            b = a;
            a = temp;
        }
        for ( int t = 40 ; t < 60 ; t++ ) {

            temp = (( a<<5 ) | ( a>>>27 )) + ((b & c) | (b & d) | (c & d)) +
                                                             (e + arrW[t] + 0x8F1BBCDC) ;
            e = d;
            d = c;
            c = ( b<<30 ) | ( b>>>2 ) ;
            b = a;
            a = temp;
        }
        for ( int t = 60 ; t < 80 ; t++ ) {

            temp = ((( a<<5 ) | ( a>>>27 ))) + (b ^ c ^ d) + (e + arrW[t] + 0xCA62C1D6) ;
            e = d;
            d = c;
            c = ( b<<30 ) | ( b>>>2 ) ;
            b = a;
            a = temp;
        }

        arrW[HASH_OFFSET   ] += a;
        arrW[HASH_OFFSET +1] += b;
        arrW[HASH_OFFSET +2] += c;
        arrW[HASH_OFFSET +3] += d;
        arrW[HASH_OFFSET +4] += e;
    }

    /**
     * The method appends new bytes to existing ones
     * within limit of a frame of 64 bytes (16 words).
     *
     * Once a length of accumulated bytes reaches the limit
     * the "computeHash(int[])" method is invoked on the array to compute updated hash,
     * and the number of bytes in the frame is set to 0.
     * Thus, after appending all bytes, the array contain only those bytes
     * that were not used in computing final hash value yet.
     *
     * No checks on arguments passed to the method, that is,
     * a calling method is responsible for such checks.
     *
     * @params
     *        intArray  - int array containing bytes to which to append;
     *                    intArray.length >= (BYTES_OFFSET+6)
     * @params
     *        byteInput - array of bytes to use for the update
     * @params
     *        from      - the offset to start in the "byteInput" array
     * @params
     *        to        - a number of the last byte in the input array to use,
     *                that is, for first byte "to"==0, for last byte "to"==input.length-1
     */
    static void updateHash(int[] intArray, byte[] byteInput, int fromByte, int toByte) {

        // As intArray contains a packed bytes
        // the buffer's index is in the intArray[BYTES_OFFSET] element

        int index = intArray[BYTES_OFFSET];
        int i = fromByte;
        int maxWord;
        int nBytes;

        int wordIndex = index >>2;
        int byteIndex = index & 0x03;

        intArray[BYTES_OFFSET] = ( index + toByte - fromByte + 1 ) & 077 ;

        // In general case there are 3 stages :
        // - appending bytes to non-full word,
        // - writing 4 bytes into empty words,
        // - writing less than 4 bytes in last word

        if ( byteIndex != 0 ) {       // appending bytes in non-full word (as if)

            for ( ; ( i <= toByte ) && ( byteIndex < 4 ) ; i++ ) {
                intArray[wordIndex] |= ( byteInput[i] & 0xFF ) << ((3 - byteIndex)<<3) ;
                byteIndex++;
            }
            if ( byteIndex == 4 ) {
                wordIndex++;
                if ( wordIndex == 16 ) {          // intArray is full, computing hash

                    computeHash(intArray);
                    wordIndex = 0;
                }
            }
            if ( i > toByte ) {                 // all input bytes appended
                return ;
            }
        }

        // writing full words

        maxWord = (toByte - i + 1) >> 2;           // # of remaining full words, may be "0"
        for ( int k = 0; k < maxWord ; k++ ) {

            intArray[wordIndex] = ( ((int) byteInput[i   ] & 0xFF) <<24 ) |
                                  ( ((int) byteInput[i +1] & 0xFF) <<16 ) |
                                  ( ((int) byteInput[i +2] & 0xFF) <<8  ) |
                                  ( ((int) byteInput[i +3] & 0xFF)      )  ;
            i += 4;
            wordIndex++;

            if ( wordIndex < 16 ) {     // buffer is not full yet
                continue;
            }
            computeHash(intArray);      // buffer is full, computing hash
            wordIndex = 0;
        }

        // writing last incomplete word
        // after writing free byte positions are set to "0"s

        nBytes = toByte - i +1;
        if ( nBytes != 0 ) {

            int w =  ((int) byteInput[i] & 0xFF) <<24 ;

            if ( nBytes != 1 ) {
                w |= ((int) byteInput[i +1] & 0xFF) <<16 ;
                if ( nBytes != 2) {
                    w |= ((int) byteInput[i +2] & 0xFF) <<8 ;
                }
            }
            intArray[wordIndex] = w;
        }

        return ;
    }

}
