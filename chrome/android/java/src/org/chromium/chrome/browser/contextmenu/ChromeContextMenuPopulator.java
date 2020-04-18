// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.contextmenu;

import android.content.Context;
import android.net.MailTo;
import android.support.annotation.IntDef;
import android.support.annotation.StringRes;
import android.text.TextUtils;
import android.util.Pair;
import android.view.ContextMenu;
import android.webkit.MimeTypeMap;

import org.chromium.base.CollectionUtil;
import org.chromium.base.library_loader.LibraryProcessType;
import org.chromium.base.metrics.RecordHistogram;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.ChromeFeatureList;
import org.chromium.chrome.browser.firstrun.FirstRunStatus;
import org.chromium.chrome.browser.locale.LocaleManager;
import org.chromium.chrome.browser.preferences.ChromePreferenceManager;
import org.chromium.chrome.browser.preferences.datareduction.DataReductionProxyUma;
import org.chromium.chrome.browser.search_engines.TemplateUrlService;
import org.chromium.chrome.browser.share.ShareHelper;
import org.chromium.chrome.browser.share.ShareParams;
import org.chromium.chrome.browser.util.UrlUtilities;
import org.chromium.components.url_formatter.UrlFormatter;
import org.chromium.content.browser.BrowserStartupController;
import org.chromium.content_public.common.ContentUrlConstants;

import java.lang.annotation.Retention;
import java.lang.annotation.RetentionPolicy;
import java.net.URI;
import java.net.URISyntaxException;
import java.util.ArrayList;
import java.util.Collections;
import java.util.HashSet;
import java.util.List;
import java.util.Set;

/**
 * A {@link ContextMenuPopulator} used for showing the default Chrome context menu.
 */
public class ChromeContextMenuPopulator implements ContextMenuPopulator {
    private static final String TAG = "CCMenuPopulator";
    private static final ShareContextMenuItem SHARE_IMAGE =
            new ShareContextMenuItem(R.drawable.ic_share_white_24dp,
                    R.string.contextmenu_share_image, R.id.contextmenu_share_image, false);
    private static final ShareContextMenuItem SHARE_LINK =
            new ShareContextMenuItem(R.drawable.ic_share_white_24dp,
                    R.string.contextmenu_share_link, R.id.contextmenu_share_link, true);

    /**
     * Defines the Groups of each Context Menu Item
     */
    @Retention(RetentionPolicy.SOURCE)
    @IntDef({LINK, IMAGE, VIDEO})
    public @interface ContextMenuGroup {}

    public static final int LINK = 0;
    public static final int IMAGE = 1;
    public static final int VIDEO = 2;

    /**
     * Defines the context menu modes
     */
    @Retention(RetentionPolicy.SOURCE)
    @IntDef({
            NORMAL_MODE, /* Default mode */
            CUSTOM_TAB_MODE, /* Custom tab mode */
            WEB_APP_MODE /* Full screen mode */
    })
    public @interface ContextMenuMode {}

    public static final int NORMAL_MODE = 0;
    public static final int CUSTOM_TAB_MODE = 1;
    public static final int WEB_APP_MODE = 2;

    // Items that are included in all context menus.
    private static final Set<? extends ContextMenuItem> BASE_WHITELIST =
            Collections.unmodifiableSet(CollectionUtil.newHashSet(
                    ChromeContextMenuItem.COPY_LINK_ADDRESS, ChromeContextMenuItem.CALL,
                    ChromeContextMenuItem.SEND_MESSAGE, ChromeContextMenuItem.ADD_TO_CONTACTS,
                    ChromeContextMenuItem.COPY, ChromeContextMenuItem.COPY_LINK_TEXT,
                    ChromeContextMenuItem.LOAD_ORIGINAL_IMAGE, ChromeContextMenuItem.SAVE_LINK_AS,
                    ChromeContextMenuItem.SAVE_IMAGE, SHARE_IMAGE, ChromeContextMenuItem.SAVE_VIDEO,
                    SHARE_LINK));

    // Items that are included for normal Chrome browser mode.
    private static final Set<? extends ContextMenuItem> NORMAL_MODE_WHITELIST =
            Collections.unmodifiableSet(CollectionUtil.newHashSet(
                    ChromeContextMenuItem.OPEN_IN_NEW_TAB,
                    ChromeContextMenuItem.OPEN_IN_OTHER_WINDOW,
                    ChromeContextMenuItem.OPEN_IN_INCOGNITO_TAB, ChromeContextMenuItem.SAVE_LINK_AS,
                    ChromeContextMenuItem.OPEN_IMAGE_IN_NEW_TAB,
                    ChromeContextMenuItem.SEARCH_BY_IMAGE));

