// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.payments;

import android.content.Context;
import android.graphics.Bitmap;
import android.graphics.drawable.BitmapDrawable;
import android.text.TextUtils;
import android.util.Pair;

import org.chromium.base.Log;
import org.chromium.base.ThreadUtils;
import org.chromium.base.VisibleForTesting;
import org.chromium.base.annotations.CalledByNative;
import org.chromium.base.annotations.JNIAdditionalImport;
import org.chromium.chrome.browser.ChromeActivity;
import org.chromium.chrome.browser.ChromeFeatureList;
import org.chromium.chrome.browser.tab.EmptyTabObserver;
import org.chromium.chrome.browser.tab.Tab;
import org.chromium.components.payments.OriginSecurityChecker;
import org.chromium.content_public.browser.WebContents;
import org.chromium.payments.mojom.PaymentDetailsModifier;
import org.chromium.payments.mojom.PaymentItem;
import org.chromium.payments.mojom.PaymentMethodData;

import java.net.URI;
import java.util.HashMap;
import java.util.Map;
import java.util.Set;

import javax.annotation.Nullable;

/**
 * Native bridge for interacting with service worker based payment apps.
 */
@JNIAdditionalImport({PaymentInstrument.class, PaymentAppFactory.class})
public class ServiceWorkerPaymentAppBridge implements PaymentAppFactory.PaymentAppFactoryAddition {
    private static final String TAG = "SWPaymentApp";
    private static boolean sCanMakePaymentForTesting;

    /** The interface for checking whether there is an installed SW payment app. */
    static public interface HasServiceWorkerPaymentAppsCallback {
        /**
         * Called to return checking result.
         *
         * @param hasPaymentApps Indicates whehter there is an installed SW payment app.
         */
        public void onHasServiceWorkerPaymentAppsResponse(boolean hasPaymentApps);
    }

    /** The interface for getting all installed SW payment apps' information. */
    static public interface GetServiceWorkerPaymentAppsInfoCallback {
        /**
         * Called to return installed SW payment apps' information.
         *
         * @param appsInfo Contains all installed SW payment apps' information.
         */
        public void onGetServiceWorkerPaymentAppsInfo(Map<String, Pair<String, Bitmap>> appsInfo);
    }

    /**
     * The interface for the requester to check whether a SW payment app can make payment.
     */
    static interface CanMakePaymentCallback {
        /**
         * Called by this app to provide an information whether can make payment asynchronously.
         *
         * @param canMakePayment Indicates whether a SW payment app can make payment.
         */
        public void onCanMakePaymentResponse(boolean canMakePayment);
    }

    @Override
    public void create(WebContents webContents, Map<String, PaymentMethodData> methodData,
            boolean mayCrawl, PaymentAppFactory.PaymentAppCreatedCallback callback) {
        ThreadUtils.assertOnUiThread();

        nativeGetAllPaymentApps(webContents,
                methodData.values().toArray(new PaymentMethodData[methodData.size()]), mayCrawl,
                callback);
    }

    /**
     * Checks whether there is a installed SW payment app.
     *
     * @param callback The callback to return result.
     */
    public static void hasServiceWorkerPaymentApps(HasServiceWorkerPaymentAppsCallback callback) {
        ThreadUtils.assertOnUiThread();

        if (!ChromeFeatureList.isEnabled(ChromeFeatureList.SERVICE_WORKER_PAYMENT_APPS)) {
            ThreadUtils.postOnUiThread(new Runnable() {
                @Override
                public void run() {
                    callback.onHasServiceWorkerPaymentAppsResponse(false);
                }
            });
            return;
        }
        nativeHasServiceWorkerPaymentApps(callback);
    }

    /**
     * Gets all installed SW payment apps' information.
     *
     * @param callback The callback to return result.
     */
    public static void getServiceWorkerPaymentAppsInfo(
            GetServiceWorkerPaymentAppsInfoCallback callback) {
        ThreadUtils.assertOnUiThread();

        if (!ChromeFeatureList.isEnabled(ChromeFeatureList.SERVICE_WORKER_PAYMENT_APPS)) {
            ThreadUtils.postOnUiThread(new Runnable() {
                @Override
                public void run() {
                    callback.onGetServiceWorkerPaymentAppsInfo(
                            new HashMap<String, Pair<String, Bitmap>>());
                }
            });
            return;
        }
        nativeGetServiceWorkerPaymentAppsInfo(callback);
    }

