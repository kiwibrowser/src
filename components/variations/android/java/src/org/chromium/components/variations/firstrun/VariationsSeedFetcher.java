// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.components.variations.firstrun;

import android.content.SharedPreferences;
import android.os.SystemClock;

import org.chromium.base.ContextUtils;
import org.chromium.base.Log;
import org.chromium.base.ThreadUtils;
import org.chromium.base.VisibleForTesting;
import org.chromium.base.metrics.CachedMetrics.SparseHistogramSample;
import org.chromium.base.metrics.CachedMetrics.TimesHistogramSample;

import java.io.ByteArrayOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.net.HttpURLConnection;
import java.net.MalformedURLException;
import java.net.SocketTimeoutException;
import java.net.URL;
import java.net.UnknownHostException;
import java.text.ParseException;
import java.text.SimpleDateFormat;
import java.util.Arrays;
import java.util.Date;
import java.util.Locale;
import java.util.concurrent.TimeUnit;

/**
 * Fetches the variations seed before the actual first run of Chrome.
 */
public class VariationsSeedFetcher {
    private static final String TAG = "VariationsSeedFetch";

    public enum VariationsPlatform { ANDROID, ANDROID_WEBVIEW }

    private static final String VARIATIONS_SERVER_URL =
            "https://clientservices.googleapis.com/chrome-variations/seed?osname=";

    private static final int BUFFER_SIZE = 4096;
    private static final int READ_TIMEOUT = 3000; // time in ms
    private static final int REQUEST_TIMEOUT = 1000; // time in ms

    // Values for the "Variations.FirstRun.SeedFetchResult" sparse histogram, which also logs
    // HTTP result codes. These are negative so that they don't conflict with the HTTP codes.
    // These values should not be renumbered or re-used since they are logged to UMA.
    private static final int SEED_FETCH_RESULT_UNKNOWN_HOST_EXCEPTION = -3;
    private static final int SEED_FETCH_RESULT_TIMEOUT = -2;
    private static final int SEED_FETCH_RESULT_IOEXCEPTION = -1;

    @VisibleForTesting
    static final String VARIATIONS_INITIALIZED_PREF = "variations_initialized";

    // Synchronization lock to make singleton thread-safe.
    private static final Object sLock = new Object();

    private static VariationsSeedFetcher sInstance;

    @VisibleForTesting
    public VariationsSeedFetcher() {}

    public static VariationsSeedFetcher get() {
        // TODO(aberent) Check not running on UI thread. Doing so however makes Robolectric testing
        // of dependent classes difficult.
        synchronized (sLock) {
            if (sInstance == null) {
                sInstance = new VariationsSeedFetcher();
            }
            return sInstance;
        }
    }

    /**
     * Override the VariationsSeedFetcher, typically with a mock, for testing classes that depend on
     * this one.
     * @param fetcher the mock.
     */
    @VisibleForTesting
    public static void setVariationsSeedFetcherForTesting(VariationsSeedFetcher fetcher) {
        sInstance = fetcher;
    }

    @VisibleForTesting
    protected HttpURLConnection getServerConnection(
            VariationsPlatform platform, String restrictMode, String milestone, String channel)
            throws MalformedURLException, IOException {
        String urlString = getConnectionString(platform, restrictMode, milestone, channel);
        URL url = new URL(urlString);
        return (HttpURLConnection) url.openConnection();
    }

    @VisibleForTesting
    protected String getConnectionString(
            VariationsPlatform platform, String restrictMode, String milestone, String channel) {
        String urlString = VARIATIONS_SERVER_URL;
        switch (platform) {
            case ANDROID:
                urlString += "android";
                break;
            case ANDROID_WEBVIEW:
                urlString += "android_webview";
                break;
            default:
                assert false;
        }
        if (restrictMode != null && !restrictMode.isEmpty()) {
            urlString += "&restrict=" + restrictMode;
        }
        if (milestone != null && !milestone.isEmpty()) {
            urlString += "&milestone=" + milestone;
        }
        if (channel != null && !channel.isEmpty()) {
            urlString += "&channel=" + channel;
        }

        return urlString;
    }