    // Additional items for custom tabs mode.
    private static final Set<? extends ContextMenuItem> CUSTOM_TAB_MODE_WHITELIST =
            Collections.unmodifiableSet(CollectionUtil.newHashSet(ChromeContextMenuItem.OPEN_IMAGE,
                    ChromeContextMenuItem.SEARCH_BY_IMAGE,
                    ChromeContextMenuItem.OPEN_IN_NEW_CHROME_TAB,
                    ChromeContextMenuItem.OPEN_IN_CHROME_INCOGNITO_TAB,
                    ChromeContextMenuItem.OPEN_IN_BROWSER_ID));

    // Additional items for fullscreen tabs mode.
    private static final Set<? extends ContextMenuItem> WEB_APP_MODE_WHITELIST =
            Collections.unmodifiableSet(
                    CollectionUtil.newHashSet(ChromeContextMenuItem.OPEN_IN_CHROME));

    // The order of the items within each lists determines the order of the context menu.
    private static final List<? extends ContextMenuItem> CUSTOM_TAB_GROUP =
            Collections.unmodifiableList(
                    CollectionUtil.newArrayList(ChromeContextMenuItem.OPEN_IN_NEW_CHROME_TAB,
                            ChromeContextMenuItem.OPEN_IN_CHROME_INCOGNITO_TAB,
                            ChromeContextMenuItem.OPEN_IN_BROWSER_ID));

    private static final List<? extends ContextMenuItem> LINK_GROUP = Collections.unmodifiableList(
            CollectionUtil.newArrayList(ChromeContextMenuItem.OPEN_IN_OTHER_WINDOW,
                    ChromeContextMenuItem.OPEN_IN_NEW_TAB,
                    ChromeContextMenuItem.OPEN_IN_INCOGNITO_TAB,
                    ChromeContextMenuItem.COPY_LINK_ADDRESS, ChromeContextMenuItem.COPY_LINK_TEXT,
                    ChromeContextMenuItem.SAVE_LINK_AS, SHARE_LINK));

    private static final List<? extends ContextMenuItem> IMAGE_GROUP =
            Collections.unmodifiableList(CollectionUtil.newArrayList(
                    ChromeContextMenuItem.LOAD_ORIGINAL_IMAGE, ChromeContextMenuItem.OPEN_IMAGE,
                    ChromeContextMenuItem.OPEN_IMAGE_IN_NEW_TAB, ChromeContextMenuItem.SAVE_IMAGE,
                    ChromeContextMenuItem.SEARCH_BY_IMAGE, SHARE_IMAGE));

    private static final List<? extends ContextMenuItem> MESSAGE_GROUP =
            Collections.unmodifiableList(CollectionUtil.newArrayList(ChromeContextMenuItem.CALL,
                    ChromeContextMenuItem.SEND_MESSAGE, ChromeContextMenuItem.ADD_TO_CONTACTS,
                    ChromeContextMenuItem.COPY));

    private static final List<? extends ContextMenuItem> VIDEO_GROUP = Collections.unmodifiableList(
            CollectionUtil.newArrayList(ChromeContextMenuItem.SAVE_VIDEO));

    private static final List<? extends ContextMenuItem> OTHER_GROUP = Collections.unmodifiableList(
            CollectionUtil.newArrayList(ChromeContextMenuItem.OPEN_IN_CHROME));

    private final ContextMenuItemDelegate mDelegate;
    private final int mMode;

    static class ContextMenuUma {
        // Note: these values must match the ContextMenuOption enum in histograms.xml.
        static final int ACTION_OPEN_IN_NEW_TAB = 0;
        static final int ACTION_OPEN_IN_INCOGNITO_TAB = 1;
        static final int ACTION_COPY_LINK_ADDRESS = 2;
        static final int ACTION_COPY_EMAIL_ADDRESS = 3;
        static final int ACTION_COPY_LINK_TEXT = 4;
        static final int ACTION_SAVE_LINK = 5;
        static final int ACTION_SAVE_IMAGE = 6;
        static final int ACTION_OPEN_IMAGE = 7;
        static final int ACTION_OPEN_IMAGE_IN_NEW_TAB = 8;
        static final int ACTION_SEARCH_BY_IMAGE = 11;
        static final int ACTION_LOAD_ORIGINAL_IMAGE = 13;
        static final int ACTION_SAVE_VIDEO = 14;
        static final int ACTION_SHARE_IMAGE = 19;
        static final int ACTION_OPEN_IN_OTHER_WINDOW = 20;
        static final int ACTION_SEND_EMAIL = 23;
        static final int ACTION_ADD_TO_CONTACTS = 24;
        static final int ACTION_CALL = 30;
        static final int ACTION_SEND_TEXT_MESSAGE = 31;
        static final int ACTION_COPY_PHONE_NUMBER = 32;
        static final int ACTION_OPEN_IN_NEW_CHROME_TAB = 33;
        static final int ACTION_OPEN_IN_CHROME_INCOGNITO_TAB = 34;
        static final int ACTION_OPEN_IN_BROWSER = 35;
        static final int ACTION_OPEN_IN_CHROME = 36;
        static final int ACTION_SHARE_LINK = 37;
        static final int NUM_ACTIONS = 38;

