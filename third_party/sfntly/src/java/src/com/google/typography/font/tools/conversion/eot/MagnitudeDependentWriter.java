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

import java.io.ByteArrayOutputStream;

/**
 * Write a stream of values using magnitude dependent encoding, as per section 5.3 of the spec.
 * 
 * @author Raph Levien
 */
public class MagnitudeDependentWriter {

  private final ByteArrayOutputStream buf;
  private byte byteBuf;
  private int bitCount;
  
  public MagnitudeDependentWriter() {
    buf = new ByteArrayOutputStream();
    bitCount = 0;
    byteBuf = 0;
  }
  
  private void writeBit(int bit) {
    byteBuf |= (bit << bitCount);
    bitCount++;
    if (bitCount == 8) {
      buf.write(byteBuf);
      byteBuf = 0;
      bitCount = 0;
    }
  }

  public void writeValue(int value) {
    if (value == 0) {
      writeBit(0);
    } else {
      int absValue = Math.abs(value);
      for (int i = 0; i < absValue; i++) {
        writeBit(1);
      }
      writeBit(0);
      writeBit(value > 0 ? 0 : 1);
    }
  }

  public void flush() {
    if (bitCount > 0) {
      buf.write(byteBuf);
      byteBuf = 0;
      bitCount = 0;
    }
  }
  
  public byte[] toByteArray() {
    return buf.toByteArray();
  }
}
