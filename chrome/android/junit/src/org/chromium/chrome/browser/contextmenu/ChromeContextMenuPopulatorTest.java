// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.contextmenu;

import static org.mockito.Mockito.doReturn;

import android.util.Pair;

import org.hamcrest.Matchers;
import org.junit.Assert;
import org.junit.Before;
import org.junit.Rule;
import org.junit.Test;
import org.junit.rules.TestRule;
import org.junit.runner.RunWith;
import org.mockito.Mock;
import org.mockito.Mockito;
import org.mockito.MockitoAnnotations;
import org.robolectric.RuntimeEnvironment;
import org.robolectric.annotation.Config;
import org.robolectric.annotation.Implementation;
import org.robolectric.annotation.Implements;

import org.chromium.base.test.BaseRobolectricTestRunner;
import org.chromium.blink_public.web.WebContextMenuMediaType;
import org.chromium.chrome.browser.ChromeFeatureList;
import org.chromium.chrome.browser.contextmenu.ChromeContextMenuPopulator.ContextMenuMode;
import org.chromium.chrome.browser.contextmenu.ChromeContextMenuPopulatorTest.ShadowUrlUtilities;
import org.chromium.chrome.browser.firstrun.FirstRunStatus;
import org.chromium.chrome.browser.search_engines.TemplateUrlService;
import org.chromium.chrome.browser.util.UrlUtilities;
import org.chromium.chrome.test.util.browser.Features;
import org.chromium.chrome.test.util.browser.Features.DisableFeatures;
import org.chromium.ui.base.MenuSourceType;

import java.util.ArrayList;
import java.util.List;

/**
 * Unit tests for the context menu logic of Chrome.
 */
@RunWith(BaseRobolectricTestRunner.class)
@Config(manifest = Config.NONE, shadows = {ShadowUrlUtilities.class})
@DisableFeatures(ChromeFeatureList.CUSTOM_CONTEXT_MENU)
public class ChromeContextMenuPopulatorTest {
    private static final String PAGE_URL = "http://www.blah.com";
    private static final String LINK_URL = "http://www.blah.com/other_blah";
    private static final String LINK_TEXT = "BLAH!";
    private static final String IMAGE_SRC_URL = "http://www.blah.com/image.jpg";
    private static final String IMAGE_TITLE_TEXT = "IMAGE!";

    @Rule
    public TestRule mFeaturesProcessor = new Features.JUnitProcessor();

    @Mock
    private ContextMenuItemDelegate mItemDelegate;
    @Mock
    private TemplateUrlService mTemplateUrlService;

    private ChromeContextMenuPopulator mPopulator;

    @Before
    public void setUp() throws Exception {
        MockitoAnnotations.initMocks(this);

        doReturn(PAGE_URL).when(mItemDelegate).getPageUrl();

        initializePopulator(ChromeContextMenuPopulator.NORMAL_MODE);
    }

    private void initializePopulator(@ContextMenuMode int mode) {
        mPopulator = Mockito.spy(new ChromeContextMenuPopulator(mItemDelegate, mode));
        doReturn(mTemplateUrlService).when(mPopulator).getTemplateUrlService();
    }

    @Test
    public void testBeforeFRE_Link() {
        FirstRunStatus.setFirstRunFlowComplete(false);

        final ContextMenuParams contextMenuParams = createLinkContextParams();

        List<ContextMenuItem> enabledItems = getEnabledItems(contextMenuParams);
        Assert.assertThat(enabledItems,
                Matchers.contains(ChromeContextMenuItem.COPY_LINK_ADDRESS,
                        ChromeContextMenuItem.COPY_LINK_TEXT));

        initializePopulator(ChromeContextMenuPopulator.CUSTOM_TAB_MODE);
        enabledItems = getEnabledItems(contextMenuParams);
        Assert.assertThat(enabledItems,
                Matchers.contains(ChromeContextMenuItem.COPY_LINK_ADDRESS,
                        ChromeContextMenuItem.COPY_LINK_TEXT));

        initializePopulator(ChromeContextMenuPopulator.WEB_APP_MODE);
        enabledItems = getEnabledItems(contextMenuParams);
        Assert.assertThat(enabledItems,
                Matchers.contains(ChromeContextMenuItem.COPY_LINK_ADDRESS,
                        ChromeContextMenuItem.COPY_LINK_TEXT));
    }