        // Note: these values must match the ContextMenuSaveLinkType enum in histograms.xml.
        // Only add new values at the end, right before NUM_TYPES. We depend on these specific
        // values in UMA histograms.
        static final int TYPE_UNKNOWN = 0;
        static final int TYPE_TEXT = 1;
        static final int TYPE_IMAGE = 2;
        static final int TYPE_AUDIO = 3;
        static final int TYPE_VIDEO = 4;
        static final int TYPE_PDF = 5;
        static final int NUM_TYPES = 6;

        // Note: these values must match the ContextMenuSaveImage enum in histograms.xml.
        // Only add new values at the end, right before NUM_SAVE_IMAGE_TYPES.
        static final int TYPE_SAVE_IMAGE_LOADED = 0;
        static final int TYPE_SAVE_IMAGE_FETCHED_LOFI = 1;
        static final int TYPE_SAVE_IMAGE_NOT_DOWNLOADABLE = 2;
        static final int TYPE_SAVE_IMAGE_DISABLED_AND_IS_NOT_IMAGE_PARAM = 3;
        static final int TYPE_SAVE_IMAGE_DISABLED_AND_IS_IMAGE_PARAM = 4;
        static final int TYPE_SAVE_IMAGE_SHOWN = 5;
        static final int NUM_SAVE_IMAGE_TYPES = 6;

        /**
         * Records a histogram entry when the user selects an item from a context menu.
         * @param params The ContextMenuParams describing the current context menu.
         * @param action The action that the user selected (e.g. ACTION_SAVE_IMAGE).
         */
        static void record(ContextMenuParams params, int action) {
            assert action >= 0;
            assert action < NUM_ACTIONS;
            String histogramName;
            if (params.isVideo()) {
                histogramName = "ContextMenu.SelectedOption.Video";
            } else if (params.isImage()) {
                histogramName = params.isAnchor()
                        ? "ContextMenu.SelectedOption.ImageLink"
                        : "ContextMenu.SelectedOption.Image";
            } else {
                assert params.isAnchor();
                histogramName = "ContextMenu.SelectedOption.Link";
            }
            RecordHistogram.recordEnumeratedHistogram(histogramName, action, NUM_ACTIONS);
        }

        /**
         * Records the content types when user downloads the file by long pressing the
         * save link context menu option.
         */
        static void recordSaveLinkTypes(String url) {
            String extension = MimeTypeMap.getFileExtensionFromUrl(url);
            int mimeType = TYPE_UNKNOWN;
            if (extension != null) {
                String type = MimeTypeMap.getSingleton().getMimeTypeFromExtension(extension);
                if (type != null) {
                    if (type.startsWith("text")) {
                        mimeType = TYPE_TEXT;
                    } else if (type.startsWith("image")) {
                        mimeType = TYPE_IMAGE;
                    } else if (type.startsWith("audio")) {
                        mimeType = TYPE_AUDIO;
                    } else if (type.startsWith("video")) {
                        mimeType = TYPE_VIDEO;
                    } else if (type.equals("application/pdf")) {
                        mimeType = TYPE_PDF;
                    }
                }
            }
            RecordHistogram.recordEnumeratedHistogram(
                    "ContextMenu.SaveLinkType", mimeType, NUM_TYPES);
        }

        /**
         * Helper method to record MobileDownload.ContextMenu.SaveImage UMA
         * @param type Type to record
         */
        static void recordSaveImageUma(int type) {
            RecordHistogram.recordEnumeratedHistogram(
                    "MobileDownload.ContextMenu.SaveImage", type, NUM_SAVE_IMAGE_TYPES);
        }
    }

    /**
     * Builds a {@link ChromeContextMenuPopulator}.
     * @param delegate The {@link ContextMenuItemDelegate} that will be notified with actions
     *                 to perform when menu items are selected.
     * @param mode Defines the context menu mode
     */
    public ChromeContextMenuPopulator(ContextMenuItemDelegate delegate, @ContextMenuMode int mode) {
        mDelegate = delegate;
        mMode = mode;
    }

    @Override
    public void onDestroy() {
        mDelegate.onDestroy();
    }

