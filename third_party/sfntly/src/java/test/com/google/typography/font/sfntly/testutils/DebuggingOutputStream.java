/*
 * Copyright 2010 Google Inc. All Rights Reserved.
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

package com.google.typography.font.sfntly.testutils;

import java.io.FilterOutputStream;
import java.io.IOException;
import java.io.OutputStream;

public class DebuggingOutputStream extends FilterOutputStream {

  private static final int LINE_LENGTH = 20;

  private int lineLength;
  private boolean debug;

  public DebuggingOutputStream(OutputStream out, boolean debug) {
    super(out);
    this.debug = debug;
  }

  @Override
  public void write(byte[] b, int off, int len) throws IOException {
    for (byte bi : b) {
      this.write(bi);
    }
  }

  @Override
  public void write(byte[] b) throws IOException {
    this.write(b, 0, b.length);
  }

  @Override
  public void write(int b) throws IOException {
    if (this.debug) {
      System.out.print(Integer.toHexString(0xff & b) + " ");
      if (++lineLength == LINE_LENGTH) {
        System.out.println();
        lineLength = 0;
      }
    }
    super.write(b);
  }

}
