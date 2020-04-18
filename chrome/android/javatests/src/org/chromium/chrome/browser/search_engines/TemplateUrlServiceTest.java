// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.search_engines;

import android.net.Uri;
import android.support.test.filters.SmallTest;

import org.junit.Assert;
import org.junit.Rule;
import org.junit.Test;
import org.junit.rules.RuleChain;
import org.junit.runner.RunWith;

import org.chromium.base.ThreadUtils;
import org.chromium.base.test.BaseJUnit4ClassRunner;
import org.chromium.base.test.util.DisabledTest;
import org.chromium.base.test.util.Feature;
import org.chromium.base.test.util.Restriction;
import org.chromium.base.test.util.RetryOnFailure;
import org.chromium.chrome.browser.preferences.SearchEngineAdapter;
import org.chromium.chrome.browser.search_engines.TemplateUrlService.LoadListener;
import org.chromium.chrome.browser.test.ChromeBrowserTestRule;
import org.chromium.chrome.browser.test.ClearAppDataTestRule;
import org.chromium.content.browser.test.util.Criteria;
import org.chromium.content.browser.test.util.CriteriaHelper;
import org.chromium.ui.test.util.UiRestriction;

import java.util.List;
import java.util.concurrent.Callable;
import java.util.concurrent.ExecutionException;
import java.util.concurrent.atomic.AtomicBoolean;

/**
 * Tests for Chrome on Android's usage of the TemplateUrlService API.
 */
@RunWith(BaseJUnit4ClassRunner.class)
public class TemplateUrlServiceTest {
    @Rule
    public final RuleChain mChain =
            RuleChain.outerRule(new ClearAppDataTestRule()).around(new ChromeBrowserTestRule());

    private static final String QUERY_PARAMETER = "q";
    private static final String QUERY_VALUE = "cat";

    private static final String ALTERNATIVE_PARAMETER = "ctxsl_alternate_term";
    private static final String ALTERNATIVE_VALUE = "lion";

    private static final String VERSION_PARAMETER = "ctxs";
    private static final String VERSION_VALUE_TWO_REQUEST_PROTOCOL = "2";
    private static final String VERSION_VALUE_SINGLE_REQUEST_PROTOCOL = "3";

    private static final String PREFETCH_PARAMETER = "pf";
    private static final String PREFETCH_VALUE = "c";

    @Test
    @SmallTest
    @Feature({"ContextualSearch"})
    @RetryOnFailure
    public void testUrlForContextualSearchQueryValid() throws ExecutionException {
        waitForTemplateUrlServiceToLoad();

        Assert.assertTrue(ThreadUtils.runOnUiThreadBlockingNoException(new Callable<Boolean>() {
            @Override
            public Boolean call() throws Exception {
                return TemplateUrlService.getInstance().isLoaded();
            }
        }));

        validateQuery(QUERY_VALUE, ALTERNATIVE_VALUE, true, VERSION_VALUE_TWO_REQUEST_PROTOCOL);
        validateQuery(QUERY_VALUE, ALTERNATIVE_VALUE, false, VERSION_VALUE_TWO_REQUEST_PROTOCOL);
        validateQuery(QUERY_VALUE, null, true, VERSION_VALUE_TWO_REQUEST_PROTOCOL);
        validateQuery(QUERY_VALUE, null, false, VERSION_VALUE_TWO_REQUEST_PROTOCOL);
        validateQuery(QUERY_VALUE, null, true, VERSION_VALUE_SINGLE_REQUEST_PROTOCOL);
    }

    private void validateQuery(final String query, final String alternative, final boolean prefetch,
            final String protocolVersion)
            throws ExecutionException {
        String result = ThreadUtils.runOnUiThreadBlocking(new Callable<String>() {
            @Override
            public String call() throws Exception {
                return TemplateUrlService.getInstance().getUrlForContextualSearchQuery(
                        query, alternative, prefetch, protocolVersion);
            }
        });
        Assert.assertNotNull(result);
        Uri uri = Uri.parse(result);
        Assert.assertEquals(query, uri.getQueryParameter(QUERY_PARAMETER));
        Assert.assertEquals(alternative, uri.getQueryParameter(ALTERNATIVE_PARAMETER));
        Assert.assertEquals(protocolVersion, uri.getQueryParameter(VERSION_PARAMETER));
        if (prefetch) {
            Assert.assertEquals(PREFETCH_VALUE, uri.getQueryParameter(PREFETCH_PARAMETER));
        } else {
            Assert.assertNull(uri.getQueryParameter(PREFETCH_PARAMETER));
        }
    }