    /**
     * Gets the link of the item or the alternate text of an image.
     * @return A string with either the link or with the alternate text.
     */
    public static String createHeaderText(ContextMenuParams params) {
        if (!isEmptyUrl(params.getLinkUrl())) {
            // The context menu can be created without native library
            // being loaded. Only use native URL formatting methods
            // if the native libraries have been loaded.
            if (BrowserStartupController.get(LibraryProcessType.PROCESS_BROWSER)
                            .isStartupSuccessfullyCompleted()) {
                return UrlFormatter.formatUrlForDisplayOmitHTTPScheme(params.getLinkUrl());
            } else {
                return params.getLinkUrl();
            }
        } else if (!TextUtils.isEmpty(params.getTitleText())) {
            return params.getTitleText();
        }
        return "";
    }

    @Override
    public List<Pair<Integer, List<ContextMenuItem>>> buildContextMenu(
            ContextMenu menu, Context context, ContextMenuParams params) {
        // Add all items in a group
        Set<ContextMenuItem> supportedOptions = new HashSet<>();
        if (FirstRunStatus.getFirstRunFlowComplete()) {
            supportedOptions.addAll(BASE_WHITELIST);
            if (mMode == WEB_APP_MODE) {
                supportedOptions.addAll(WEB_APP_MODE_WHITELIST);
            } else if (mMode == CUSTOM_TAB_MODE) {
                supportedOptions.addAll(CUSTOM_TAB_MODE_WHITELIST);
            } else {
                supportedOptions.addAll(NORMAL_MODE_WHITELIST);
            }
        } else {
            supportedOptions.add(ChromeContextMenuItem.COPY_LINK_ADDRESS);
            supportedOptions.add(ChromeContextMenuItem.COPY_LINK_TEXT);
            supportedOptions.add(ChromeContextMenuItem.COPY);
        }

        Set<ContextMenuItem> disabledOptions = getDisabledOptions(params);
        // Split the items into their respective groups.
        List<Pair<Integer, List<ContextMenuItem>>> groupedItems = new ArrayList<>();
        if (params.isAnchor()) {
            populateItemGroup(LINK, R.string.contextmenu_link_title, groupedItems, supportedOptions,
                    disabledOptions);
        }
        if (params.isImage()) {
            populateItemGroup(IMAGE, R.string.contextmenu_image_title, groupedItems,
                    supportedOptions, disabledOptions);
        }
        if (params.isVideo()) {
            populateItemGroup(VIDEO, R.string.contextmenu_video_title, groupedItems,
                    supportedOptions, disabledOptions);
        }

        // If there are no groups there still needs to be a way to add items from the OTHER_GROUP
        // and CUSTOM_TAB_GROUP.
        if (groupedItems.isEmpty()) {
            int titleResId = R.string.contextmenu_link_title;

            if (params.isVideo()) {
                titleResId = R.string.contextmenu_video_title;
            } else if (params.isImage()) {
                titleResId = R.string.contextmenu_image_title;
            }
            groupedItems.add(new Pair<Integer, List<ContextMenuItem>>(
                    titleResId, new ArrayList<ContextMenuItem>()));
        }

        // These items don't belong to any official group so they are added to a possible visible
        // list.
        addValidItems(groupedItems.get(groupedItems.size() - 1).second, OTHER_GROUP,
                supportedOptions, disabledOptions);
        if (mMode == CUSTOM_TAB_MODE) {
            addValidItemsToFront(groupedItems.get(0).second, CUSTOM_TAB_GROUP, supportedOptions,
                    disabledOptions);
        }

        // If there are no items from the extra items within OTHER_GROUP and CUSTOM_TAB_GROUP, then
        // it's removed since there is nothing to show at all.
        if (groupedItems.get(0).second.isEmpty()) {
            groupedItems.remove(0);
        }

        if (!groupedItems.isEmpty()) {
            boolean hasSaveImage = false;
            for (int i = 0; i < groupedItems.size(); ++i) {
                Pair<Integer, List<ContextMenuItem>> menuList = groupedItems.get(i);
                if (menuList.second != null
                        && menuList.second.contains(ChromeContextMenuItem.SAVE_IMAGE)) {
                    hasSaveImage = true;
                    break;
                }
            }

            if (BrowserStartupController.get(LibraryProcessType.PROCESS_BROWSER)
                            .isStartupSuccessfullyCompleted()) {
                if (!hasSaveImage) {
                    ContextMenuUma.recordSaveImageUma(params.isImage()
                                    ? ContextMenuUma.TYPE_SAVE_IMAGE_DISABLED_AND_IS_IMAGE_PARAM
                                    : ContextMenuUma
                                              .TYPE_SAVE_IMAGE_DISABLED_AND_IS_NOT_IMAGE_PARAM);
                } else {
                    ContextMenuUma.recordSaveImageUma(ContextMenuUma.TYPE_SAVE_IMAGE_SHOWN);
                }
            }
        }

        return groupedItems;
    }

