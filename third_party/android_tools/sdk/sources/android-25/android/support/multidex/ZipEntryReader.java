/*
 * Licensed to the Apache Software Foundation (ASF) under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.
 * The ASF licenses this file to You under the Apache License, Version 2.0
 * (the "License"); you may not use this file except in compliance with
 * the License.  You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
/* Apache Harmony HEADER because the code in this class comes mostly from ZipFile, ZipEntry and
 * ZipConstants from android libcore.
 */

package android.support.multidex;

import java.io.IOException;
import java.nio.ByteBuffer;
import java.nio.charset.Charset;
import java.util.Calendar;
import java.util.GregorianCalendar;
import java.util.zip.ZipEntry;
import java.util.zip.ZipException;

class ZipEntryReader {
    static final Charset UTF_8 = Charset.forName("UTF-8");
   /**
     * General Purpose Bit Flags, Bit 0.
     * If set, indicates that the file is encrypted.
     */
    private static final int GPBF_ENCRYPTED_FLAG = 1 << 0;

    /**
     * Supported General Purpose Bit Flags Mask.
     * Bit mask of bits not supported.
     * Note: The only bit that we will enforce at this time
     * is the encrypted bit. Although other bits are not supported,
     * we must not enforce them as this could break some legitimate
     * use cases (See http://b/8617715).
     */
    private static final int GPBF_UNSUPPORTED_MASK = GPBF_ENCRYPTED_FLAG;
    private static final long CENSIG = 0x2014b50;

    static ZipEntry readEntry(ByteBuffer in) throws IOException {

        int sig = in.getInt();
        if (sig != CENSIG) {
             throw new ZipException("Central Directory Entry not found");
        }

        in.position(8);
        int gpbf = in.getShort() & 0xffff;

        if ((gpbf & GPBF_UNSUPPORTED_MASK) != 0) {
            throw new ZipException("Invalid General Purpose Bit Flag: " + gpbf);
        }

        int compressionMethod = in.getShort() & 0xffff;
        int time = in.getShort() & 0xffff;
        int modDate = in.getShort() & 0xffff;

        // These are 32-bit values in the file, but 64-bit fields in this object.
        long crc = ((long) in.getInt()) & 0xffffffffL;
        long compressedSize = ((long) in.getInt()) & 0xffffffffL;
        long size = ((long) in.getInt()) & 0xffffffffL;

        int nameLength = in.getShort() & 0xffff;
        int extraLength = in.getShort() & 0xffff;
        int commentByteCount = in.getShort() & 0xffff;

        // This is a 32-bit value in the file, but a 64-bit field in this object.
        in.position(42);
        long localHeaderRelOffset = ((long) in.getInt()) & 0xffffffffL;

        byte[] nameBytes = new byte[nameLength];
        in.get(nameBytes, 0, nameBytes.length);
        String name = new String(nameBytes, 0, nameBytes.length, UTF_8);

        ZipEntry entry = new ZipEntry(name);
        entry.setMethod(compressionMethod);
        entry.setTime(getTime(time, modDate));

        entry.setCrc(crc);
        entry.setCompressedSize(compressedSize);
        entry.setSize(size);

        // The RI has always assumed UTF-8. (If GPBF_UTF8_FLAG isn't set, the encoding is
        // actually IBM-437.)
        if (commentByteCount > 0) {
            byte[] commentBytes = new byte[commentByteCount];
            in.get(commentBytes, 0, commentByteCount);
            entry.setComment(new String(commentBytes, 0, commentBytes.length, UTF_8));
        }

        if (extraLength > 0) {
            byte[] extra = new byte[extraLength];
            in.get(extra, 0, extraLength);
            entry.setExtra(extra);
        }

        return entry;

    }

    private static long getTime(int time, int modDate) {
        GregorianCalendar cal = new GregorianCalendar();
        cal.set(Calendar.MILLISECOND, 0);
        cal.set(1980 + ((modDate >> 9) & 0x7f), ((modDate >> 5) & 0xf) - 1,
                modDate & 0x1f, (time >> 11) & 0x1f, (time >> 5) & 0x3f,
                (time & 0x1f) << 1);
        return cal.getTime().getTime();
    }

}
