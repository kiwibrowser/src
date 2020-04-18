// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.toolbar;

import android.content.Context;
import android.content.res.ColorStateList;
import android.content.res.Resources;
import android.support.annotation.DrawableRes;
import android.support.annotation.Nullable;
import android.text.TextUtils;

import org.chromium.base.ApiCompatibilityUtils;
import org.chromium.base.ContextUtils;
import org.chromium.base.VisibleForTesting;
import org.chromium.base.annotations.CalledByNative;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.ChromeFeatureList;
import org.chromium.chrome.browser.ChromeTabbedActivity;
import org.chromium.chrome.browser.UrlConstants;
import org.chromium.chrome.browser.dom_distiller.DomDistillerServiceFactory;
import org.chromium.chrome.browser.dom_distiller.DomDistillerTabUtils;
import org.chromium.chrome.browser.locale.LocaleManager;
import org.chromium.chrome.browser.ntp.NativePageFactory;
import org.chromium.chrome.browser.ntp.NewTabPage;
import org.chromium.chrome.browser.offlinepages.OfflinePageUtils;
import org.chromium.chrome.browser.omnibox.AutocompleteController;
import org.chromium.chrome.browser.omnibox.UrlBarData;
import org.chromium.chrome.browser.profiles.Profile;
import org.chromium.chrome.browser.search_engines.TemplateUrlService;
import org.chromium.chrome.browser.tab.Tab;
import org.chromium.chrome.browser.util.ColorUtils;
import org.chromium.chrome.browser.widget.bottomsheet.BottomSheet;
import org.chromium.components.dom_distiller.core.DomDistillerService;
import org.chromium.components.dom_distiller.core.DomDistillerUrlUtils;
import org.chromium.components.security_state.ConnectionSecurityLevel;
import org.chromium.content_public.browser.WebContents;

import java.net.URI;

/**
 * Provides a way of accessing toolbar data and state.
 */
public class ToolbarModel implements ToolbarDataProvider {
    private final Context mContext;
    private final BottomSheet mBottomSheet;
    private final boolean mUseModernDesign;

    private Tab mTab;
    private boolean mIsIncognito;
    private int mPrimaryColor;
    private boolean mIsUsingBrandColor;
    /** Query in Omnibox cached values to avoid unnecessary expensive calls. */
    private boolean mQueryInOmniboxEnabled;
    private String mPreviousUrl;
    @ConnectionSecurityLevel
    private int mPreviousSecurityLevel;
    private String mCachedSearchTerms;
    private boolean mIgnoreSecurityLevelForSearchTerms;

    private long mNativeToolbarModelAndroid;

    /**
     * Default constructor for this class.
     * @param context The Context used for styling the toolbar visuals.
     * @param bottomSheet The {@link BottomSheet} for the activity displaying this toolbar.
     * @param useModernDesign Whether the modern design should be used for the toolbar represented
     *                        by this model.
     */
    public ToolbarModel(
            Context context, @Nullable BottomSheet bottomSheet, boolean useModernDesign) {
        mContext = context;
        mBottomSheet = bottomSheet;
        mUseModernDesign = useModernDesign;
        mPrimaryColor =
                ColorUtils.getDefaultThemeColor(context.getResources(), useModernDesign, false);
    }

    /**
     * Handle any initialization that must occur after native has been initialized.
     */
    public void initializeWithNative() {
        mNativeToolbarModelAndroid = nativeInit();
        mQueryInOmniboxEnabled = ChromeFeatureList.isEnabled(ChromeFeatureList.QUERY_IN_OMNIBOX);
    }

    /**
     * Destroys the native ToolbarModel.
     */
    public void destroy() {
        if (mNativeToolbarModelAndroid == 0) return;
        nativeDestroy(mNativeToolbarModelAndroid);
        mNativeToolbarModelAndroid = 0;
    }