    private void populateItemGroup(@ContextMenuGroup int contextMenuType, @StringRes int titleResId,
            List<Pair<Integer, List<ContextMenuItem>>> itemGroups,
            Set<ContextMenuItem> supportedOptions, Set<ContextMenuItem> disabledOptions) {
        List<ContextMenuItem> items = new ArrayList<>();
        switch (contextMenuType) {
            case LINK:
                addValidItems(items, LINK_GROUP, supportedOptions, disabledOptions);
                addValidItems(items, MESSAGE_GROUP, supportedOptions, disabledOptions);
                break;
            case IMAGE:
                addValidItems(items, IMAGE_GROUP, supportedOptions, disabledOptions);
                break;
            case VIDEO:
                addValidItems(items, VIDEO_GROUP, supportedOptions, disabledOptions);
                break;
            default:
                return;
        }

        if (items.isEmpty()) return;

        itemGroups.add(new Pair<>(titleResId, items));
    }

    private static void addValidItems(List<ContextMenuItem> validItems,
            List<? extends ContextMenuItem> allItems, Set<ContextMenuItem> supportedOptions,
            Set<ContextMenuItem> disabledOptions) {
        for (int i = 0; i < allItems.size(); i++) {
            ContextMenuItem item = allItems.get(i);
            if (supportedOptions.contains(item) && !disabledOptions.contains(item)) {
                assert !validItems.contains(item);
                validItems.add(item);
            }
        }
    }

    /**
     *  This works in the same way as {@link #addValidItems(List, List, Set, Set)} however the list
     *  given for example (a, b, c) will be added to list d, e f, as a, b, c, d, e, f not
     *  c, b, a, d, e, f.
     */
    private static void addValidItemsToFront(List<ContextMenuItem> validItems,
            List<? extends ContextMenuItem> allItems, Set<ContextMenuItem> supportedOptions,
            Set<ContextMenuItem> disabledOptions) {
        for (int i = allItems.size() - 1; i >= 0; i--) {
            ContextMenuItem item = allItems.get(i);
            if (supportedOptions.contains(item) && !disabledOptions.contains(item)) {
                assert !validItems.contains(item);
                validItems.add(0, item);
            }
        }
    }