    /**
     * Object holding the seed data and related fields retrieved from HTTP headers.
     */
    public static class SeedInfo {
        // If you add fields, see VariationsTestUtils.
        public String signature;
        public String country;
        public String date;
        public boolean isGzipCompressed;
        public byte[] seedData;

        public Date parseDate() throws ParseException {
            // The date field comes from the HTTP "Date" header, which has this format. (See RFC
            // 2616, sections 3.3.1 and 14.18.) SimpleDateFormat is weirdly not thread-safe, so
            // instantiate a new one for each call.
            return new SimpleDateFormat("EEE, dd MMM yyyy HH:mm:ss z", Locale.US).parse(date);
        }

        @Override
        public String toString() {
            return "SeedInfo{signature=\"" + signature + "\" country=\"" + country
                    + "\" date=\"" + date + " isGzipCompressed=" + isGzipCompressed
                    + " seedData=" + Arrays.toString(seedData);
        }
    }


    /**
     * Fetch the first run variations seed.
     * @param restrictMode The restrict mode parameter to pass to the server via a URL param.
     * @param milestone The milestone parameter to pass to the server via a URL param.
     * @param channel The channel parameter to pass to the server via a URL param.
     */
    public void fetchSeed(String restrictMode, String milestone, String channel) {
        assert !ThreadUtils.runningOnUiThread();
        // Prevent multiple simultaneous fetches
        synchronized (sLock) {
            SharedPreferences prefs = ContextUtils.getAppSharedPreferences();
            // Early return if an attempt has already been made to fetch the seed, even if it
            // failed. Only attempt to get the initial Java seed once, since a failure probably
            // indicates a network problem that is unlikely to be resolved by a second attempt.
            // Note that VariationsSeedBridge.hasNativePref() is a pure Java function, reading an
            // Android preference that is set when the seed is fetched by the native code.
            if (prefs.getBoolean(VARIATIONS_INITIALIZED_PREF, false)
                    || VariationsSeedBridge.hasNativePref()) {
                return;
            }

            try {
                SeedInfo info = downloadContent(
                        VariationsPlatform.ANDROID, restrictMode, milestone, channel);
                VariationsSeedBridge.setVariationsFirstRunSeed(info.seedData, info.signature,
                        info.country, info.date, info.isGzipCompressed);
            } catch (IOException e) {
                Log.e(TAG, "IOException when fetching variations seed.", e);
                // Exceptions are handled and logged in the downloadContent method, so we don't
                // need any exception handling here. The only reason we need a catch-statement here
                // is because those exceptions are re-thrown from downloadContent to skip the
                // normal logic flow within that method.
            }
            // VARIATIONS_INITIALIZED_PREF should still be set to true when exceptions occur
            prefs.edit().putBoolean(VARIATIONS_INITIALIZED_PREF, true).apply();
        }
    }

    private void recordFetchResultOrCode(int resultOrCode) {
        SparseHistogramSample histogram =
                new SparseHistogramSample("Variations.FirstRun.SeedFetchResult");
        histogram.record(resultOrCode);
    }

    private void recordSeedFetchTime(long timeDeltaMillis) {
        Log.i(TAG, "Fetched first run seed in " + timeDeltaMillis + " ms");
        TimesHistogramSample histogram = new TimesHistogramSample(
                "Variations.FirstRun.SeedFetchTime", TimeUnit.MILLISECONDS);
        histogram.record(timeDeltaMillis);
    }

    private void recordSeedConnectTime(long timeDeltaMillis) {
        TimesHistogramSample histogram = new TimesHistogramSample(
                "Variations.FirstRun.SeedConnectTime", TimeUnit.MILLISECONDS);
        histogram.record(timeDeltaMillis);
    }