    /**
     * @return The currently active WebContents being used by the Toolbar.
     */
    @CalledByNative
    private WebContents getActiveWebContents() {
        if (!hasTab()) return null;
        return mTab.getWebContents();
    }

    /**
     * Sets the tab that contains the information to be displayed in the toolbar.
     * @param tab The tab associated currently with the toolbar.
     * @param isIncognito Whether the incognito model is currently selected, which must match the
     *                    passed in tab if non-null.
     */
    public void setTab(Tab tab, boolean isIncognito) {
        assert tab == null || tab.isIncognito() == isIncognito;
        mTab = tab;
        mIsIncognito = isIncognito;
        updateUsingBrandColor();
    }

    @Override
    public Tab getTab() {
        return hasTab() ? mTab : null;
    }

    @Override
    public boolean hasTab() {
        // TODO(dtrainor, tedchoc): Remove the isInitialized() check when we no longer wait for
        // TAB_CLOSED events to remove this tab.  Otherwise there is a chance we use this tab after
        // {@link ChromeTab#destroy()} is called.
        return mTab != null && mTab.isInitialized();
    }

    @Override
    public String getCurrentUrl() {
        // TODO(yusufo) : Consider using this for all calls from getTab() for accessing url.
        if (!hasTab()) return "";

        // Tab.getUrl() returns empty string if it does not have a URL.
        return getTab().getUrl().trim();
    }

    @Override
    public NewTabPage getNewTabPageForCurrentTab() {
        if (hasTab() && mTab.getNativePage() instanceof NewTabPage) {
            return (NewTabPage) mTab.getNativePage();
        }
        return null;
    }

    @Override
    public UrlBarData getUrlBarData() {
        if (!hasTab()) return UrlBarData.EMPTY;

        String url = getCurrentUrl();
        if (NativePageFactory.isNativePageUrl(url, isIncognito()) || NewTabPage.isNTPUrl(url)) {
            return UrlBarData.EMPTY;
        }

        String formattedUrl = getFormattedFullUrl();
        if (mTab.isFrozen()) return UrlBarData.forUrlAndText(url, formattedUrl);

        if (DomDistillerUrlUtils.isDistilledPage(url)) {
            DomDistillerService domDistillerService =
                    DomDistillerServiceFactory.getForProfile(getProfile());
            String entryIdFromUrl = DomDistillerUrlUtils.getValueForKeyInUrl(url, "entry_id");
            if (!TextUtils.isEmpty(entryIdFromUrl)
                    && domDistillerService.hasEntry(entryIdFromUrl)) {
                String originalUrl = domDistillerService.getUrlForEntry(entryIdFromUrl);
                return UrlBarData.forUrl(
                        DomDistillerTabUtils.getFormattedUrlFromOriginalDistillerUrl(originalUrl));
            }

            String originalUrl = DomDistillerUrlUtils.getOriginalUrlFromDistillerUrl(url);
            if (originalUrl != null) {
                return UrlBarData.forUrl(
                        DomDistillerTabUtils.getFormattedUrlFromOriginalDistillerUrl(originalUrl));
            }
            return UrlBarData.forUrlAndText(url, formattedUrl);
        }

        if (isOfflinePage()) {
            String originalUrl = mTab.getOriginalUrl();
            formattedUrl = OfflinePageUtils.stripSchemeFromOnlineUrl(
                    DomDistillerTabUtils.getFormattedUrlFromOriginalDistillerUrl(originalUrl));

            // Clear the editing text for untrusted offline pages.
            if (!OfflinePageUtils.isShowingTrustedOfflinePage(mTab)) {
                return UrlBarData.forUrlAndText(url, formattedUrl, "");
            }

            return UrlBarData.forUrlAndText(url, formattedUrl);
        }

        if (shouldDisplaySearchTerms()) {
            // Show the search terms in the omnibox instead of the URL if this is a DSE search URL.
            return UrlBarData.forUrlAndText(url, getDisplaySearchTerms());
        }

        if (ChromeFeatureList.isEnabled(
                    ChromeFeatureList.OMNIBOX_HIDE_SCHEME_DOMAIN_IN_STEADY_STATE)) {
            String urlForDisplay = getUrlForDisplay();
            if (!urlForDisplay.equals(formattedUrl)) {
                return UrlBarData.forUrlAndText(url, urlForDisplay, formattedUrl);
            }
        }

        return UrlBarData.forUrlAndText(url, formattedUrl);
    }

