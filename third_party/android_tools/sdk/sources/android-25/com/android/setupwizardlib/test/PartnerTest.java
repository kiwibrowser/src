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

package com.android.setupwizardlib.test;

import android.content.Context;
import android.content.ContextWrapper;
import android.content.Intent;
import android.content.pm.ActivityInfo;
import android.content.pm.ApplicationInfo;
import android.content.pm.PackageManager;
import android.content.pm.ResolveInfo;
import android.content.res.Resources;
import android.test.InstrumentationTestCase;
import android.test.mock.MockPackageManager;
import android.test.mock.MockResources;
import android.test.suitebuilder.annotation.SmallTest;
import android.util.SparseArray;

import com.android.setupwizardlib.util.Partner;
import com.android.setupwizardlib.util.Partner.ResourceEntry;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

public class PartnerTest extends InstrumentationTestCase {

    private TestContext mTestContext;

    @Override
    protected void setUp() throws Exception {
        super.setUp();
        mTestContext = new TestContext(getInstrumentation().getTargetContext());
        Partner.resetForTesting();
    }

    @SmallTest
    public void testLoadPartner() {
        mTestContext.partnerList = Arrays.asList(
                createResolveInfo("hocus.pocus", false),
                createResolveInfo("com.android.setupwizardlib.test", true)
        );

        Partner partner = Partner.get(mTestContext);
        assertNotNull("Partner should not be null", partner);
    }

    @SmallTest
    public void testLoadNoPartner() {
        mTestContext.partnerList = new ArrayList<>();

        Partner partner = Partner.get(mTestContext);
        assertNull("Partner should be null", partner);
    }

    @SmallTest
    public void testLoadNonSystemPartner() {
        mTestContext.partnerList = Arrays.asList(
                createResolveInfo("hocus.pocus", false),
                createResolveInfo("com.android.setupwizardlib.test", false)
        );

        Partner partner = Partner.get(mTestContext);
        assertNull("Partner should be null", partner);
    }

    @SmallTest
    public void testLoadPartnerValue() {
        mTestContext.partnerList = Arrays.asList(
                createResolveInfo("hocus.pocus", false),
                createResolveInfo("com.android.setupwizardlib.test", true)
        );

        ResourceEntry entry =
                Partner.getResourceEntry(mTestContext, R.integer.suwTransitionDuration);
        int partnerValue = entry.resources.getInteger(entry.id);
        assertEquals("Partner value should be overlaid to 5000", 5000, partnerValue);
        assertTrue("Partner value should come from overlay", entry.isOverlay);
    }

    @SmallTest
    public void testLoadDefaultValue() {
        mTestContext.partnerList = Arrays.asList(
                createResolveInfo("hocus.pocus", false),
                createResolveInfo("com.android.setupwizardlib.test", true)
        );

        ResourceEntry entry =
                Partner.getResourceEntry(mTestContext, R.color.suw_color_accent_dark);
        int partnerValue = entry.resources.getColor(entry.id);
        assertEquals("Partner value should default to 0xff448aff", 0xff448aff, partnerValue);
        assertFalse("Partner value should come from fallback", entry.isOverlay);
    }

    private ResolveInfo createResolveInfo(String packageName, boolean isSystem) {
        ResolveInfo info = new ResolveInfo();
        info.resolvePackageName = packageName;
        ActivityInfo activityInfo = new ActivityInfo();
        ApplicationInfo appInfo = new ApplicationInfo();
        appInfo.flags = isSystem ? ApplicationInfo.FLAG_SYSTEM : 0;
        appInfo.packageName = packageName;
        activityInfo.applicationInfo = appInfo;
        activityInfo.packageName = packageName;
        activityInfo.name = packageName;
        info.activityInfo = activityInfo;
        return info;
    }

    private static class TestResources extends MockResources {

        private static final Map<String, Integer> TEST_RESOURCE_IDS = new HashMap<>();
        private static final SparseArray<Object> TEST_RESOURCES = new SparseArray<>();

        private static void addItem(String name, int id, Object value) {
            TEST_RESOURCE_IDS.put(name, id);
            TEST_RESOURCES.put(id, value);
        }

        static {
            addItem("integer/suwTransitionDuration", 0x7f010000, 5000);
        }

        @Override
        public int getIdentifier(String name, String defType, String defPackage) {
            String key = defType + "/" + name;
            if (TEST_RESOURCE_IDS.containsKey(key)) {
                return TEST_RESOURCE_IDS.get(key);
            }
            return 0;
        }

        @Override
        public int getInteger(int id) throws NotFoundException {
            if (TEST_RESOURCES.indexOfKey(id) >= 0) {
                return (int) TEST_RESOURCES.get(id);
            } else {
                throw new NotFoundException();
            }
        }

        @Override
        public int getColor(int id) throws NotFoundException {
            if (TEST_RESOURCES.indexOfKey(id) >= 0) {
                return (int) TEST_RESOURCES.get(id);
            } else {
                throw new NotFoundException();
            }
        }
    }

    private static class TestPackageManager extends MockPackageManager {

        private Context mTestContext;
        private Resources mTestResources;

        public TestPackageManager(Context testContext) {
            mTestContext = testContext;
            mTestResources = new TestResources();
        }

        @Override
        public Resources getResourcesForApplication(ApplicationInfo app) {
            if (app != null && "com.android.setupwizardlib.test".equals(app.packageName)) {
                return mTestResources;
            } else {
                return super.getResourcesForApplication(app);
            }
        }

        @Override
        public List<ResolveInfo> queryBroadcastReceivers(Intent intent, int flags) {
            if ("com.android.setupwizard.action.PARTNER_CUSTOMIZATION".equals(intent.getAction())) {
                return ((TestContext) mTestContext).partnerList;
            } else {
                return super.queryBroadcastReceivers(intent, flags);
            }
        }
    }

    private static class TestContext extends ContextWrapper {

        public List<ResolveInfo> partnerList;

        public TestContext(Context context) {
            super(context);
        }

        @Override
        public PackageManager getPackageManager() {
            return new TestPackageManager(this);
        }
    }
}