    /**
     * Download the variations seed data with platform and retrictMode.
     * @param platform the platform parameter to let server only return experiments which can be
     * run on that platform.
     * @param restrictMode the restrict mode parameter to pass to the server via a URL param.
     * @param milestone the milestone parameter to pass to the server via a URL param.
     * @param channel the channel parameter to pass to the server via a URL param.
     * @return the object holds the seed data and its related header fields.
     * @throws SocketTimeoutException when fetching seed connection times out.
     * @throws UnknownHostException when fetching seed connection has an unknown host.
     * @throws IOException when response code is not HTTP_OK or transmission fails on the open
     * connection.
     */
    public SeedInfo downloadContent(
            VariationsPlatform platform, String restrictMode, String milestone, String channel)
            throws SocketTimeoutException, UnknownHostException, IOException {
        HttpURLConnection connection = null;
        try {
            long startTimeMillis = SystemClock.elapsedRealtime();
            connection = getServerConnection(platform, restrictMode, milestone, channel);
            connection.setReadTimeout(READ_TIMEOUT);
            connection.setConnectTimeout(REQUEST_TIMEOUT);
            connection.setDoInput(true);
            connection.setRequestProperty("A-IM", "gzip");
            connection.connect();
            int responseCode = connection.getResponseCode();
            recordFetchResultOrCode(responseCode);
            if (responseCode != HttpURLConnection.HTTP_OK) {
                String errorMsg = "Non-OK response code = " + responseCode;
                Log.w(TAG, errorMsg);
                throw new IOException(errorMsg);
            }

            recordSeedConnectTime(SystemClock.elapsedRealtime() - startTimeMillis);

            SeedInfo info = new SeedInfo();
            info.seedData = getRawSeed(connection);
            info.signature = getHeaderFieldOrEmpty(connection, "X-Seed-Signature");
            info.country = getHeaderFieldOrEmpty(connection, "X-Country");
            info.date = getHeaderFieldOrEmpty(connection, "Date");
            info.isGzipCompressed = getHeaderFieldOrEmpty(connection, "IM").equals("gzip");
            recordSeedFetchTime(SystemClock.elapsedRealtime() - startTimeMillis);
            return info;
        } catch (SocketTimeoutException e) {
            recordFetchResultOrCode(SEED_FETCH_RESULT_TIMEOUT);
            Log.w(TAG, "SocketTimeoutException timeout when fetching variations seed.", e);
            throw e;
        } catch (UnknownHostException e) {
            recordFetchResultOrCode(SEED_FETCH_RESULT_UNKNOWN_HOST_EXCEPTION);
            Log.w(TAG, "UnknownHostException unknown host when fetching variations seed.", e);
            throw e;
        } catch (IOException e) {
            recordFetchResultOrCode(SEED_FETCH_RESULT_IOEXCEPTION);
            Log.w(TAG, "IOException when fetching variations seed.", e);
            throw e;
        } finally {
            if (connection != null) {
                connection.disconnect();
            }
        }
    }

    /**
     * Convert a input stream into a byte array.
     * @param inputStream the input stream
     * @return the byte array which holds the data from the input stream
     * @throws IOException if I/O error occurs when reading data from the input stream
     */
    public static byte[] convertInputStreamToByteArray(InputStream inputStream) throws IOException {
        ByteArrayOutputStream byteBuffer = new ByteArrayOutputStream();
        byte[] buffer = new byte[BUFFER_SIZE];
        int charactersReadCount = 0;
        while ((charactersReadCount = inputStream.read(buffer)) != -1) {
            byteBuffer.write(buffer, 0, charactersReadCount);
        }
        return byteBuffer.toByteArray();
    }

    private String getHeaderFieldOrEmpty(HttpURLConnection connection, String name) {
        String headerField = connection.getHeaderField(name);
        if (headerField == null) {
            return "";
        }
        return headerField.trim();
    }

    private byte[] getRawSeed(HttpURLConnection connection) throws IOException {
        InputStream inputStream = null;
        try {
            inputStream = connection.getInputStream();
            return convertInputStreamToByteArray(inputStream);
        } finally {
            if (inputStream != null) {
                inputStream.close();
            }
        }
    }
}
