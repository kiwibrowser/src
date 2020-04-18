// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.contextualsearch;

import android.content.Context;
import android.net.Uri;
import android.telephony.TelephonyManager;
import android.text.TextUtils;

import org.chromium.base.CollectionUtil;
import org.chromium.base.VisibleForTesting;
import org.chromium.chrome.browser.ChromeVersionInfo;
import org.chromium.chrome.browser.UrlConstants;
import org.chromium.chrome.browser.compositor.bottombar.contextualsearch.ContextualSearchPanel;
import org.chromium.chrome.browser.contextualsearch.ContextualSearchSelectionController.SelectionType;
import org.chromium.chrome.browser.preferences.ChromePreferenceManager;
import org.chromium.chrome.browser.preferences.PrefServiceBridge;

import java.net.URL;
import java.util.HashSet;
import java.util.List;
import java.util.Locale;
import java.util.regex.Pattern;

import javax.annotation.Nullable;


/**
 * Handles policy decisions for the {@code ContextualSearchManager}.
 */
class ContextualSearchPolicy {
    private static final Pattern CONTAINS_WHITESPACE_PATTERN = Pattern.compile("\\s");
    private static final String DOMAIN_GOOGLE = "google";
    private static final String PATH_AMP = "/amp/";
    private static final int REMAINING_NOT_APPLICABLE = -1;
    private static final int ONE_DAY_IN_MILLIS = 24 * 60 * 60 * 1000;
    private static final int TAP_TRIGGERED_PROMO_LIMIT = 50;

    private static final HashSet<String> PREDOMINENTLY_ENGLISH_SPEAKING_COUNTRIES =
            CollectionUtil.newHashSet("GB", "US");

    private final ChromePreferenceManager mPreferenceManager;
    private final ContextualSearchSelectionController mSelectionController;
    private ContextualSearchNetworkCommunicator mNetworkCommunicator;
    private ContextualSearchPanel mSearchPanel;

    // Members used only for testing purposes.
    private boolean mDidOverrideDecidedStateForTesting;
    private boolean mDecidedStateForTesting;
    private Integer mTapTriggeredPromoLimitForTesting;

    /**
     * ContextualSearchPolicy constructor.
     */
    public ContextualSearchPolicy(ContextualSearchSelectionController selectionController,
            ContextualSearchNetworkCommunicator networkCommunicator) {
        mPreferenceManager = ChromePreferenceManager.getInstance();

        mSelectionController = selectionController;
        mNetworkCommunicator = networkCommunicator;
    }

    /**
     * Sets the handle to the ContextualSearchPanel.
     * @param panel The ContextualSearchPanel.
     */
    public void setContextualSearchPanel(ContextualSearchPanel panel) {
        mSearchPanel = panel;
    }

    // TODO(donnd): Consider adding a test-only constructor that uses dependency injection of a
    // preference manager and PrefServiceBridge.  Currently this is not possible because the
    // PrefServiceBridge is final.

    /**
     * @return The number of additional times to show the promo on tap, 0 if it should not be shown,
     *         or a negative value if the counter has been disabled or the user has accepted
     *         the promo.
     */
    int getPromoTapsRemaining() {
        if (!isUserUndecided()) return REMAINING_NOT_APPLICABLE;

        // Return a non-negative value if opt-out promo counter is enabled, and there's a limit.
        DisableablePromoTapCounter counter = getPromoTapCounter();
        if (counter.isEnabled()) {
            int limit = getPromoTapTriggeredLimit();
            if (limit >= 0) return Math.max(0, limit - counter.getCount());
        }

        return REMAINING_NOT_APPLICABLE;
    }

    private int getPromoTapTriggeredLimit() {
        if (mTapTriggeredPromoLimitForTesting != null) {
            return mTapTriggeredPromoLimitForTesting.intValue();
        }
        return TAP_TRIGGERED_PROMO_LIMIT;
    }

    /**
     * @return the {@link DisableablePromoTapCounter}.
     */
    DisableablePromoTapCounter getPromoTapCounter() {
        return DisableablePromoTapCounter.getInstance(mPreferenceManager);
    }

    /**
     * @return Whether a Tap gesture is currently supported as a trigger for the feature.
     */
    boolean isTapSupported() {
        if (!isUserUndecided()) return true;

        if (ContextualSearchFieldTrial.isContextualSearchTapDisableOverrideEnabled()) return true;

        return getPromoTapsRemaining() != 0;
    }

