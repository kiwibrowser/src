// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.payments;

import android.graphics.drawable.BitmapDrawable;
import android.os.Handler;
import android.text.TextUtils;

import org.chromium.chrome.browser.ChromeActivity;
import org.chromium.content_public.browser.WebContents;
import org.chromium.payments.mojom.PaymentDetailsModifier;
import org.chromium.payments.mojom.PaymentItem;
import org.chromium.payments.mojom.PaymentMethodData;

import java.net.URI;
import java.util.Arrays;
import java.util.Collections;
import java.util.HashSet;
import java.util.List;
import java.util.Map;
import java.util.Set;

import javax.annotation.Nullable;

/**
 * This app class represents a service worker based payment app.
 *
 * Such apps are implemented as service workers according to the Payment
 * Handler API specification.
 *
 * @see https://w3c.github.io/webpayments-payment-handler/
 */
public class ServiceWorkerPaymentApp extends PaymentInstrument implements PaymentApp {
    private final WebContents mWebContents;
    private final long mRegistrationId;
    private final Set<String> mMethodNames;
    private final boolean mExplicitlyVerified;
    private final Capabilities[] mCapabilities;
    private final boolean mCanPreselect;
    private final Set<String> mPreferredRelatedApplicationIds;
    private final boolean mIsIncognito;

    // Below variables are used for installable service worker payment app specifically.
    private final boolean mNeedsInstallation;
    private final String mAppName;
    private final URI mSwUri;
    private final URI mScope;
    private final boolean mUseCache;

    /**
     * This class represents capabilities of a payment instrument. It is currently only used for
     * 'basic-card' payment instrument.
     */
    protected static class Capabilities {
        // Stores mojom::BasicCardNetwork.
        private int[] mSupportedCardNetworks;

        // Stores mojom::BasicCardType.
        private int[] mSupportedCardTypes;

        /**
         * Build capabilities for a payment instrument.
         *
         * @param supportedCardNetworks The supported card networks of a 'basic-card' payment
         *                              instrument.
         * @param supportedCardTypes    The supported card types of a 'basic-card' payment
         *                              instrument.
         */
        /* package */ Capabilities(int[] supportedCardNetworks, int[] supportedCardTypes) {
            mSupportedCardNetworks = supportedCardNetworks;
            mSupportedCardTypes = supportedCardTypes;
        }

        /**
         * Gets supported card networks.
         *
         * @return a set of mojom::BasicCardNetwork.
         */
        /* package */ int[] getSupportedCardNetworks() {
            return mSupportedCardNetworks;
        }

        /**
         * Gets supported card types.
         *
         * @return a set of mojom::BasicCardType.
         */
        /* package */ int[] getSupportedCardTypes() {
            return mSupportedCardTypes;
        }
    }

    /**
     * Build a service worker payment app instance per origin.
     *
     * @see https://w3c.github.io/webpayments-payment-handler/#structure-of-a-web-payment-app
     *
     * @param webContents                    The web contents where PaymentRequest was invoked.
     * @param registrationId                 The registration id of the corresponding service worker
     *                                       payment app.
     * @param scope                          The registration scope of the corresponding service
     *                                       worker.
     * @param name                           The name of the payment app.
     * @param userHint                       The user hint of the payment app.
     * @param origin                         The origin of the payment app.
     * @param icon                           The drawable icon of the payment app.
     * @param methodNames                    A set of payment method names supported by the payment
     *                                       app.
     * @param explicitlyVerified             A flag indicates whether this app has explicitly
     *                                       verified payment methods, like listed as default
     *                                       application or supported origin in the payment methods'
     *                                       manifest.
     * @param capabilities                   A set of capabilities of the payment instruments in
     *                                       this payment app (only valid for basic-card payment
     *                                       method for now).
     * @param preferredRelatedApplicationIds A set of preferred related application Ids.
     */
    public ServiceWorkerPaymentApp(WebContents webContents, long registrationId, URI scope,
            @Nullable String name, @Nullable String userHint, String origin,
            @Nullable BitmapDrawable icon, String[] methodNames, boolean explicitlyVerified,
            Capabilities[] capabilities, String[] preferredRelatedApplicationIds) {
        // Do not display duplicate information.
        super(scope.toString(), TextUtils.isEmpty(name) ? origin : name, userHint,
                TextUtils.isEmpty(name) ? null : origin, icon);
        mWebContents = webContents;
        mRegistrationId = registrationId;

        // Name and/or icon are set to null if fetching or processing the corresponding web
        // app manifest failed. Then do not preselect this payment app.
        mCanPreselect = !TextUtils.isEmpty(name) && icon != null;

        mMethodNames = new HashSet<>();
        for (int i = 0; i < methodNames.length; i++) {
            mMethodNames.add(methodNames[i]);
        }

        mExplicitlyVerified = explicitlyVerified;

        mCapabilities = Arrays.copyOf(capabilities, capabilities.length);

        mPreferredRelatedApplicationIds = new HashSet<>();
        Collections.addAll(mPreferredRelatedApplicationIds, preferredRelatedApplicationIds);

        ChromeActivity activity = ChromeActivity.fromWebContents(mWebContents);
        mIsIncognito = activity != null && activity.getCurrentTabModel() != null
                && activity.getCurrentTabModel().isIncognito();

        mNeedsInstallation = false;
        mAppName = name;
        mSwUri = null;
        mScope = null;
        mUseCache = false;
    }