    /**
     * Returns whether the app can make a payment.
     *
     * @param webContents      The web contents that invoked PaymentRequest.
     * @param registrationId   The service worker registration ID of the Payment App.
     * @param origin           The origin of this merchant.
     * @param iframeOrigin     The origin of the iframe that invoked PaymentRequest. Same as origin
     *                         if PaymentRequest was not invoked from inside an iframe.
     * @param methodData       The PaymentMethodData objects that are relevant for this payment
     *                         app.
     * @param modifiers        Payment method specific modifiers to the payment items and the total.
     * @param callback         Called after the payment app is finished running.
     */
    public static void canMakePayment(WebContents webContents, long registrationId, String origin,
            String iframeOrigin, Set<PaymentMethodData> methodData,
            Set<PaymentDetailsModifier> modifiers, CanMakePaymentCallback callback) {
        ThreadUtils.assertOnUiThread();

        if (sCanMakePaymentForTesting) {
            ThreadUtils.postOnUiThread(new Runnable() {
                @Override
                public void run() {
                    callback.onCanMakePaymentResponse(true);
                }
            });
            return;
        }
        nativeCanMakePayment(webContents, registrationId, origin, iframeOrigin,
                methodData.toArray(new PaymentMethodData[0]),
                modifiers.toArray(new PaymentDetailsModifier[0]), callback);
    }

    /**
     * Make canMakePayment() return true always for testing purpose.
     *
     * @param canMakePayment Indicates whether a SW payment app can make payment.
     */
    @VisibleForTesting
    public static void setCanMakePaymentForTesting(boolean canMakePayment) {
        sCanMakePaymentForTesting = canMakePayment;
    }

    /**
     * Invoke a payment app with a given option and matching method data.
     *
     * @param webContents      The web contents that invoked PaymentRequest.
     * @param registrationId   The service worker registration ID of the Payment App.
     * @param origin           The origin of this merchant.
     * @param iframeOrigin     The origin of the iframe that invoked PaymentRequest. Same as origin
     *                         if PaymentRequest was not invoked from inside an iframe.
     * @param paymentRequestId The unique identifier of the PaymentRequest.
     * @param methodData       The PaymentMethodData objects that are relevant for this payment
     *                         app.
     * @param total            The PaymentItem that represents the total cost of the payment.
     * @param modifiers        Payment method specific modifiers to the payment items and the total.
     * @param callback         Called after the payment app is finished running.
     */
    public static void invokePaymentApp(WebContents webContents, long registrationId, String origin,
            String iframeOrigin, String paymentRequestId, Set<PaymentMethodData> methodData,
            PaymentItem total, Set<PaymentDetailsModifier> modifiers,
            PaymentInstrument.InstrumentDetailsCallback callback) {
        ThreadUtils.assertOnUiThread();

        nativeInvokePaymentApp(webContents, registrationId, origin, iframeOrigin, paymentRequestId,
                methodData.toArray(new PaymentMethodData[0]), total,
                modifiers.toArray(new PaymentDetailsModifier[0]), callback);
    }

    /**
     * Install and invoke a payment app with a given option and matching method data.
     *
     * @param webContents      The web contents that invoked PaymentRequest.
     * @param origin           The origin of this merchant.
     * @param iframeOrigin     The origin of the iframe that invoked PaymentRequest. Same as origin
     *                         if PaymentRequest was not invoked from inside an iframe.
     * @param paymentRequestId The unique identifier of the PaymentRequest.
     * @param methodData       The PaymentMethodData objects that are relevant for this payment
     *                         app.
     * @param total            The PaymentItem that represents the total cost of the payment.
     * @param modifiers        Payment method specific modifiers to the payment items and the total.
     * @param callback         Called after the payment app is finished running.
     * @param appName          The installable app name.
     * @param icon             The installable app icon.
     * @param swUri            The URI to get the app's service worker js script.
     * @param scope            The scope of the service worker that should be registered.
     * @param useCache         Whether to use cache when registering the service worker.
     * @param method           Supported method name of the app.
     */
    public static void installAndInvokePaymentApp(WebContents webContents, String origin,
            String iframeOrigin, String paymentRequestId, Set<PaymentMethodData> methodData,
            PaymentItem total, Set<PaymentDetailsModifier> modifiers,
            PaymentInstrument.InstrumentDetailsCallback callback, String appName,
            @Nullable Bitmap icon, URI swUri, URI scope, boolean useCache, String method) {
        ThreadUtils.assertOnUiThread();

        nativeInstallAndInvokePaymentApp(webContents, origin, iframeOrigin, paymentRequestId,
                methodData.toArray(new PaymentMethodData[0]), total,
                modifiers.toArray(new PaymentDetailsModifier[0]), callback, appName, icon,
                swUri.toString(), scope.toString(), useCache, method);
    }

