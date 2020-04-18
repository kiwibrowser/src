// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.invalidation;

import android.content.Intent;
import android.support.test.InstrumentationRegistry;
import android.support.test.annotation.UiThreadTest;
import android.support.test.filters.SmallTest;
import android.support.test.rule.UiThreadTestRule;

import com.google.ipc.invalidation.external.client.types.ObjectId;

import org.junit.Assert;
import org.junit.Before;
import org.junit.Rule;
import org.junit.Test;
import org.junit.rules.RuleChain;
import org.junit.runner.RunWith;

import org.chromium.base.ApiCompatibilityUtils;
import org.chromium.base.ContextUtils;
import org.chromium.base.test.BaseJUnit4ClassRunner;
import org.chromium.base.test.util.Feature;
import org.chromium.chrome.browser.test.ChromeBrowserTestRule;
import org.chromium.chrome.test.invalidation.IntentSavingContext;
import org.chromium.components.invalidation.InvalidationService;
import org.chromium.components.sync.AndroidSyncSettings;
import org.chromium.components.sync.ModelType;
import org.chromium.components.sync.ModelTypeHelper;
import org.chromium.components.sync.notifier.InvalidationIntentProtocol;
import org.chromium.components.sync.test.util.MockSyncContentResolverDelegate;

import java.util.Set;

/**
 * Tests for {@link InvalidationService}.
 */
@RunWith(BaseJUnit4ClassRunner.class)
public class InvalidationServiceTest {
    @Rule
    public final RuleChain mChain =
            RuleChain.outerRule(new ChromeBrowserTestRule()).around(new UiThreadTestRule());

    private IntentSavingContext mAppContext;

    @Before
    public void setUp() throws Exception {
        mAppContext = new IntentSavingContext(InstrumentationRegistry.getInstrumentation()
                                                      .getTargetContext()
                                                      .getApplicationContext());
        // We don't want to use the system content resolver, so we override it.
        MockSyncContentResolverDelegate delegate = new MockSyncContentResolverDelegate();
        // Android master sync can safely always be on.
        delegate.setMasterSyncAutomatically(true);
        AndroidSyncSettings.overrideForTests(mAppContext, delegate, null);
        ContextUtils.initApplicationContextForTests(mAppContext);
    }

    @Test
    @SmallTest
    @UiThreadTest
    @Feature({"Sync"})
    public void testSetRegisteredObjectIds() throws Throwable {
        InvalidationService service = InvalidationServiceFactory.getForTest();
        ObjectId bookmark = ModelTypeHelper.toObjectId(ModelType.BOOKMARKS);
        service.setRegisteredObjectIds(new int[] {1, 2, bookmark.getSource()},
                new String[] {"a", "b", new String(bookmark.getName())});
        Assert.assertEquals(1, mAppContext.getNumStartedIntents());

        // Validate destination.
        Intent intent = mAppContext.getStartedIntent(0);
        validateIntentComponent(intent);
        Assert.assertEquals(InvalidationIntentProtocol.ACTION_REGISTER, intent.getAction());

        // Validate registered object ids. The bookmark object should not be registered
        // since it is a Sync type.
        Assert.assertNull(
                intent.getStringArrayListExtra(InvalidationIntentProtocol.EXTRA_REGISTERED_TYPES));
        Set<ObjectId> objectIds = InvalidationIntentProtocol.getRegisteredObjectIds(intent);
        Assert.assertEquals(2, objectIds.size());
        Assert.assertTrue(objectIds.contains(
                ObjectId.newInstance(1, ApiCompatibilityUtils.getBytesUtf8("a"))));
        Assert.assertTrue(objectIds.contains(
                ObjectId.newInstance(2, ApiCompatibilityUtils.getBytesUtf8("b"))));
    }

    /**
     * Asserts that {@code intent} is destined for the correct component.
     */
    private static void validateIntentComponent(Intent intent) {
        Assert.assertNotNull(intent.getComponent());
        Assert.assertEquals(ChromeInvalidationClientService.class.getName(),
                intent.getComponent().getClassName());
    }

}
