/*
 * Copyright (C) 2014 The Android Open Source Project
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

package android.support.multidex;

import android.support.multidex.ZipUtil.CentralDirectory;

import org.junit.Assert;
import org.junit.BeforeClass;
import org.junit.Test;

import java.io.EOFException;
import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.io.RandomAccessFile;
import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.util.Enumeration;
import java.util.HashMap;
import java.util.Map;
import java.util.zip.ZipEntry;
import java.util.zip.ZipException;
import java.util.zip.ZipFile;

/**
 * Tests of ZipUtil class.
 *
 * The test assumes that ANDROID_BUILD_TOP environment variable is defined and point to the top of a
 * built android tree. This is the case when the console used for running the tests is setup for
 * android tree compilation.
 */
public class ZipUtilTest {
    private static final File zipFile = new File(System.getenv("ANDROID_BUILD_TOP"),
        "out/target/common/obj/JAVA_LIBRARIES/android-support-multidex_intermediates/javalib.jar");
    @BeforeClass
    public static void setupClass() throws ZipException, IOException {
        // just verify the zip is valid
        new ZipFile(zipFile).close();
    }

    @Test
    public void testCrcDoNotCrash() throws IOException {

        long crc =
                ZipUtil.getZipCrc(zipFile);
        System.out.println("crc is " + crc);

    }

    @Test
    public void testCrcRange() throws IOException {
        RandomAccessFile raf = new RandomAccessFile(zipFile, "r");
        CentralDirectory dir = ZipUtil.findCentralDirectory(raf);
        byte[] dirData = new byte[(int) dir.size];
        int length = dirData.length;
        int off = 0;
        raf.seek(dir.offset);
        while (length > 0) {
            int read = raf.read(dirData, off, length);
            if (length == -1) {
                throw new EOFException();
            }
            length -= read;
            off += read;
        }
        raf.close();
        ByteBuffer buffer = ByteBuffer.wrap(dirData);
        Map<String, ZipEntry> toCheck = new HashMap<String, ZipEntry>();
        while (buffer.hasRemaining()) {
            buffer = buffer.slice();
            buffer.order(ByteOrder.LITTLE_ENDIAN);
            ZipEntry entry = ZipEntryReader.readEntry(buffer);
            toCheck.put(entry.getName(), entry);
        }

        ZipFile zip = new ZipFile(zipFile);
        Assert.assertEquals(zip.size(), toCheck.size());
        Enumeration<? extends ZipEntry> ref = zip.entries();
        while (ref.hasMoreElements()) {
            ZipEntry refEntry = ref.nextElement();
            ZipEntry checkEntry = toCheck.get(refEntry.getName());
            Assert.assertNotNull(checkEntry);
            Assert.assertEquals(refEntry.getName(), checkEntry.getName());
            Assert.assertEquals(refEntry.getComment(), checkEntry.getComment());
            Assert.assertEquals(refEntry.getTime(), checkEntry.getTime());
            Assert.assertEquals(refEntry.getCrc(), checkEntry.getCrc());
            Assert.assertEquals(refEntry.getCompressedSize(), checkEntry.getCompressedSize());
            Assert.assertEquals(refEntry.getSize(), checkEntry.getSize());
            Assert.assertEquals(refEntry.getMethod(), checkEntry.getMethod());
            Assert.assertArrayEquals(refEntry.getExtra(), checkEntry.getExtra());
        }
        zip.close();
    }

    @Test
    public void testCrcValue() throws IOException {
        ZipFile zip = new ZipFile(zipFile);
        Enumeration<? extends ZipEntry> ref = zip.entries();
        byte[] buffer = new byte[0x2000];
        while (ref.hasMoreElements()) {
            ZipEntry refEntry = ref.nextElement();
            if (refEntry.getSize() > 0) {
                File tmp = File.createTempFile("ZipUtilTest", ".fakezip");
                InputStream in = zip.getInputStream(refEntry);
                OutputStream out = new FileOutputStream(tmp);
                int read = in.read(buffer);
                while (read != -1) {
                    out.write(buffer, 0, read);
                    read = in.read(buffer);
                }
                in.close();
                out.close();
                RandomAccessFile raf = new RandomAccessFile(tmp, "r");
                CentralDirectory dir = new CentralDirectory();
                dir.offset = 0;
                dir.size = raf.length();
                long crc = ZipUtil.computeCrcOfCentralDir(raf, dir);
                Assert.assertEquals(refEntry.getCrc(), crc);
                raf.close();
                tmp.delete();
            }
        }
        zip.close();
    }
    @Test
    public void testInvalidCrcValue() throws IOException {
        ZipFile zip = new ZipFile(zipFile);
        Enumeration<? extends ZipEntry> ref = zip.entries();
        byte[] buffer = new byte[0x2000];
        while (ref.hasMoreElements()) {
            ZipEntry refEntry = ref.nextElement();
            if (refEntry.getSize() > 0) {
                File tmp = File.createTempFile("ZipUtilTest", ".fakezip");
                InputStream in = zip.getInputStream(refEntry);
                OutputStream out = new FileOutputStream(tmp);
                int read = in.read(buffer);
                while (read != -1) {
                    out.write(buffer, 0, read);
                    read = in.read(buffer);
                }
                in.close();
                out.close();
                RandomAccessFile raf = new RandomAccessFile(tmp, "r");
                CentralDirectory dir = new CentralDirectory();
                dir.offset = 0;
                dir.size = raf.length() - 1;
                long crc = ZipUtil.computeCrcOfCentralDir(raf, dir);
                Assert.assertNotEquals(refEntry.getCrc(), crc);
                raf.close();
                tmp.delete();
            }
        }
        zip.close();
    }

}