    /**
     * Given a set of params. It creates a list of items that should not be accessible in specific
     * instances.
     * @param params The parameters used to create a list of items that should not be allowed.
     */
    private Set<ContextMenuItem> getDisabledOptions(ContextMenuParams params) {
        Set<ContextMenuItem> disabledOptions = new HashSet<>();
        if (!params.isAnchor()) {
            disabledOptions.addAll(LINK_GROUP);
        }
        if (!params.isImage()) {
            disabledOptions.addAll(IMAGE_GROUP);
        }
        if (!params.isVideo()) {
            disabledOptions.addAll(VIDEO_GROUP);
        }
        if (!MailTo.isMailTo(params.getLinkUrl())
                && !UrlUtilities.isTelScheme(params.getLinkUrl())) {
            disabledOptions.addAll(MESSAGE_GROUP);
        }

        if (params.isAnchor() && !mDelegate.isOpenInOtherWindowSupported()) {
            disabledOptions.add(ChromeContextMenuItem.OPEN_IN_OTHER_WINDOW);
        }

        if (mDelegate.isIncognito() || !mDelegate.isIncognitoSupported()) {
            disabledOptions.add(ChromeContextMenuItem.OPEN_IN_INCOGNITO_TAB);
        }

        if (params.getLinkText().trim().isEmpty() || params.isImage()) {
            disabledOptions.add(ChromeContextMenuItem.COPY_LINK_TEXT);
        }

        if (isEmptyUrl(params.getUrl()) || !UrlUtilities.isAcceptedScheme(params.getUrl())) {
            disabledOptions.add(ChromeContextMenuItem.OPEN_IN_OTHER_WINDOW);
            disabledOptions.add(ChromeContextMenuItem.OPEN_IN_NEW_TAB);
            disabledOptions.add(ChromeContextMenuItem.OPEN_IN_INCOGNITO_TAB);
        }

        if (MailTo.isMailTo(params.getLinkUrl())) {
            disabledOptions.add(ChromeContextMenuItem.COPY_LINK_TEXT);
            disabledOptions.add(ChromeContextMenuItem.COPY_LINK_ADDRESS);
            if (!mDelegate.supportsSendEmailMessage()) {
                disabledOptions.add(ChromeContextMenuItem.SEND_MESSAGE);
            }
            if (TextUtils.isEmpty(MailTo.parse(params.getLinkUrl()).getTo())
                    || !mDelegate.supportsAddToContacts()) {
                disabledOptions.add(ChromeContextMenuItem.ADD_TO_CONTACTS);
            }
            disabledOptions.add(ChromeContextMenuItem.CALL);
        } else if (UrlUtilities.isTelScheme(params.getLinkUrl())) {
            disabledOptions.add(ChromeContextMenuItem.COPY_LINK_TEXT);
            disabledOptions.add(ChromeContextMenuItem.COPY_LINK_ADDRESS);
            if (!mDelegate.supportsCall()) {
                disabledOptions.add(ChromeContextMenuItem.CALL);
            }
            if (!mDelegate.supportsSendTextMessage()) {
                disabledOptions.add(ChromeContextMenuItem.SEND_MESSAGE);
            }
            if (!mDelegate.supportsAddToContacts()) {
                disabledOptions.add(ChromeContextMenuItem.ADD_TO_CONTACTS);
            }
        }

        if (!UrlUtilities.isDownloadableScheme(params.getLinkUrl())) {
            disabledOptions.add(ChromeContextMenuItem.SAVE_LINK_AS);
        }

        boolean isSrcDownloadableScheme = UrlUtilities.isDownloadableScheme(params.getSrcUrl());
        if (params.isVideo()) {
            boolean saveableAndDownloadable = params.canSaveMedia() && isSrcDownloadableScheme;
            if (!saveableAndDownloadable) {
                disabledOptions.add(ChromeContextMenuItem.SAVE_VIDEO);
            }
        } else if (params.isImage() && params.imageWasFetchedLoFi()) {
            DataReductionProxyUma.previewsLoFiContextMenuAction(
                    DataReductionProxyUma.ACTION_LOFI_LOAD_IMAGE_CONTEXT_MENU_SHOWN);
            // All image context menu items other than "Load image," "Open original image in
            // new tab," and "Copy image URL" should be disabled on Lo-Fi images.
            disabledOptions.add(ChromeContextMenuItem.SAVE_IMAGE);
            disabledOptions.add(ChromeContextMenuItem.OPEN_IMAGE);
            disabledOptions.add(ChromeContextMenuItem.SEARCH_BY_IMAGE);
            disabledOptions.add(SHARE_IMAGE);
            recordSaveImageContextMenuResult(true, isSrcDownloadableScheme);
        } else if (params.isImage() && !params.imageWasFetchedLoFi()) {
            disabledOptions.add(ChromeContextMenuItem.LOAD_ORIGINAL_IMAGE);

            if (!isSrcDownloadableScheme) {
                disabledOptions.add(ChromeContextMenuItem.SAVE_IMAGE);
            }
            recordSaveImageContextMenuResult(false, isSrcDownloadableScheme);
            // Avoid showing open image option for same image which is already opened.
            if (mDelegate.getPageUrl().equals(params.getSrcUrl())) {
                disabledOptions.add(ChromeContextMenuItem.OPEN_IMAGE);
            }
            final TemplateUrlService templateUrlServiceInstance = getTemplateUrlService();
            final boolean isSearchByImageAvailable = isSrcDownloadableScheme
                    && templateUrlServiceInstance.isLoaded()
                    && templateUrlServiceInstance.isSearchByImageAvailable()
                    && templateUrlServiceInstance.getDefaultSearchEngineTemplateUrl() != null
                    && !LocaleManager.getInstance().needToCheckForSearchEnginePromo();

            if (!isSearchByImageAvailable) {
                disabledOptions.add(ChromeContextMenuItem.SEARCH_BY_IMAGE);
            }
        }

        if (mMode == CUSTOM_TAB_MODE) {
            try {
                URI uri = new URI(params.getUrl());
                if (UrlUtilities.isInternalScheme(uri) || isEmptyUrl(params.getUrl())) {
                    disabledOptions.add(ChromeContextMenuItem.OPEN_IN_NEW_CHROME_TAB);
                    disabledOptions.add(ChromeContextMenuItem.OPEN_IN_CHROME_INCOGNITO_TAB);
                    disabledOptions.add(ChromeContextMenuItem.OPEN_IN_BROWSER_ID);
                } else if (ChromePreferenceManager.getInstance().getCachedChromeDefaultBrowser()) {
                    disabledOptions.add(ChromeContextMenuItem.OPEN_IN_BROWSER_ID);
                    if (!mDelegate.isIncognitoSupported()) {
                        disabledOptions.add(ChromeContextMenuItem.OPEN_IN_CHROME_INCOGNITO_TAB);
                    }
                } else {
                    disabledOptions.add(ChromeContextMenuItem.OPEN_IN_NEW_CHROME_TAB);
                    disabledOptions.add(ChromeContextMenuItem.OPEN_IN_CHROME_INCOGNITO_TAB);
                }
            } catch (URISyntaxException e) {
                return disabledOptions;
            }
        }

        if (ChromeFeatureList.isEnabled(ChromeFeatureList.CUSTOM_CONTEXT_MENU)) {
            disabledOptions.add(ChromeContextMenuItem.COPY_LINK_TEXT);
        }

        return disabledOptions;
    }

