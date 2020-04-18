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
 * @author Raph Levien
 */
public class BitIOWriter {
  
  private ByteArrayOutputStream buf;
  private byte byteBuf;
  private int bitCount;
  
  public BitIOWriter() {
    buf = new ByteArrayOutputStream();
    bitCount = 0;
    byteBuf = 0;
  }
  
  public void writeBit(int bit) {
    byteBuf |= (bit << (7 - bitCount));
    bitCount++;
    if (bitCount == 8) {
      buf.write(byteBuf);
      byteBuf = 0;
      bitCount = 0;
    }
  }

  public void writeBit(boolean bit) {
    writeBit(bit ? 1 : 0);
  }
  
  public void writeValue(int value, int numBits) {
    // TODO: optimize, but we're shooting for correctness first
    for (int i = numBits - 1; i >= 0; i--) {
      writeBit((value >> i) & 1);
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