    /**
     * @return whether or not the Contextual Search Result should be preloaded before the user
     *         explicitly interacts with the feature.
     */
    boolean shouldPrefetchSearchResult() {
        if (isMandatoryPromoAvailable()) return false;

        if (!PrefServiceBridge.getInstance().getNetworkPredictionEnabled()) return false;

        // We never preload on long-press so users can cut & paste without hitting the servers.
        return mSelectionController.getSelectionType() == SelectionType.TAP;
    }

    /**
     * Returns whether the previous tap (the tap last counted) should resolve.
     * @return Whether the previous tap should resolve.
     */
    boolean shouldPreviousTapResolve() {
        if (isMandatoryPromoAvailable()) return false;

        if (ContextualSearchFieldTrial.isSearchTermResolutionDisabled()) return false;

        if (isPromoAvailable()) return isBasePageHTTP(mNetworkCommunicator.getBasePageUrl());

        return true;
    }

    /**
     * Returns whether surrounding context can be accessed by other systems or not.
     * @return Whether surroundings are available.
     */
    boolean canSendSurroundings() {
        if (mDidOverrideDecidedStateForTesting) return mDecidedStateForTesting;

        if (isPromoAvailable()) return isBasePageHTTP(mNetworkCommunicator.getBasePageUrl());

        return true;
    }

    /**
     * @return Whether the Mandatory Promo is enabled.
     */
    boolean isMandatoryPromoAvailable() {
        if (!isUserUndecided()) return false;

        if (!ContextualSearchFieldTrial.isMandatoryPromoEnabled()) return false;

        return getPromoOpenCount() >= ContextualSearchFieldTrial.getMandatoryPromoLimit();
    }

    /**
     * @return Whether the Opt-out promo is available to be shown in any panel.
     */
    boolean isPromoAvailable() {
        return isUserUndecided();
    }

    /**
     * Registers that a tap has taken place by incrementing tap-tracking counters.
     */
    void registerTap() {
        if (isPromoAvailable()) {
            DisableablePromoTapCounter promoTapCounter = getPromoTapCounter();
            // Bump the counter only when it is still enabled.
            if (promoTapCounter.isEnabled()) {
                promoTapCounter.increment();
            }
        }
        int tapsSinceOpen = mPreferenceManager.getContextualSearchTapCount();
        mPreferenceManager.setContextualSearchTapCount(++tapsSinceOpen);
        if (isUserUndecided()) {
            ContextualSearchUma.logTapsSinceOpenForUndecided(tapsSinceOpen);
        } else {
            ContextualSearchUma.logTapsSinceOpenForDecided(tapsSinceOpen);
        }
    }

    /**
     * Updates all the counters to account for an open-action on the panel.
     */
    void updateCountersForOpen() {
        // Always completely reset the tap counter, since it just counts taps
        // since the last open.
        mPreferenceManager.setContextualSearchTapCount(0);
        mPreferenceManager.setContextualSearchTapQuickAnswerCount(0);

        // Disable the "promo tap" counter, but only if we're using the Opt-out onboarding.
        // For Opt-in, we never disable the promo tap counter.
        if (isPromoAvailable()) {
            getPromoTapCounter().disable();

            // Bump the total-promo-opens counter.
            int count = mPreferenceManager.getContextualSearchPromoOpenCount();
            mPreferenceManager.setContextualSearchPromoOpenCount(++count);
            ContextualSearchUma.logPromoOpenCount(count);
        }
    }

    /**
     * Updates Tap counters to account for a quick-answer caption shown on the panel.
     * @param wasActivatedByTap Whether the triggering gesture was a Tap or not.
     * @param doesAnswer Whether the caption is considered an answer rather than just
     *                          informative.
     */
    void updateCountersForQuickAnswer(boolean wasActivatedByTap, boolean doesAnswer) {
        if (wasActivatedByTap && doesAnswer) {
            int tapsWithAnswerSinceOpen =
                    mPreferenceManager.getContextualSearchTapQuickAnswerCount();
            mPreferenceManager.setContextualSearchTapQuickAnswerCount(++tapsWithAnswerSinceOpen);
        }
    }

    /**
     * @return Whether a verbatim request should be made for the given base page, assuming there
     *         is no exiting request.
     */
    boolean shouldCreateVerbatimRequest() {
        SelectionType selectionType = mSelectionController.getSelectionType();
        return (mSelectionController.getSelectedText() != null
                && (selectionType == SelectionType.LONG_PRESS
                || (selectionType == SelectionType.TAP && !shouldPreviousTapResolve())));
    }