    /**
     * Build a service worker payment app instance which has not been installed yet.
     * The payment app will be installed when paying with it.
     *
     * @param webContents The web contents where PaymentRequest was invoked.
     * @param name        The name of the payment app.
     * @param origin      The origin of the payment app.
     * @param swUri       The URI to get the service worker js script.
     * @param scope       The registration scope of the corresponding service worker.
     * @param useCache    Whether cache is used to register the service worker.
     * @param icon        The drawable icon of the payment app.
     * @param methodName  The supported method name.
     */
    public ServiceWorkerPaymentApp(WebContents webContents, @Nullable String name, String origin,
            URI swUri, URI scope, boolean useCache, @Nullable BitmapDrawable icon,
            String methodName) {
        // Do not display duplicate information.
        super(scope.toString(), TextUtils.isEmpty(name) ? origin : name, null,
                TextUtils.isEmpty(name) ? null : origin, icon);

        mWebContents = webContents;
        // No registration ID before the app is registered (installed).
        mRegistrationId = -1;
        // If name and/or icon is missing or failed to parse from the web app manifest, then do not
        // preselect this payment app.
        mCanPreselect = !TextUtils.isEmpty(name) && icon != null;
        mMethodNames = new HashSet<>();
        mMethodNames.add(methodName);
        // Installable payment apps must be default application of a payment method.
        mExplicitlyVerified = true;
        mCapabilities = new Capabilities[0];
        mPreferredRelatedApplicationIds = new HashSet<>();

        ChromeActivity activity = ChromeActivity.fromWebContents(mWebContents);
        mIsIncognito = activity != null && activity.getCurrentTabModel() != null
                && activity.getCurrentTabModel().isIncognito();

        mNeedsInstallation = true;
        mAppName = name;
        mSwUri = swUri;
        mScope = scope;
        mUseCache = useCache;
    }

    @Override
    public void getInstruments(Map<String, PaymentMethodData> methodDataMap, String origin,
            String iframeOrigin, byte[][] unusedCertificateChain,
            Map<String, PaymentDetailsModifier> modifiers, final InstrumentsCallback callback) {
        // Do not send canMakePayment event when in incognito mode or basic-card is the only
        // supported payment method or this app needs installation for the payment request or this
        // app has not been explicitly verified.
        if (mIsIncognito || isOnlySupportBasiccard(methodDataMap) || mNeedsInstallation
                || !mExplicitlyVerified) {
            new Handler().post(() -> {
                List<PaymentInstrument> instruments =
                        Collections.singletonList(ServiceWorkerPaymentApp.this);
                callback.onInstrumentsReady(ServiceWorkerPaymentApp.this, instruments);
            });
            return;
        }

        ServiceWorkerPaymentAppBridge.canMakePayment(mWebContents, mRegistrationId, origin,
                iframeOrigin, new HashSet<>(methodDataMap.values()),
                new HashSet<>(modifiers.values()), (boolean canMakePayment) -> {
                    List<PaymentInstrument> instruments = canMakePayment
                            ? Collections.singletonList(ServiceWorkerPaymentApp.this)
                            : Collections.emptyList();
                    callback.onInstrumentsReady(ServiceWorkerPaymentApp.this, instruments);
                });
    }

    // Returns true if 'basic-card' is the only supported payment method of this payment app in the
    // payment request.
    private boolean isOnlySupportBasiccard(Map<String, PaymentMethodData> methodDataMap) {
        Set<String> requestMethods = new HashSet<>(methodDataMap.keySet());
        requestMethods.retainAll(mMethodNames);
        return requestMethods.size() == 1
                && requestMethods.contains(BasicCardUtils.BASIC_CARD_METHOD_NAME);
    }