    @Override
    public boolean onItemSelected(ContextMenuHelper helper, ContextMenuParams params, int itemId) {
        if (itemId == R.id.contextmenu_open_in_other_window) {
            ContextMenuUma.record(params, ContextMenuUma.ACTION_OPEN_IN_OTHER_WINDOW);
            mDelegate.onOpenInOtherWindow(params.getUrl(), params.getReferrer());
        } else if (itemId == R.id.contextmenu_open_in_new_tab) {
            ContextMenuUma.record(params, ContextMenuUma.ACTION_OPEN_IN_NEW_TAB);
            mDelegate.onOpenInNewTab(params.getUrl(), params.getReferrer());
        } else if (itemId == R.id.contextmenu_open_in_incognito_tab) {
            ContextMenuUma.record(params, ContextMenuUma.ACTION_OPEN_IN_INCOGNITO_TAB);
            mDelegate.onOpenInNewIncognitoTab(params.getUrl());
        } else if (itemId == R.id.contextmenu_open_image) {
            ContextMenuUma.record(params, ContextMenuUma.ACTION_OPEN_IMAGE);
            mDelegate.onOpenImageUrl(params.getSrcUrl(), params.getReferrer());
        } else if (itemId == R.id.contextmenu_open_image_in_new_tab) {
            ContextMenuUma.record(params, ContextMenuUma.ACTION_OPEN_IMAGE_IN_NEW_TAB);
            mDelegate.onOpenImageInNewTab(params.getSrcUrl(), params.getReferrer());
        } else if (itemId == R.id.contextmenu_load_original_image) {
            ContextMenuUma.record(params, ContextMenuUma.ACTION_LOAD_ORIGINAL_IMAGE);
            DataReductionProxyUma.previewsLoFiContextMenuAction(
                    DataReductionProxyUma.ACTION_LOFI_LOAD_IMAGE_CONTEXT_MENU_CLICKED);
            if (!mDelegate.wasLoadOriginalImageRequestedForPageLoad()) {
                DataReductionProxyUma.previewsLoFiContextMenuAction(
                        DataReductionProxyUma.ACTION_LOFI_LOAD_IMAGE_CONTEXT_MENU_CLICKED_ON_PAGE);
            }
            mDelegate.onLoadOriginalImage();
        } else if (itemId == R.id.contextmenu_copy_link_address) {
            ContextMenuUma.record(params, ContextMenuUma.ACTION_COPY_LINK_ADDRESS);
            mDelegate.onSaveToClipboard(params.getUnfilteredLinkUrl(),
                    ContextMenuItemDelegate.CLIPBOARD_TYPE_LINK_URL);
        } else if (itemId == R.id.contextmenu_call) {
            ContextMenuUma.record(params, ContextMenuUma.ACTION_CALL);
            mDelegate.onCall(params.getLinkUrl());
        } else if (itemId == R.id.contextmenu_send_message) {
            if (MailTo.isMailTo(params.getLinkUrl())) {
                ContextMenuUma.record(params, ContextMenuUma.ACTION_SEND_EMAIL);
                mDelegate.onSendEmailMessage(params.getLinkUrl());
            } else if (UrlUtilities.isTelScheme(params.getLinkUrl())) {
                ContextMenuUma.record(params, ContextMenuUma.ACTION_SEND_TEXT_MESSAGE);
                mDelegate.onSendTextMessage(params.getLinkUrl());
            }
        } else if (itemId == R.id.contextmenu_add_to_contacts) {
            ContextMenuUma.record(params, ContextMenuUma.ACTION_ADD_TO_CONTACTS);
            mDelegate.onAddToContacts(params.getLinkUrl());
        } else if (itemId == R.id.contextmenu_copy) {
            if (MailTo.isMailTo(params.getLinkUrl())) {
                ContextMenuUma.record(params, ContextMenuUma.ACTION_COPY_EMAIL_ADDRESS);
                mDelegate.onSaveToClipboard(MailTo.parse(params.getLinkUrl()).getTo(),
                        ContextMenuItemDelegate.CLIPBOARD_TYPE_LINK_URL);
            } else if (UrlUtilities.isTelScheme(params.getLinkUrl())) {
                ContextMenuUma.record(params, ContextMenuUma.ACTION_COPY_PHONE_NUMBER);
                mDelegate.onSaveToClipboard(UrlUtilities.getTelNumber(params.getLinkUrl()),
                        ContextMenuItemDelegate.CLIPBOARD_TYPE_LINK_URL);
            }
        } else if (itemId == R.id.contextmenu_copy_link_text) {
            ContextMenuUma.record(params, ContextMenuUma.ACTION_COPY_LINK_TEXT);
            mDelegate.onSaveToClipboard(
                    params.getLinkText(), ContextMenuItemDelegate.CLIPBOARD_TYPE_LINK_TEXT);
        } else if (itemId == R.id.contextmenu_save_image) {
            ContextMenuUma.record(params, ContextMenuUma.ACTION_SAVE_IMAGE);
            if (mDelegate.startDownload(params.getSrcUrl(), false)) {
                helper.startContextMenuDownload(
                        false, mDelegate.isDataReductionProxyEnabledForURL(params.getSrcUrl()));
            }
        } else if (itemId == R.id.contextmenu_save_video) {
            ContextMenuUma.record(params, ContextMenuUma.ACTION_SAVE_VIDEO);
            if (mDelegate.startDownload(params.getSrcUrl(), false)) {
                helper.startContextMenuDownload(false, false);
            }
        } else if (itemId == R.id.contextmenu_save_link_as) {
            ContextMenuUma.record(params, ContextMenuUma.ACTION_SAVE_LINK);
            String url = params.getUnfilteredLinkUrl();
            if (mDelegate.startDownload(url, true)) {
                ContextMenuUma.recordSaveLinkTypes(url);
                helper.startContextMenuDownload(true, false);
            }
        } else if (itemId == R.id.contextmenu_share_link) {
            ContextMenuUma.record(params, ContextMenuUma.ACTION_SHARE_LINK);
            ShareParams linkShareParams =
                    new ShareParams.Builder(helper.getActivity(), params.getUrl(), params.getUrl())
                            .setShareDirectly(false)
                            .setSaveLastUsed(true)
                            .build();
            ShareHelper.share(linkShareParams);
        } else if (itemId == R.id.contextmenu_search_by_image) {
            ContextMenuUma.record(params, ContextMenuUma.ACTION_SEARCH_BY_IMAGE);
            helper.searchForImage();
        } else if (itemId == R.id.contextmenu_share_image) {
            ContextMenuUma.record(params, ContextMenuUma.ACTION_SHARE_IMAGE);
            helper.shareImage();
        } else if (itemId == R.id.contextmenu_open_in_chrome) {
            ContextMenuUma.record(params, ContextMenuUma.ACTION_OPEN_IN_CHROME);
            mDelegate.onOpenInChrome(params.getUrl(), params.getPageUrl());
        } else if (itemId == R.id.contextmenu_open_in_new_chrome_tab) {
            ContextMenuUma.record(params, ContextMenuUma.ACTION_OPEN_IN_NEW_CHROME_TAB);
            mDelegate.onOpenInNewChromeTabFromCCT(params.getUrl(), false);
        } else if (itemId == R.id.contextmenu_open_in_chrome_incognito_tab) {
            ContextMenuUma.record(params, ContextMenuUma.ACTION_OPEN_IN_CHROME_INCOGNITO_TAB);
            mDelegate.onOpenInNewChromeTabFromCCT(params.getUrl(), true);
        } else if (itemId == R.id.contextmenu_open_in_browser_id) {
            ContextMenuUma.record(params, ContextMenuUma.ACTION_OPEN_IN_BROWSER);
            mDelegate.onOpenInDefaultBrowser(params.getUrl());
        } else {
            assert false;
        }

        return true;
    }