    @Test
    public void testBeforeFRE_Image() {
        FirstRunStatus.setFirstRunFlowComplete(false);

        final ContextMenuParams contextMenuParams = createImageContextParams();

        List<ContextMenuItem> enabledItems = getEnabledItems(contextMenuParams);
        Assert.assertThat(enabledItems, Matchers.empty());

        initializePopulator(ChromeContextMenuPopulator.CUSTOM_TAB_MODE);
        enabledItems = getEnabledItems(contextMenuParams);
        Assert.assertThat(enabledItems, Matchers.empty());

        initializePopulator(ChromeContextMenuPopulator.WEB_APP_MODE);
        enabledItems = getEnabledItems(contextMenuParams);
        Assert.assertThat(enabledItems, Matchers.empty());
    }

    @Test
    public void testBeforeFRE_ImageLink() {
        FirstRunStatus.setFirstRunFlowComplete(false);

        final ContextMenuParams contextMenuParams = createImageLinkContextParams();

        List<ContextMenuItem> enabledItems = getEnabledItems(contextMenuParams);
        Assert.assertThat(
                enabledItems, Matchers.containsInAnyOrder(ChromeContextMenuItem.COPY_LINK_ADDRESS));

        initializePopulator(ChromeContextMenuPopulator.CUSTOM_TAB_MODE);
        enabledItems = getEnabledItems(contextMenuParams);
        Assert.assertThat(
                enabledItems, Matchers.containsInAnyOrder(ChromeContextMenuItem.COPY_LINK_ADDRESS));

        initializePopulator(ChromeContextMenuPopulator.WEB_APP_MODE);
        enabledItems = getEnabledItems(contextMenuParams);
        Assert.assertThat(
                enabledItems, Matchers.containsInAnyOrder(ChromeContextMenuItem.COPY_LINK_ADDRESS));
    }

    private List<ContextMenuItem> getEnabledItems(ContextMenuParams params) {
        List<Pair<Integer, List<ContextMenuItem>>> contextMenuState =
                mPopulator.buildContextMenu(null, RuntimeEnvironment.application, params);

        List<ContextMenuItem> enabledItems = new ArrayList<>();
        for (int i = 0; i < contextMenuState.size(); i++) {
            enabledItems.addAll(contextMenuState.get(i).second);
        }
        return enabledItems;
    }

    private static ContextMenuParams createLinkContextParams() {
        return new ContextMenuParams(0, PAGE_URL, LINK_URL, LINK_TEXT, "", "", "", false, null,
                false, 0, 0, MenuSourceType.MENU_SOURCE_TOUCH);
    }

    private static ContextMenuParams createImageContextParams() {
        return new ContextMenuParams(WebContextMenuMediaType.IMAGE, PAGE_URL, "", "",
                IMAGE_SRC_URL, IMAGE_TITLE_TEXT, "", false, null, true, 0, 0,
                MenuSourceType.MENU_SOURCE_TOUCH);
    }

    private static ContextMenuParams createImageLinkContextParams() {
        return new ContextMenuParams(WebContextMenuMediaType.IMAGE, PAGE_URL, PAGE_URL,
                LINK_URL, IMAGE_SRC_URL, IMAGE_TITLE_TEXT, "", false, null, true, 0, 0,
                MenuSourceType.MENU_SOURCE_TOUCH);
    }

    /**
     * Shadow for UrlUtilities
     */
    @Implements(UrlUtilities.class)
    public static class ShadowUrlUtilities {
        @Implementation
        public static boolean isDownloadableScheme(String uri) {
            return true;
        }

        @Implementation
        public static boolean isAcceptedScheme(String uri) {
            return true;
        }
    }
}