    @Override
    public String getTitle() {
        if (!hasTab()) return "";

        String title = getTab().getTitle();
        return TextUtils.isEmpty(title) ? title : title.trim();
    }

    @Override
    public boolean isIncognito() {
        return mIsIncognito;
    }

    @Override
    public Profile getProfile() {
        Profile lastUsedProfile = Profile.getLastUsedProfile();
        if (mIsIncognito) {
            assert lastUsedProfile.hasOffTheRecordProfile();
            return lastUsedProfile.getOffTheRecordProfile();
        }
        return lastUsedProfile.getOriginalProfile();
    }

    /**
     * Sets the primary color and changes the state for isUsingBrandColor.
     * @param color The primary color for the current tab.
     */
    public void setPrimaryColor(int color) {
        mPrimaryColor = color;
        updateUsingBrandColor();
    }

    private void updateUsingBrandColor() {
        Context context = ContextUtils.getApplicationContext();
        mIsUsingBrandColor = !isIncognito()
                && mPrimaryColor
                        != ColorUtils.getDefaultThemeColor(
                                   context.getResources(), mUseModernDesign, isIncognito())
                && hasTab() && !mTab.isNativePage();
    }

    @Override
    public int getPrimaryColor() {
        return mPrimaryColor;
    }

    @Override
    public boolean isUsingBrandColor() {
        return mIsUsingBrandColor && mBottomSheet == null;
    }

    @Override
    public boolean isOfflinePage() {
        return hasTab() && OfflinePageUtils.isOfflinePage(mTab);
    }

    @Override
    public boolean shouldShowGoogleG(String urlBarText) {
        LocaleManager localeManager = LocaleManager.getInstance();
        if (localeManager.hasCompletedSearchEnginePromo()
                || localeManager.hasShownSearchEnginePromoThisSession()) {
            return false;
        }

        // Only access ChromeFeatureList and TemplateUrlService after the NTP check,
        // to prevent native method calls before the native side has been initialized.
        NewTabPage ntp = getNewTabPageForCurrentTab();
        boolean isShownInRegularNtp = ntp != null && ntp.isLocationBarShownInNTP()
                && ChromeFeatureList.isEnabled(ChromeFeatureList.NTP_SHOW_GOOGLE_G_IN_OMNIBOX);

        return isShownInRegularNtp
                && TemplateUrlService.getInstance().isDefaultSearchEngineGoogle();
    }

    @Override
    public boolean shouldShowVerboseStatus() {
        // Because is offline page is cleared a bit slower, we also ensure that connection security
        // level is NONE or HTTP_SHOW_WARNING (http://crbug.com/671453).
        int securityLevel = getSecurityLevel();
        return isOfflinePage()
                && (securityLevel == ConnectionSecurityLevel.NONE
                           || securityLevel == ConnectionSecurityLevel.HTTP_SHOW_WARNING);
    }

    @Override
    public int getSecurityLevel() {
        Tab tab = getTab();
        return getSecurityLevel(
                tab, isOfflinePage(), tab == null ? null : tab.getTrustedCdnPublisherUrl());
    }

    @Override
    public int getSecurityIconResource(boolean isTablet) {
        // If we're showing a query in the omnibox, and the security level is high enough to show
        // the search icon, return that instead of the security icon.
        if (shouldDisplaySearchTerms()) {
            return R.drawable.omnibox_search;
        }
        return getSecurityIconResource(getSecurityLevel(), !isTablet, isOfflinePage());
    }