    @Test
    @SmallTest
    @Feature({"SearchEngines"})
    @RetryOnFailure
    @Restriction(UiRestriction.RESTRICTION_TYPE_PHONE) // see crbug.com/581268
    public void testLoadUrlService() {
        waitForTemplateUrlServiceToLoad();

        Assert.assertTrue(ThreadUtils.runOnUiThreadBlockingNoException(new Callable<Boolean>() {
            @Override
            public Boolean call() throws Exception {
                return TemplateUrlService.getInstance().isLoaded();
            }
        }));

        // Add another load listener and ensure that is notified without needing to call load()
        // again.
        final AtomicBoolean observerNotified = new AtomicBoolean(false);
        ThreadUtils.runOnUiThreadBlocking(new Runnable() {
            @Override
            public void run() {
                TemplateUrlService service = TemplateUrlService.getInstance();
                service.registerLoadListener(new LoadListener() {
                    @Override
                    public void onTemplateUrlServiceLoaded() {
                        observerNotified.set(true);
                    }
                });
            }
        });
        CriteriaHelper.pollInstrumentationThread(
                new Criteria("Observer wasn't notified of TemplateUrlService load.") {
                    @Override
                    public boolean isSatisfied() {
                        return observerNotified.get();
                    }
                });
    }

    @Test
    @SmallTest
    @Feature({"SearchEngines"})
    public void testSetAndGetSearchEngine() {
        final TemplateUrlService templateUrlService = waitForTemplateUrlServiceToLoad();

        List<TemplateUrl> searchEngines =
                ThreadUtils.runOnUiThreadBlockingNoException(new Callable<List<TemplateUrl>>() {
                    @Override
                    public List<TemplateUrl> call() throws Exception {
                        return templateUrlService.getTemplateUrls();
                    }
                });
        // Ensure known state of default search index before running test.
        TemplateUrl defaultSearchEngine =
                ThreadUtils.runOnUiThreadBlockingNoException(new Callable<TemplateUrl>() {
                    @Override
                    public TemplateUrl call() throws Exception {
                        return templateUrlService.getDefaultSearchEngineTemplateUrl();
                    }
                });
        SearchEngineAdapter.sortAndFilterUnnecessaryTemplateUrl(searchEngines, defaultSearchEngine);
        Assert.assertEquals(searchEngines.get(0), defaultSearchEngine);

        // Set search engine index and verified it stuck.
        ThreadUtils.runOnUiThreadBlocking(new Runnable() {
            @Override
            public void run() {
                Assert.assertTrue(
                        "There must be more than one search engine to change searchEngines",
                        searchEngines.size() > 1);
                templateUrlService.setSearchEngine(searchEngines.get(1).getKeyword());
            }
        });
        defaultSearchEngine =
                ThreadUtils.runOnUiThreadBlockingNoException(new Callable<TemplateUrl>() {
                    @Override
                    public TemplateUrl call() throws Exception {
                        return templateUrlService.getDefaultSearchEngineTemplateUrl();
                    }
                });
        Assert.assertEquals(searchEngines.get(1), defaultSearchEngine);
    }