    /**
     * Abort invocation of the payment app.
     *
     * @param webContents      The web contents that invoked PaymentRequest.
     * @param registrationId   The service worker registration ID of the Payment App.
     * @param callback         Called after abort invoke payment app is finished running.
     */
    public static void abortPaymentApp(WebContents webContents, long registrationId,
            PaymentInstrument.AbortCallback callback) {
        ThreadUtils.assertOnUiThread();

        nativeAbortPaymentApp(webContents, registrationId, callback);
    }

    /**
     * Add observer for the opened payment app window tab so as to validate whether the web
     * contents is secure.
     *
     * @param tab The opened payment app window tab.
     */
    public static void addTabObserverForPaymentRequestTab(Tab tab) {
        tab.addObserver(new EmptyTabObserver() {
            @Override
            public void onPageLoadFinished(Tab tab) {
                // Notify closing payment app window so as to abort payment if unsecure.
                WebContents webContents = tab.getWebContents();
                if (!OriginSecurityChecker.isOriginSecure(webContents.getLastCommittedUrl())
                        || (!OriginSecurityChecker.isSchemeCryptographic(
                                    webContents.getLastCommittedUrl())
                                   && !OriginSecurityChecker.isOriginLocalhostOrFile(
                                              webContents.getLastCommittedUrl()))
                        || !SslValidityChecker.isSslCertificateValid(webContents)) {
                    onClosingPaymentAppWindow(webContents);
                }
            }
        });
    }

    /**
     * Notify closing the opened payment app window.
     *
     * @param webContents The web contents in the opened window.
     */
    public static void onClosingPaymentAppWindow(WebContents webContents) {
        nativeOnClosingPaymentAppWindow(webContents);
    }

    @CalledByNative
    private static String[] getSupportedMethodsFromMethodData(PaymentMethodData data) {
        return data.supportedMethods;
    }

    @CalledByNative
    private static String getStringifiedDataFromMethodData(PaymentMethodData data) {
        return data.stringifiedData;
    }

    @CalledByNative
    private static int[] getSupportedNetworksFromMethodData(PaymentMethodData data) {
        return data.supportedNetworks;
    }

    @CalledByNative
    private static int[] getSupportedTypesFromMethodData(PaymentMethodData data) {
        return data.supportedTypes;
    }

    @CalledByNative
    private static PaymentMethodData getMethodDataFromModifier(PaymentDetailsModifier modifier) {
        return modifier.methodData;
    }

    @CalledByNative
    private static PaymentItem getTotalFromModifier(PaymentDetailsModifier modifier) {
        return modifier.total;
    }

    @CalledByNative
    private static String getLabelFromPaymentItem(PaymentItem item) {
        return item.label;
    }

    @CalledByNative
    private static String getCurrencyFromPaymentItem(PaymentItem item) {
        return item.amount.currency;
    }

    @CalledByNative
    private static String getValueFromPaymentItem(PaymentItem item) {
        return item.amount.value;
    }

    @CalledByNative
    private static Object[] createCapabilities(int count) {
        return new ServiceWorkerPaymentApp.Capabilities[count];
    }

    @CalledByNative
    private static void addCapabilities(Object[] capabilities, int index,
            int[] supportedCardNetworks, int[] supportedCardTypes) {
        assert index < capabilities.length;
        capabilities[index] =
                new ServiceWorkerPaymentApp.Capabilities(supportedCardNetworks, supportedCardTypes);
    }

    @CalledByNative
    private static void onPaymentAppCreated(long registrationId, String scope,
            @Nullable String name, @Nullable String userHint, String origin, @Nullable Bitmap icon,
            String[] methodNameArray, boolean explicitlyVerified, Object[] capabilities,
            String[] preferredRelatedApplications, WebContents webContents,
            PaymentAppFactory.PaymentAppCreatedCallback callback) {
        ThreadUtils.assertOnUiThread();

        Context context = ChromeActivity.fromWebContents(webContents);
        if (context == null) return;
        URI scopeUri = UriUtils.parseUriFromString(scope);
        if (scopeUri == null) {
            Log.e(TAG, "%s service worker scope is not a valid URI", scope);
            return;
        }
        callback.onPaymentAppCreated(new ServiceWorkerPaymentApp(webContents, registrationId,
                scopeUri, name, userHint, origin,
                icon == null ? null : new BitmapDrawable(context.getResources(), icon),
                methodNameArray, explicitlyVerified,
                (ServiceWorkerPaymentApp.Capabilities[]) capabilities,
                preferredRelatedApplications));
    }

