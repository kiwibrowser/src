// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.installedapp;

import org.junit.Assert;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.BlockJUnit4ClassRunner;

import org.chromium.base.test.util.Feature;

/**
 * Tests that PackageHash generates correct hashes.
 */
@RunWith(BlockJUnit4ClassRunner.class)
public class PackageHashTest {
    @Test
    @Feature({"InstalledApp"})
    public void testPackageHash() {
        byte[] salt = {0x64, 0x09, -0x68, -0x25, 0x70, 0x11, 0x25, 0x24, 0x68, -0x1a, 0x08, 0x79,
                -0x12, -0x50, 0x3b, -0x57, -0x17, -0x4d, 0x46, 0x02};
        PackageHash.setGlobalSaltForTesting(salt);
        // These expectations are based on the salt + ':' + packageName, encoded in UTF-8, hashed
        // with SHA-256, and looking at the first two bytes of the result.
        Assert.assertEquals((short) 0x0d4d, PackageHash.hashForPackage("com.example.test1"));
        Assert.assertEquals((short) 0xfa6f, PackageHash.hashForPackage("com.example.t\u00e9st2"));

        byte[] salt2 = {-0x10, 0x38, -0x28, 0x1f, 0x59, 0x2d, -0x2d, -0x4a, 0x23, 0x76, 0x6d, -0x54,
                0x27, -0x2d, -0x3f, -0x59, -0x2e, -0x0e, 0x67, 0x7a};
        PackageHash.setGlobalSaltForTesting(salt2);
        Assert.assertEquals((short) 0xd6d6, PackageHash.hashForPackage("com.example.test1"));
        Assert.assertEquals((short) 0x5193, PackageHash.hashForPackage("com.example.t\u00e9st2"));
    }
}