    /**
     * Determines whether an error from a search term resolution request should
     * be shown to the user, or not.
     */
    boolean shouldShowErrorCodeInBar() {
        // Builds with lots of real users should not see raw error codes.
        return !(ChromeVersionInfo.isStableBuild() || ChromeVersionInfo.isBetaBuild());
    }

    /**
     * Logs the current user's state, including preference, tap and open counters, etc.
     */
    void logCurrentState() {
        ContextualSearchUma.logPreferenceState();

        // Log the number of promo taps remaining.
        int promoTapsRemaining = getPromoTapsRemaining();
        if (promoTapsRemaining >= 0) ContextualSearchUma.logPromoTapsRemaining(promoTapsRemaining);

        // Also log the total number of taps before opening the promo, even for those
        // that are no longer tap limited. That way we'll know the distribution of the
        // number of taps needed before opening the promo.
        DisableablePromoTapCounter promoTapCounter = getPromoTapCounter();
        boolean wasOpened = !promoTapCounter.isEnabled();
        int count = promoTapCounter.getCount();
        if (wasOpened) {
            ContextualSearchUma.logPromoTapsBeforeFirstOpen(count);
        } else {
            ContextualSearchUma.logPromoTapsForNeverOpened(count);
        }
    }

    /**
     * Logs details about the Search Term Resolution.
     * Should only be called when a search term has been resolved.
     * @param searchTerm The Resolved Search Term.
     */
    void logSearchTermResolutionDetails(String searchTerm) {
        // Only log for decided users so the data reflect fully-enabled behavior.
        // Otherwise we'll get skewed data; more HTTP pages than HTTPS (since those don't resolve),
        // and it's also possible that public pages, e.g. news, have more searches for multi-word
        // entities like people.
        if (!isUserUndecided()) {
            URL url = mNetworkCommunicator.getBasePageUrl();
            ContextualSearchUma.logBasePageProtocol(isBasePageHTTP(url));
            boolean isSingleWord = !CONTAINS_WHITESPACE_PATTERN.matcher(searchTerm.trim()).find();
            ContextualSearchUma.logSearchTermResolvedWords(isSingleWord);
        }
    }

    /**
     * Whether sending the URL of the base page to the server may be done for policy reasons.
     * NOTE: There may be additional privacy reasons why the base page URL should not be sent.
     * TODO(donnd): Update this API to definitively determine if it's OK to send the URL,
     * by merging the checks in the native contextual_search_delegate here.
     * @return {@code true} if the URL may be sent for policy reasons.
     *         Note that a return value of {@code true} may still require additional checks
     *         to see if all privacy-related conditions are met to send the base page URL.
     */
    boolean maySendBasePageUrl() {
        return !isUserUndecided();
    }

    /**
     * The search provider icon is animated every time on long press if the user has never opened
     * the panel before and once a day on tap.
     *
     * @return Whether the search provider icon should be animated.
     */
    boolean shouldAnimateSearchProviderIcon() {
        if (mSearchPanel.isShowing()) {
            return false;
        }

        SelectionType selectionType = mSelectionController.getSelectionType();
        if (selectionType == SelectionType.TAP) {
            long currentTimeMillis = System.currentTimeMillis();
            long lastAnimatedTimeMillis =
                    mPreferenceManager.getContextualSearchLastAnimationTime();
            if (Math.abs(currentTimeMillis - lastAnimatedTimeMillis) > ONE_DAY_IN_MILLIS) {
                mPreferenceManager.setContextualSearchLastAnimationTime(currentTimeMillis);
                return true;
            } else {
                return false;
            }
        } else if (selectionType == SelectionType.LONG_PRESS) {
            // If the panel has never been opened before, getPromoOpenCount() will be 0.
            // Once the panel has been opened, regardless of whether or not the user has opted-in or
            // opted-out, the promo open count will be greater than zero.
            return isUserUndecided() && getPromoOpenCount() == 0;
        }

        return false;
    }

    /**
     * @return Whether the given URL is used for Accelerated Mobile Pages by Google.
     */
    boolean isAmpUrl(String url) {
        Uri uri = Uri.parse(url);
        if (uri == null || uri.getHost() == null || uri.getPath() == null) return false;

        return uri.getHost().contains(DOMAIN_GOOGLE) && uri.getPath().startsWith(PATH_AMP);
    }

    // --------------------------------------------------------------------------------------------
    // Testing support.
    // --------------------------------------------------------------------------------------------