    /**
     * @return The service that handles TemplateUrls.
     */
    protected TemplateUrlService getTemplateUrlService() {
        return TemplateUrlService.getInstance();
    }

    /**
     * Checks whether a url is empty or blank.
     * @param url The url need to be checked.
     * @return True if the url is empty or "about:blank".
     */
    private static boolean isEmptyUrl(String url) {
        return TextUtils.isEmpty(url) || url.equals(ContentUrlConstants.ABOUT_BLANK_DISPLAY_URL);
    }

    /**
     * Record the UMA related to save image context menu option.
     * @param wasFetchedLoFi The image was fectched LoFi.
     * @param isDownloadableScheme The image is downloadable.
     */
    private void recordSaveImageContextMenuResult(
            boolean wasFetchedLoFi, boolean isDownloadableScheme) {
        if (!BrowserStartupController.get(LibraryProcessType.PROCESS_BROWSER)
                        .isStartupSuccessfullyCompleted()) {
            return;
        }

        ContextMenuUma.recordSaveImageUma(ContextMenuUma.TYPE_SAVE_IMAGE_LOADED);

        if (wasFetchedLoFi) {
            ContextMenuUma.recordSaveImageUma(ContextMenuUma.TYPE_SAVE_IMAGE_FETCHED_LOFI);
            return;
        }

        if (!isDownloadableScheme) {
            ContextMenuUma.recordSaveImageUma(ContextMenuUma.TYPE_SAVE_IMAGE_NOT_DOWNLOADABLE);
        }
    }
}
