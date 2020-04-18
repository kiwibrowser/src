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

import org.junit.Assert;
import org.junit.Test;

/**
 * Test for {@link MultiDex} class.
 */
public class MultiDexTest {
    @Test
    public void testVersionCheck() {
        Assert.assertFalse(MultiDex.isVMMultidexCapable(null));
        Assert.assertFalse(MultiDex.isVMMultidexCapable("-1.32.54"));
        Assert.assertFalse(MultiDex.isVMMultidexCapable("1.32.54"));
        Assert.assertFalse(MultiDex.isVMMultidexCapable("1.32"));
        Assert.assertFalse(MultiDex.isVMMultidexCapable("2.0"));
        Assert.assertFalse(MultiDex.isVMMultidexCapable("2.000.1254"));
        Assert.assertTrue(MultiDex.isVMMultidexCapable("2.1.1254"));
        Assert.assertTrue(MultiDex.isVMMultidexCapable("2.1"));
        Assert.assertTrue(MultiDex.isVMMultidexCapable("2.2"));
        Assert.assertTrue(MultiDex.isVMMultidexCapable("2.1.0000"));
        Assert.assertTrue(MultiDex.isVMMultidexCapable("2.2.0000"));
        Assert.assertTrue(MultiDex.isVMMultidexCapable("002.0001.0010"));
        Assert.assertTrue(MultiDex.isVMMultidexCapable("3.0"));
        Assert.assertTrue(MultiDex.isVMMultidexCapable("3.0.0"));
        Assert.assertTrue(MultiDex.isVMMultidexCapable("3.0.1"));
        Assert.assertTrue(MultiDex.isVMMultidexCapable("3.1.0"));
        Assert.assertTrue(MultiDex.isVMMultidexCapable("03.1.132645"));
        Assert.assertTrue(MultiDex.isVMMultidexCapable("03.2"));
    }
}
