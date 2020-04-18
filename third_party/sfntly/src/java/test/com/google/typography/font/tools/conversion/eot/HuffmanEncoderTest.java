/*
 * Copyright 2011 Google Inc. All Rights Reserved.
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

package com.google.typography.font.tools.conversion.eot;

import junit.framework.TestCase;

/**
 * @author Raph Levien
 */
public class HuffmanEncoderTest extends TestCase {
  
   public void testBitsUsed() {
     assertEquals(1, HuffmanEncoder.bitsUsed(0));
     assertEquals(1, HuffmanEncoder.bitsUsed(1));
     assertEquals(8, HuffmanEncoder.bitsUsed(255));
     assertEquals(9, HuffmanEncoder.bitsUsed(256));
     assertEquals(32, HuffmanEncoder.bitsUsed(-1));
   }
   
   public void testInitConsistency() {
     BitIOWriter bits = new BitIOWriter();
     HuffmanEncoder h = new HuffmanEncoder(bits, 512);
     assertNull(h.checkTree());
   }

   public void testRetainConsistency() {
     BitIOWriter bits = new BitIOWriter();
     HuffmanEncoder h = new HuffmanEncoder(bits, 8);
     for (int i = 0; i < 1024; i++) {
       assertNull("Iteration " + i, h.checkTree());
       h.writeSymbol((i & 1) != 0 ? 0 : i & 7);
     }
   }
}
