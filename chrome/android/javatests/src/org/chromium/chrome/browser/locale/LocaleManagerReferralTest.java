// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.locale;

import static org.hamcrest.CoreMatchers.containsString;
import static org.hamcrest.CoreMatchers.not;

import android.support.test.InstrumentationRegistry;
import android.support.test.filters.SmallTest;

import org.junit.After;
import org.junit.Assert;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;

import org.chromium.base.ThreadUtils;
import org.chromium.base.library_loader.ProcessInitException;
import org.chromium.base.test.BaseJUnit4ClassRunner;
import org.chromium.base.test.util.CallbackHelper;
import org.chromium.chrome.browser.init.ChromeBrowserInitializer;
import org.chromium.chrome.browser.search_engines.TemplateUrlService;
import org.chromium.chrome.test.util.ApplicationData;

import java.util.Locale;
import java.util.concurrent.Callable;
import java.util.concurrent.ExecutionException;
import java.util.concurrent.TimeoutException;

/**
 * Tests that verify the end to end behavior of appending referral IDs to search engines.
 */
@RunWith(BaseJUnit4ClassRunner.class)
public class LocaleManagerReferralTest {
    private Locale mDefaultLocale;
    private String mYandexReferralId = "";

    @Before
    public void setUp() throws ExecutionException, ProcessInitException {
        mDefaultLocale = Locale.getDefault();
        Locale.setDefault(new Locale("ru", "RU"));

        ApplicationData.clearAppData(InstrumentationRegistry.getTargetContext());

        LocaleManager.setInstanceForTest(new LocaleManager() {
            @Override
            protected String getYandexReferralId() {
                return mYandexReferralId;
            }
        });

        ThreadUtils.runOnUiThreadBlocking(new Callable<Void>() {
            @Override
            public Void call() throws ProcessInitException {
                ChromeBrowserInitializer.getInstance(InstrumentationRegistry.getTargetContext())
                        .handleSynchronousStartup();
                return null;
            }
        });
    }

    @After
    public void tearDown() {
        Locale.setDefault(mDefaultLocale);
    }

    @SmallTest
    @Test
    public void testYandexReferralId() throws InterruptedException, TimeoutException {
        final CallbackHelper templateUrlServiceLoaded = new CallbackHelper();
        ThreadUtils.runOnUiThreadBlocking(new Runnable() {
            @Override
            public void run() {
                TemplateUrlService templateUrlService = TemplateUrlService.getInstance();
                templateUrlService.registerLoadListener(new TemplateUrlService.LoadListener() {
                    @Override
                    public void onTemplateUrlServiceLoaded() {
                        templateUrlServiceLoaded.notifyCalled();
                    }
                });

                templateUrlService.load();
            }
        });

        templateUrlServiceLoaded.waitForCallback("Template URLs never loaded", 0);

        ThreadUtils.runOnUiThreadBlocking(new Runnable() {
            @Override
            public void run() {
                TemplateUrlService.getInstance().setSearchEngine("yandex.ru");

                // The initial param is empty, so ensure no clid param is passed.
                String url = TemplateUrlService.getInstance().getUrlForSearchQuery("blah");
                Assert.assertThat(url, not(containsString("&clid=")));

                // Initialize the value to something and verify it is included in the generated
                // URL.
                mYandexReferralId = "TESTING_IS_AWESOME";
                url = TemplateUrlService.getInstance().getUrlForSearchQuery("blah");
                Assert.assertThat(url, containsString("&clid=TESTING_IS_AWESOME"));

                // Switch to google and ensure the clid param is no longer included.
                TemplateUrlService.getInstance().setSearchEngine("google.com");
                url = TemplateUrlService.getInstance().getUrlForSearchQuery("blah");
                Assert.assertThat(url, not(containsString("&clid=")));
            }
        });
    }
}
