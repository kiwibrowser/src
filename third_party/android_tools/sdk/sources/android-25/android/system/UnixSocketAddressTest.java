/*
 * Copyright (C) 2015 The Android Open Source Project
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

package android.system;

import junit.framework.TestCase;

import java.nio.charset.StandardCharsets;
import java.util.Arrays;

public class UnixSocketAddressTest extends TestCase {

    public void testFilesystemSunPath() throws Exception {
        String path = "/foo/bar";
        UnixSocketAddress sa = UnixSocketAddress.createFileSystem(path);

        byte[] abstractNameBytes = path.getBytes(StandardCharsets.UTF_8);
        byte[] expected = new byte[abstractNameBytes.length + 1];
        // See unix(7)
        System.arraycopy(abstractNameBytes, 0, expected, 0, abstractNameBytes.length);
        assertTrue(Arrays.equals(expected, sa.getSunPath()));
    }

    public void testUnnamedSunPath() throws Exception {
        UnixSocketAddress sa = UnixSocketAddress.createUnnamed();
        assertEquals(0, sa.getSunPath().length);
    }

    public void testAbstractSunPath() throws Exception {
        String abstractName = "abstract";
        UnixSocketAddress sa = UnixSocketAddress.createAbstract(abstractName);
        byte[] abstractNameBytes = abstractName.getBytes(StandardCharsets.UTF_8);
        byte[] expected = new byte[abstractNameBytes.length + 1];
        // See unix(7)
        System.arraycopy(abstractNameBytes, 0, expected, 1, abstractNameBytes.length);
        assertTrue(Arrays.equals(expected, sa.getSunPath()));
    }
}
