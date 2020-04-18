/*
 * Copyright (C) 2012 Google Inc.
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

package benchmarks.regression;

import com.google.caliper.Param;
import java.nio.ByteBuffer;

public class ByteBufferScalarVersusVectorBenchmark {
  @Param private ByteBufferBenchmark.MyByteOrder byteOrder;
  @Param({"true", "false"}) private boolean aligned;
  @Param private ByteBufferBenchmark.MyBufferType bufferType;

  public void timeManualByteBufferCopy(int reps) throws Exception {
    ByteBuffer src = ByteBufferBenchmark.newBuffer(byteOrder, aligned, bufferType);
    ByteBuffer dst = ByteBufferBenchmark.newBuffer(byteOrder, aligned, bufferType);
    for (int rep = 0; rep < reps; ++rep) {
      src.position(0);
      dst.position(0);
      for (int i = 0; i < 8192; ++i) {
        dst.put(src.get());
      }
    }
  }

  public void timeByteBufferBulkGet(int reps) throws Exception {
    ByteBuffer src = ByteBuffer.allocate(aligned ? 8192 : 8192 + 1);
    byte[] dst = new byte[8192];
    for (int rep = 0; rep < reps; ++rep) {
      src.position(aligned ? 0 : 1);
      src.get(dst, 0, dst.length);
    }
  }

  public void timeDirectByteBufferBulkGet(int reps) throws Exception {
    ByteBuffer src = ByteBuffer.allocateDirect(aligned ? 8192 : 8192 + 1);
    byte[] dst = new byte[8192];
    for (int rep = 0; rep < reps; ++rep) {
      src.position(aligned ? 0 : 1);
      src.get(dst, 0, dst.length);
    }
  }
}