    // Matches |requestMethodData|.supportedTypes and |requestMethodData|.supportedNetwokrs for
    // 'basic-card' payment method with the Capabilities in this payment app to determine whether
    // this payment app supports |requestMethodData|.
    private boolean matchBasiccardCapabilities(PaymentMethodData requestMethodData) {
        // Empty supported card types and networks in payment request method data indicates it
        // supports all card types and networks.
        if (requestMethodData.supportedTypes.length == 0
                && requestMethodData.supportedNetworks.length == 0) {
            return true;
        }
        // Payment app with emtpy capabilities can only match payment request method data with empty
        // supported card types and networks.
        if (mCapabilities.length == 0) return false;

        Set<Integer> requestSupportedTypes = new HashSet<>();
        for (int i = 0; i < requestMethodData.supportedTypes.length; i++) {
            requestSupportedTypes.add(requestMethodData.supportedTypes[i]);
        }
        Set<Integer> requestSupportedNetworks = new HashSet<>();
        for (int i = 0; i < requestMethodData.supportedNetworks.length; i++) {
            requestSupportedNetworks.add(requestMethodData.supportedNetworks[i]);
        }

        // If requestSupportedTypes and requestSupportedNetworks are not empty, match them with the
        // capabilities. Break out of the for loop if a matched capability has been found. So 'j
        // < mCapabilities.length' indicates that there is a matched capability in this payment
        // app.
        int j = 0;
        for (; j < mCapabilities.length; j++) {
            if (!requestSupportedTypes.isEmpty()) {
                int[] supportedTypes = mCapabilities[j].getSupportedCardTypes();

                Set<Integer> capabilitiesSupportedCardTypes = new HashSet<>();
                for (int i = 0; i < supportedTypes.length; i++) {
                    capabilitiesSupportedCardTypes.add(supportedTypes[i]);
                }

                capabilitiesSupportedCardTypes.retainAll(requestSupportedTypes);
                if (capabilitiesSupportedCardTypes.isEmpty()) continue;
            }

            if (!requestSupportedNetworks.isEmpty()) {
                int[] supportedNetworks = mCapabilities[j].getSupportedCardNetworks();

                Set<Integer> capabilitiesSupportedCardNetworks = new HashSet<>();
                for (int i = 0; i < supportedNetworks.length; i++) {
                    capabilitiesSupportedCardNetworks.add(supportedNetworks[i]);
                }

                capabilitiesSupportedCardNetworks.retainAll(requestSupportedNetworks);
                if (capabilitiesSupportedCardNetworks.isEmpty()) continue;
            }

            break;
        }
        return j < mCapabilities.length;
    }

    @Override
    public Set<String> getAppMethodNames() {
        return Collections.unmodifiableSet(mMethodNames);
    }

    @Override
    public boolean supportsMethodsAndData(Map<String, PaymentMethodData> methodsAndData) {
        Set<String> methodNames = new HashSet<>(methodsAndData.keySet());
        methodNames.retainAll(mMethodNames);
        return !methodNames.isEmpty();
    }

    @Override
    public Set<String> getPreferredRelatedApplicationIds() {
        return Collections.unmodifiableSet(mPreferredRelatedApplicationIds);
    }

    @Override
    public String getAppIdentifier() {
        return getIdentifier();
    }

    @Override
    public Set<String> getInstrumentMethodNames() {
        return getAppMethodNames();
    }

    @Override
    public boolean isValidForPaymentMethodData(String method, PaymentMethodData data) {
        boolean isSupportedMethod = super.isValidForPaymentMethodData(method, data);
        if (isSupportedMethod && BasicCardUtils.BASIC_CARD_METHOD_NAME.equals(method)) {
            return matchBasiccardCapabilities(data);
        }
        return isSupportedMethod;
    }

    @Override
    public void invokePaymentApp(String id, String merchantName, String origin, String iframeOrigin,
            byte[][] unusedCertificateChain, Map<String, PaymentMethodData> methodData,
            PaymentItem total, List<PaymentItem> displayItems,
            Map<String, PaymentDetailsModifier> modifiers, InstrumentDetailsCallback callback) {
        if (mNeedsInstallation) {
            BitmapDrawable icon = (BitmapDrawable) getDrawableIcon();
            ServiceWorkerPaymentAppBridge.installAndInvokePaymentApp(mWebContents, origin,
                    iframeOrigin, id, new HashSet<>(methodData.values()), total,
                    new HashSet<>(modifiers.values()), callback, mAppName,
                    icon == null ? null : icon.getBitmap(), mSwUri, mScope, mUseCache,
                    mMethodNames.toArray(new String[0])[0]);
        } else {
            ServiceWorkerPaymentAppBridge.invokePaymentApp(mWebContents, mRegistrationId, origin,
                    iframeOrigin, id, new HashSet<>(methodData.values()), total,
                    new HashSet<>(modifiers.values()), callback);
        }
    }

    @Override
    public void abortPaymentApp(AbortCallback callback) {
        ServiceWorkerPaymentAppBridge.abortPaymentApp(mWebContents, mRegistrationId, callback);
    }

    @Override
    public void dismissInstrument() {}

    @Override
    public boolean canMakePayment() {
        // Return false for PaymentRequest.canMakePayment() if installation is needed.
        return !mNeedsInstallation;
    }

    @Override
    public boolean canPreselect() {
        return mCanPreselect;
    }
}