    /**
     * Overrides the decided/undecided state for the user preference.
     * @param decidedState Whether the user has decided or not.
     */
    @VisibleForTesting
    void overrideDecidedStateForTesting(boolean decidedState) {
        mDidOverrideDecidedStateForTesting = true;
        mDecidedStateForTesting = decidedState;
    }

    /**
     * @return count of times the panel with the promo has been opened.
     */
    @VisibleForTesting
    int getPromoOpenCount() {
        return mPreferenceManager.getContextualSearchPromoOpenCount();
    }

    /**
     * @return The number of times the user has tapped since the last panel open.
     */
    @VisibleForTesting
    int getTapCount() {
        return mPreferenceManager.getContextualSearchTapCount();
    }

    // --------------------------------------------------------------------------------------------
    // Translation support.
    // --------------------------------------------------------------------------------------------

    /**
     * Determines whether translation is needed between the given languages.
     * @param sourceLanguage The source language code; language we're translating from.
     * @param targetLanguages A list of target language codes; languages we might translate to.
     * @return Whether translation is needed or not.
     */
    boolean needsTranslation(String sourceLanguage, List<String> targetLanguages) {
        // For now, we just look for a language match.
        for (String targetLanguage : targetLanguages) {
            if (TextUtils.equals(sourceLanguage, targetLanguage)) {
                return false;
            }
        }
        return true;
    }

    /**
     * @return The best target language from the ordered list, or the empty string if
     *         none is available.
     */
    String bestTargetLanguage(List<String> targetLanguages) {
        return bestTargetLanguage(targetLanguages, Locale.getDefault().getCountry());
    }

    /**
     * Determines the best language to convert into, given the ordered list of languages the user
     * knows, and the UX language.
     * @param targetLanguages The list of languages to consider converting to.
     * @param countryOfUx The country of the UX.
     * @return the best language or an empty string.
     */
    @VisibleForTesting
    String bestTargetLanguage(List<String> targetLanguages, String countryOfUx) {
        // For now, we just return the first language, unless it's English
        // (due to over-usage).
        // TODO(donnd): Improve this logic. Determining the right language seems non-trivial.
        // E.g. If this language doesn't match the user's server preferences, they might see a page
        // in one language and the one box translation in another, which might be confusing.
        // Also this logic should only apply on Android, where English setup is overused.
        if (targetLanguages.size() > 1
                && TextUtils.equals(targetLanguages.get(0), Locale.ENGLISH.getLanguage())
                && !PREDOMINENTLY_ENGLISH_SPEAKING_COUNTRIES.contains(countryOfUx)) {
            return targetLanguages.get(1);
        } else if (targetLanguages.size() > 0) {
            return targetLanguages.get(0);
        } else {
            return "";
        }
    }

    /**
     * @return Whether any translation feature for Contextual Search is enabled.
     */
    boolean isTranslationDisabled() {
        return ContextualSearchFieldTrial.isTranslationDisabled();
    }

    /**
     * @return The ISO country code for the user's home country, or an empty string if not
     *         available or privacy-enabled.
     */
    String getHomeCountry(Context context) {
        if (ContextualSearchFieldTrial.isSendHomeCountryDisabled()) return "";

        TelephonyManager telephonyManager =
                (TelephonyManager) context.getSystemService(Context.TELEPHONY_SERVICE);
        if (telephonyManager == null) return "";

        String simCountryIso = telephonyManager.getSimCountryIso();
        if (TextUtils.isEmpty(simCountryIso)) return "";

        return simCountryIso;
    }

    /**
     * @return Whether a promo is needed because the user is still undecided
     *         on enabling or disabling the feature.
     */
    boolean isUserUndecided() {
        // TODO(donnd) use dependency injection for the PrefServiceBridge instead!
        if (mDidOverrideDecidedStateForTesting) return !mDecidedStateForTesting;

        return PrefServiceBridge.getInstance().isContextualSearchUninitialized();
    }

    /**
     * @param url The URL of the base page.
     * @return Whether the given content view is for an HTTP page.
     */
    boolean isBasePageHTTP(@Nullable URL url) {
        return url != null && UrlConstants.HTTP_SCHEME.equals(url.getProtocol());
    }

    // --------------------------------------------------------------------------------------------
    // Testing helpers.
    // --------------------------------------------------------------------------------------------

    /**
     * Sets the {@link ContextualSearchNetworkCommunicator} to use for server requests.
     * @param networkCommunicator The communicator for all future requests.
     */
    @VisibleForTesting
    public void setNetworkCommunicator(ContextualSearchNetworkCommunicator networkCommunicator) {
        mNetworkCommunicator = networkCommunicator;
    }
}