    @VisibleForTesting
    @ConnectionSecurityLevel
    static int getSecurityLevel(Tab tab, boolean isOfflinePage, @Nullable String publisherUrl) {
        if (tab == null || isOfflinePage) {
            return ConnectionSecurityLevel.NONE;
        }

        int securityLevel = tab.getSecurityLevel();
        if (publisherUrl != null) {
            assert securityLevel != ConnectionSecurityLevel.DANGEROUS;
            return (URI.create(publisherUrl).getScheme().equals(UrlConstants.HTTPS_SCHEME))
                    ? ConnectionSecurityLevel.SECURE
                    : ConnectionSecurityLevel.HTTP_SHOW_WARNING;
        }
        return securityLevel;
    }

    @VisibleForTesting
    @DrawableRes
    static int getSecurityIconResource(
            int securityLevel, boolean isSmallDevice, boolean isOfflinePage) {
        if (isOfflinePage) {
            return R.drawable.offline_pin_round;
        }

        switch (securityLevel) {
            case ConnectionSecurityLevel.NONE:
                return isSmallDevice ? 0 : R.drawable.omnibox_info;
            case ConnectionSecurityLevel.HTTP_SHOW_WARNING:
                return R.drawable.omnibox_info;
            case ConnectionSecurityLevel.DANGEROUS:
                return R.drawable.omnibox_https_invalid;
            case ConnectionSecurityLevel.SECURE_WITH_POLICY_INSTALLED_CERT:
            case ConnectionSecurityLevel.SECURE:
            case ConnectionSecurityLevel.EV_SECURE:
                return R.drawable.omnibox_https_valid;
            default:
                assert false;
        }
        return 0;
    }

    @Override
    public ColorStateList getSecurityIconColorStateList() {
        int securityLevel = getSecurityLevel();

        Resources resources = mContext.getResources();
        ColorStateList list = null;
        int color = getPrimaryColor();
        boolean needLightIcon = ColorUtils.shouldUseLightForegroundOnBackground(color);
        if (isIncognito() || needLightIcon) {
            // For a dark theme color, use light icons.
            list = ApiCompatibilityUtils.getColorStateList(resources, R.color.light_mode_tint);
        } else if (!hasTab() || isUsingBrandColor()
                || ChromeFeatureList.isEnabled(
                           ChromeFeatureList.OMNIBOX_HIDE_SCHEME_DOMAIN_IN_STEADY_STATE)) {
            // For theme colors which are not dark and are also not
            // light enough to warrant an opaque URL bar, use dark
            // icons.
            list = ApiCompatibilityUtils.getColorStateList(resources, R.color.dark_mode_tint);
        } else {
            // For the default toolbar color, use a green or red icon.
            if (securityLevel == ConnectionSecurityLevel.DANGEROUS) {
                assert !shouldDisplaySearchTerms();
                list = ApiCompatibilityUtils.getColorStateList(resources, R.color.google_red_700);
            } else if (!shouldDisplaySearchTerms()
                    && (securityLevel == ConnectionSecurityLevel.SECURE
                               || securityLevel == ConnectionSecurityLevel.EV_SECURE)) {
                list = ApiCompatibilityUtils.getColorStateList(resources, R.color.google_green_700);
            } else {
                list = ApiCompatibilityUtils.getColorStateList(resources, R.color.dark_mode_tint);
            }
        }
        assert list != null : "Missing ColorStateList for Security Button.";
        return list;
    }

    @Override
    public boolean shouldDisplaySearchTerms() {
        if (!securityLevelSafeForQueryInOmnibox() && !mIgnoreSecurityLevelForSearchTerms) {
            return false;
        }
        if (!mQueryInOmniboxEnabled) return false;
        if (mTab != null && !(mTab.getActivity() instanceof ChromeTabbedActivity)) return false;
        if (TextUtils.isEmpty(getDisplaySearchTerms())) return false;
        return true;
    }

