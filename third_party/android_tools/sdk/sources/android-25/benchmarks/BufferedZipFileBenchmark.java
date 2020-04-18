/*
 * Copyright (C) 2011 Google Inc.
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

package benchmarks;

import com.google.caliper.BeforeExperiment;
import com.google.caliper.Param;
import java.io.BufferedInputStream;
import java.io.File;
import java.io.FileOutputStream;
import java.io.InputStream;
import java.util.Random;
import java.util.zip.ZipEntry;
import java.util.zip.ZipFile;
import java.util.zip.ZipOutputStream;

public final class BufferedZipFileBenchmark {
    @Param({"128", "1024", "8192", "65536"}) int compressedSize;
    @Param({"4", "32", "128"}) int readSize;

    private File file;

    @BeforeExperiment
    protected void setUp() throws Exception {
        System.setProperty("java.io.tmpdir", "/data/local/tmp");
        file = File.createTempFile(getClass().getName(), ".zip");
        file.deleteOnExit();

        Random random = new Random(0);
        ZipOutputStream out = new ZipOutputStream(new FileOutputStream(file));
        byte[] data = new byte[8192];
        out.putNextEntry(new ZipEntry("entry.data"));
        int written = 0;
        while (written < compressedSize) {
            random.nextBytes(data);
            int toWrite = Math.min(compressedSize - written, data.length);
            out.write(data, 0, toWrite);
            written += toWrite;
        }
        out.close();
    }

    public void timeUnbufferedRead(int reps) throws Exception {
        for (int i = 0; i < reps; i++) {
            ZipFile zipFile = new ZipFile(file);
            ZipEntry entry = zipFile.getEntry("entry.data");
            InputStream in = zipFile.getInputStream(entry);
            byte[] buffer = new byte[readSize];
            while (in.read(buffer) != -1) {
            }
            in.close();
            zipFile.close();
        }
    }

    public void timeBufferedRead(int reps) throws Exception {
        for (int i = 0; i < reps; i++) {
            ZipFile zipFile = new ZipFile(file);
            ZipEntry entry = zipFile.getEntry("entry.data");
            InputStream in = new BufferedInputStream(zipFile.getInputStream(entry));
            byte[] buffer = new byte[readSize];
            while (in.read(buffer) != -1) {
            }
            in.close();
            zipFile.close();
        }
    }
}