    @CalledByNative
    private static void onInstallablePaymentAppCreated(@Nullable String name, String swUrl,
            String scope, boolean useCache, @Nullable Bitmap icon, String methodName,
            WebContents webContents, PaymentAppFactory.PaymentAppCreatedCallback callback) {
        ThreadUtils.assertOnUiThread();

        Context context = ChromeActivity.fromWebContents(webContents);
        if (context == null) return;
        URI swUri = UriUtils.parseUriFromString(swUrl);
        if (swUri == null) {
            Log.e(TAG, "%s service worker installation url is not a valid URI", swUrl);
            return;
        }
        URI scopeUri = UriUtils.parseUriFromString(scope);
        if (scopeUri == null) {
            Log.e(TAG, "%s service worker scope is not a valid URI", scope);
            return;
        }
        callback.onPaymentAppCreated(new ServiceWorkerPaymentApp(webContents, name,
                scopeUri.getHost(), swUri, scopeUri, useCache,
                icon == null ? null : new BitmapDrawable(context.getResources(), icon),
                methodName));
    }

    @CalledByNative
    private static void onAllPaymentAppsCreated(
            PaymentAppFactory.PaymentAppCreatedCallback callback) {
        ThreadUtils.assertOnUiThread();

        callback.onAllPaymentAppsCreated();
    }

    @CalledByNative
    private static void onHasServiceWorkerPaymentApps(
            HasServiceWorkerPaymentAppsCallback callback, boolean hasPaymentApps) {
        ThreadUtils.assertOnUiThread();

        callback.onHasServiceWorkerPaymentAppsResponse(hasPaymentApps);
    }

    @CalledByNative
    private static Object createPaymentAppsInfo() {
        return new HashMap<String, Pair<String, Bitmap>>();
    }

    @SuppressWarnings("unchecked")
    @CalledByNative
    private static void addPaymentAppInfo(
            Object appsInfo, String scope, @Nullable String name, @Nullable Bitmap icon) {
        ((Map<String, Pair<String, Bitmap>>) appsInfo).put(scope, new Pair<>(name, icon));
    }

    @SuppressWarnings("unchecked")
    @CalledByNative
    private static void onGetServiceWorkerPaymentAppsInfo(
            GetServiceWorkerPaymentAppsInfoCallback callback, Object appsInfo) {
        ThreadUtils.assertOnUiThread();

        callback.onGetServiceWorkerPaymentAppsInfo(((Map<String, Pair<String, Bitmap>>) appsInfo));
    }

    @CalledByNative
    private static void onPaymentAppInvoked(PaymentInstrument.InstrumentDetailsCallback callback,
            String methodName, String stringifiedDetails) {
        ThreadUtils.assertOnUiThread();

        if (TextUtils.isEmpty(methodName) || TextUtils.isEmpty(stringifiedDetails)) {
            callback.onInstrumentDetailsError();
        } else {
            callback.onInstrumentDetailsReady(methodName, stringifiedDetails);
        }
    }

    @CalledByNative
    private static void onPaymentAppAborted(
            PaymentInstrument.AbortCallback callback, boolean result) {
        ThreadUtils.assertOnUiThread();

        callback.onInstrumentAbortResult(result);
    }

    @CalledByNative
    private static void onCanMakePayment(CanMakePaymentCallback callback, boolean canMakePayment) {
        ThreadUtils.assertOnUiThread();

        callback.onCanMakePaymentResponse(canMakePayment);
    }

    private static native void nativeGetAllPaymentApps(WebContents webContents,
            PaymentMethodData[] methodData, boolean mayCrawlForInstallablePaymentApps,
            PaymentAppFactory.PaymentAppCreatedCallback callback);

    private static native void nativeHasServiceWorkerPaymentApps(
            HasServiceWorkerPaymentAppsCallback callback);
    private static native void nativeGetServiceWorkerPaymentAppsInfo(
            GetServiceWorkerPaymentAppsInfoCallback callback);

    private static native void nativeInvokePaymentApp(WebContents webContents, long registrationId,
            String topOrigin, String paymentRequestOrigin, String paymentRequestId,
            PaymentMethodData[] methodData, PaymentItem total, PaymentDetailsModifier[] modifiers,
            PaymentInstrument.InstrumentDetailsCallback callback);

    private static native void nativeInstallAndInvokePaymentApp(WebContents webContents,
            String topOrigin, String paymentRequestOrigin, String paymentRequestId,
            PaymentMethodData[] methodData, PaymentItem total, PaymentDetailsModifier[] modifiers,
            PaymentInstrument.InstrumentDetailsCallback callback, String appName,
            @Nullable Bitmap icon, String swUrl, String scope, boolean useCache, String method);

    private static native void nativeAbortPaymentApp(
            WebContents webContents, long registrationId, PaymentInstrument.AbortCallback callback);

    private static native void nativeCanMakePayment(WebContents webContents, long registrationId,
            String topOrigin, String paymentRequestOrigin, PaymentMethodData[] methodData,
            PaymentDetailsModifier[] modifiers, CanMakePaymentCallback callback);

    private static native void nativeOnClosingPaymentAppWindow(WebContents webContents);
}