    @Test
    @SmallTest
    @Feature({"SearchEngines"})
    @DisabledTest(message = "crbug.com/841098")
    public void testSortandGetCustomSearchEngine() {
        final TemplateUrlService templateUrlService = waitForTemplateUrlServiceToLoad();

        // Get the number of prepopulated search engine.
        final int prepopulatedEngineNum = getSearchEngineCount(templateUrlService);

        TemplateUrl defaultSearchEngine =
                ThreadUtils.runOnUiThreadBlockingNoException(new Callable<TemplateUrl>() {
                    @Override
                    public TemplateUrl call() throws Exception {
                        return templateUrlService.getDefaultSearchEngineTemplateUrl();
                    }
                });

        // Add custom search engines and verified only engines visited within 2 days are added.
        // Also verified custom engines are sorted correctly.
        List<TemplateUrl> customSearchEngines =
                ThreadUtils.runOnUiThreadBlockingNoException(new Callable<List<TemplateUrl>>() {
                    @Override
                    public List<TemplateUrl> call() throws Exception {
                        templateUrlService.addSearchEngineForTesting("keyword1", 0);
                        templateUrlService.addSearchEngineForTesting("keyword2", 0);
                        templateUrlService.addSearchEngineForTesting("keyword3", 3);
                        List<TemplateUrl> searchEngines = templateUrlService.getTemplateUrls();
                        SearchEngineAdapter.sortAndFilterUnnecessaryTemplateUrl(
                                searchEngines, defaultSearchEngine);
                        return searchEngines.subList(prepopulatedEngineNum, searchEngines.size());
                    }
                });
        Assert.assertEquals(2, customSearchEngines.size());
        Assert.assertEquals("keyword2", customSearchEngines.get(0).getKeyword());
        Assert.assertEquals("keyword1", customSearchEngines.get(1).getKeyword());

        // Add more custom search engines and verified at most 3 custom engines are returned.
        // Also verified custom engines are sorted correctly.
        customSearchEngines =
                ThreadUtils.runOnUiThreadBlockingNoException(new Callable<List<TemplateUrl>>() {
                    @Override
                    public List<TemplateUrl> call() throws Exception {
                        templateUrlService.addSearchEngineForTesting("keyword4", 0);
                        templateUrlService.addSearchEngineForTesting("keyword5", 0);
                        List<TemplateUrl> searchEngines = templateUrlService.getTemplateUrls();
                        SearchEngineAdapter.sortAndFilterUnnecessaryTemplateUrl(
                                searchEngines, defaultSearchEngine);
                        return searchEngines.subList(prepopulatedEngineNum, searchEngines.size());
                    }
                });
        Assert.assertEquals(3, customSearchEngines.size());
        Assert.assertEquals("keyword5", customSearchEngines.get(0).getKeyword());
        Assert.assertEquals("keyword4", customSearchEngines.get(1).getKeyword());
        Assert.assertEquals("keyword2", customSearchEngines.get(2).getKeyword());

        // Verified last_visited is updated correctly and sorting in descending order correctly.
        customSearchEngines =
                ThreadUtils.runOnUiThreadBlockingNoException(new Callable<List<TemplateUrl>>() {
                    @Override
                    public List<TemplateUrl> call() throws Exception {
                        templateUrlService.updateLastVisitedForTesting("keyword3");
                        List<TemplateUrl> searchEngines = templateUrlService.getTemplateUrls();
                        SearchEngineAdapter.sortAndFilterUnnecessaryTemplateUrl(
                                searchEngines, defaultSearchEngine);
                        return searchEngines.subList(prepopulatedEngineNum, searchEngines.size());
                    }
                });
        Assert.assertEquals(3, customSearchEngines.size());
        Assert.assertEquals("keyword3", customSearchEngines.get(0).getKeyword());
        Assert.assertEquals("keyword5", customSearchEngines.get(1).getKeyword());
        Assert.assertEquals("keyword4", customSearchEngines.get(2).getKeyword());

        // Set a custom engine as default provider and verified still 3 custom engines are returned.
        // Also verified custom engines are sorted correctly.
        customSearchEngines =
                ThreadUtils.runOnUiThreadBlockingNoException(new Callable<List<TemplateUrl>>() {
                    @Override
                    public List<TemplateUrl> call() throws Exception {
                        templateUrlService.setSearchEngine("keyword4");
                        List<TemplateUrl> searchEngines = templateUrlService.getTemplateUrls();
                        TemplateUrl newDefaultSearchEngine =
                                templateUrlService.getDefaultSearchEngineTemplateUrl();
                        SearchEngineAdapter.sortAndFilterUnnecessaryTemplateUrl(
                                searchEngines, newDefaultSearchEngine);
                        return searchEngines.subList(prepopulatedEngineNum, searchEngines.size());
                    }
                });
        Assert.assertEquals(4, customSearchEngines.size());
        Assert.assertEquals("keyword4", customSearchEngines.get(0).getKeyword());
        Assert.assertEquals("keyword3", customSearchEngines.get(1).getKeyword());
        Assert.assertEquals("keyword5", customSearchEngines.get(2).getKeyword());
        Assert.assertEquals("keyword2", customSearchEngines.get(3).getKeyword());
    }

    private int getSearchEngineCount(final TemplateUrlService templateUrlService) {
        return ThreadUtils.runOnUiThreadBlockingNoException(new Callable<Integer>() {
            @Override
            public Integer call() throws Exception {
                return templateUrlService.getTemplateUrls().size();
            }
        });
    }

    private TemplateUrlService waitForTemplateUrlServiceToLoad() {
        final AtomicBoolean observerNotified = new AtomicBoolean(false);
        final LoadListener listener = new LoadListener() {
            @Override
            public void onTemplateUrlServiceLoaded() {
                observerNotified.set(true);
            }
        };
        final TemplateUrlService templateUrlService = ThreadUtils.runOnUiThreadBlockingNoException(
                new Callable<TemplateUrlService>() {
                    @Override
                    public TemplateUrlService call() {
                        TemplateUrlService service = TemplateUrlService.getInstance();
                        service.registerLoadListener(listener);
                        service.load();
                        return service;
                    }
                });

        CriteriaHelper.pollInstrumentationThread(new Criteria(
                "Observer wasn't notified of TemplateUrlService load.") {
            @Override
            public boolean isSatisfied() {
                return observerNotified.get();
            }
        });
        return templateUrlService;
    }
}