    private boolean securityLevelSafeForQueryInOmnibox() {
        int securityLevel = getSecurityLevel();
        return securityLevel == ConnectionSecurityLevel.SECURE
                || securityLevel == ConnectionSecurityLevel.EV_SECURE;
    }

    /**
     * Extracts query terms from the current URL if it's a SRP URL from the default search engine,
     * caching the result of the more expensive call to {@link #getDisplaySearchTermsInternal}.
     *
     * @return The search terms. Returns null if not a DSE SRP URL or there are no search terms to
     *         extract, if query in omnibox is disabled, or if the security level is insufficient to
     *         display search terms in place of SRP URL. This will also return nothing if the search
     *         terms look too much like a URL, since we don't want to display URL look-a-likes with
     *         "Query in Omnibox" to avoid confusion.
     */
    private String getDisplaySearchTerms() {
        String url = getCurrentUrl();
        if (url == null) {
            mCachedSearchTerms = null;
        } else {
            @ConnectionSecurityLevel
            int securityLevel = getSecurityLevel();
            if (!url.equals(mPreviousUrl) || securityLevel != mPreviousSecurityLevel) {
                mCachedSearchTerms = getDisplaySearchTermsInternal(url);
                mPreviousUrl = url;
                mPreviousSecurityLevel = securityLevel;
            }
        }
        return mCachedSearchTerms;
    }

    /**
     * Extracts query terms from a URL if it's a SRP URL from the default search engine, returning
     * the cached result of the previous call if this call is being performed for the same URL and
     * security level as last time.
     *
     * @param url The URL to extract search terms from.
     * @return The search terms. Returns null if not a DSE SRP URL or there are no search terms to
     *         extract, if query in omnibox is disabled, or if the security level is insufficient to
     *         display search terms in place of SRP URL.
     */
    private String getDisplaySearchTermsInternal(String url) {
        TemplateUrlService templateUrlService = TemplateUrlService.getInstance();
        if (!templateUrlService.isSearchResultsPageFromDefaultSearchProvider(url)) return null;

        String searchTerms = templateUrlService.extractSearchTermsFromUrl(url);
        // Avoid showing search terms that would be interpreted as a URL if typed into the
        // omnibox.
        return looksLikeUrl(searchTerms) ? null : searchTerms;
    }

    /**
     * Checks if the provided {@link String} look like a URL.
     *
     * @param input The {@link String} to check.
     * @return Whether or not the {@link String} appeared to be a URL.
     */
    private boolean looksLikeUrl(String input) {
        return !TextUtils.isEmpty(input)
                && !TextUtils.isEmpty(AutocompleteController.nativeQualifyPartialURLQuery(input));
    }

    /**
     * Sets a flag telling the model to ignore the security level in its check for whether to
     * display search terms or not. This is useful for avoiding the flicker that occurs when loading
     * a SRP URL before our SSL state updates.
     *
     * @param ignore Whether or not we should ignore the security level.
     */
    protected void setIgnoreSecurityLevelForSearchTerms(boolean ignore) {
        mIgnoreSecurityLevelForSearchTerms = ignore;
    }

    /** @return The formatted URL suitable for editing. */
    public String getFormattedFullUrl() {
        if (mNativeToolbarModelAndroid == 0) return "";
        return nativeGetFormattedFullURL(mNativeToolbarModelAndroid);
    }

    /** @return The formatted URL suitable for display only. */
    public String getUrlForDisplay() {
        if (mNativeToolbarModelAndroid == 0) return "";
        return nativeGetURLForDisplay(mNativeToolbarModelAndroid);
    }

    private native long nativeInit();
    private native void nativeDestroy(long nativeToolbarModelAndroid);
    private native String nativeGetFormattedFullURL(long nativeToolbarModelAndroid);
    private native String nativeGetURLForDisplay(long nativeToolbarModelAndroid);
}
