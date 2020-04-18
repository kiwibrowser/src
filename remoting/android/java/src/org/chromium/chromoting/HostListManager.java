// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chromoting;

import android.os.Handler;
import android.os.HandlerThread;
import android.os.Looper;

import org.json.JSONArray;
import org.json.JSONException;
import org.json.JSONObject;

import org.chromium.base.Log;

import java.io.IOException;
import java.io.OutputStream;
import java.net.HttpURLConnection;
import java.net.MalformedURLException;
import java.net.URL;
import java.util.ArrayList;
import java.util.Collections;
import java.util.Comparator;
import java.util.Locale;
import java.util.Scanner;

/** Helper for fetching and modifying the host list. */
@SuppressWarnings("JavaLangClash")
public class HostListManager {
    public enum Error {
        AUTH_FAILED,
        NETWORK_ERROR,
        SERVICE_UNAVAILABLE,
        UNEXPECTED_RESPONSE,
        UNKNOWN,
    }

    /** Callback for receiving the host list, or getting notified of an error. */
    public interface Callback {
        void onHostListReceived(HostInfo[] response);
        void onHostUpdated();
        void onHostDeleted();
        void onError(Error error);
    }

    /**
     * Represents a response from the directory server.
     * If the request failed, |error| will not be null and |body| will be null.
     * If the request succeeds, |error| will be null and |body| will not be null.
     **/
    private static class Response {
        public final Error error;
        public final String body;
        public Response(Error error, String body) {
            this.error = error;
            this.body = body;
        }
    }

    private static final String TAG = "Chromoting";

    /** Path from which to download a user's host list JSON object. */
    private static final String HOST_LIST_PATH =
            "https://www.googleapis.com/chromoting/v1/@me/hosts";

    /** Callback handler to be used for network operations. */
    private Handler mNetworkThread;

    /** Handler for main thread. */
    private Handler mMainThread;

    public HostListManager() {
        // Thread responsible for downloading the host list.

        mMainThread = new Handler(Looper.getMainLooper());
    }

    private void runOnNetworkThread(Runnable runnable) {
        if (mNetworkThread == null) {
            HandlerThread thread = new HandlerThread("network");
            thread.start();
            mNetworkThread = new Handler(thread.getLooper());
        }
        mNetworkThread.post(runnable);
    }

    /**
      * Causes the host list to be fetched on a background thread. This should be called on the
      * main thread, and callbacks will also be invoked on the main thread. On success,
      * callback.onHostListReceived() will be called, otherwise callback.onError() will be called
      * with an error-code describing the failure.
      */
    public void retrieveHostList(final String authToken, final Callback callback) {
        runOnNetworkThread(new Runnable() {
            @Override
            public void run() {
                doRetrieveHostList(authToken, callback);
            }
        });
    }

    private void doRetrieveHostList(String authToken, Callback callback) {
        Response response = sendRequest(authToken, HOST_LIST_PATH, "GET", null, null);
        if (response.error != null) {
            postError(callback, response.error);
            return;
        }

        // Parse directory response.
        ArrayList<HostInfo> hostList = new ArrayList<HostInfo>();
        try {
            JSONObject data = new JSONObject(response.body).getJSONObject("data");
            if (data.has("items")) {
                JSONArray hostsJson = data.getJSONArray("items");

                int index = 0;
                while (!hostsJson.isNull(index)) {
                    JSONObject hostJson = hostsJson.getJSONObject(index);
                    // If a host is only recently registered, it may be missing some of the keys
                    // below. It should still be visible in the list, even though a connection
                    // attempt will fail because of the missing keys. The failed attempt will
                    // trigger reloading of the host-list, by which time the keys will hopefully be
                    // present, and the retried connection can succeed.
                    HostInfo host = HostInfo.create(hostJson);
                    hostList.add(host);
                    ++index;
                }
            }
        } catch (JSONException ex) {
            // Logging the exception stack trace may be too spammy.
            Log.e(TAG, "Error parsing host list response: %s", ex.getMessage());
            postError(callback, Error.UNEXPECTED_RESPONSE);
            return;
        }

        sortHosts(hostList);

        final Callback callbackFinal = callback;
        final HostInfo[] hosts = hostList.toArray(new HostInfo[hostList.size()]);
        mMainThread.post(new Runnable() {
            @Override
            public void run() {
                callbackFinal.onHostListReceived(hosts);
            }
        });
    }

    /**
     * Updates a host on the background thread. On success, callback.onHostUpdated() will be called,
     * otherwise callback.onError() will be called with an error-code describing the failure.
     */
    public void putHost(final String authToken, final String hostId, final String hostName,
                        final String publicKey, final Callback callback) {
        runOnNetworkThread(new Runnable() {
            @Override
            public void run() {
                doPutHost(authToken, hostId, hostName, publicKey, callback);
            }
        });
    }

    private void doPutHost(String authToken, String hostId, String hostName, String publicKey,
                           final Callback callback) {
        String requestJson;
        try {
            JSONObject data = new JSONObject();
            data.put("hostId", hostId);
            data.put("hostName", hostName);
            data.put("publicKey", publicKey);
            JSONObject request = new JSONObject();
            request.put("data", data);
            requestJson = request.toString();
        } catch (JSONException ex) {
            Log.e(TAG, "Error creating put host JSON string: %s", ex.getMessage());
            postError(callback, Error.UNKNOWN);
            return;
        }
        Response response = sendRequest(authToken, HOST_LIST_PATH + '/' + hostId, "PUT",
                "application/json", requestJson);
        if (response.error != null) {
            postError(callback, response.error);
        } else {
            mMainThread.post(new Runnable() {
                @Override
                public void run() {
                    callback.onHostUpdated();
                }
            });
        }
    }

    /**
     * Deletes a host on the background thread. On success, callback.onHostUpdated() will be called,
     * otherwise callback.onError() will be called with an error-code describing the failure.
     */
    public void deleteHost(final String authToken, final String hostId,
                           final Callback callback) {
        runOnNetworkThread(new Runnable() {
            @Override
            public void run() {
                doDeleteHost(authToken, hostId, callback);
            }
        });
    }

    private void doDeleteHost(String authToken, String hostId, final Callback callback) {
        Response response = sendRequest(authToken, HOST_LIST_PATH + '/' + hostId, "DELETE",
                null, null);
        if (response.error != null) {
            postError(callback, response.error);
        } else {
            mMainThread.post(new Runnable() {
                @Override
                public void run() {
                    callback.onHostDeleted();
                }
            });
        }
    }

    /** Posts error to callback on main thread. */
    private void postError(Callback callback, Error error) {
        final Callback callbackFinal = callback;
        final Error errorFinal = error;
        mMainThread.post(new Runnable() {
            @Override
            public void run() {
                callbackFinal.onError(errorFinal);
            }
        });
    }

    private static void sortHosts(ArrayList<HostInfo> hosts) {
        Comparator<HostInfo> hostComparator = new Comparator<HostInfo>() {
            @Override
            public int compare(HostInfo a, HostInfo b) {
                if (a.isOnline != b.isOnline) {
                    return a.isOnline ? -1 : 1;
                }
                String aName = a.name.toUpperCase(Locale.getDefault());
                String bName = b.name.toUpperCase(Locale.getDefault());
                return aName.compareTo(bName);
            }
        };
        Collections.sort(hosts, hostComparator);
    }

    /**
     * Sends request to the url and returns the response.
     * @param authToken auth token
     * @param url the URL to send the request
     * @param method /GET/POST/PUT/DELETE/etc.
     * @param requestContentType The content type of the request body. This can be null.
     * @param requestBody This can be null.
     * @return The response.
     */
    private static Response sendRequest(String authToken, String url, String method,
                                        String requestContentType, String requestBody) {
        HttpURLConnection link = null;
        Error error = null;
        try {
            link = (HttpURLConnection) new URL(url).openConnection();
            link.setRequestMethod(method);
            link.setRequestProperty("Authorization", "OAuth " + authToken);
            if (requestContentType != null) {
                link.setRequestProperty("Content-Type", requestContentType);
            }
            if (requestBody != null) {
                byte[] requestBytes = requestBody.getBytes("UTF-8");
                OutputStream outStream = link.getOutputStream();
                outStream.write(requestBytes);
                outStream.close();
            }

            // Listen for the server to respond.
            int status = link.getResponseCode();
            // TODO(yuweih): Turn this switch statement into range testing. e.g. 200-299 = OK.
            switch (status) {
                case HttpURLConnection.HTTP_OK:  // 200
                case HttpURLConnection.HTTP_NO_CONTENT:  // 204
                    break;
                case HttpURLConnection.HTTP_UNAUTHORIZED:  // 401
                    error = Error.AUTH_FAILED;
                    break;
                case HttpURLConnection.HTTP_BAD_GATEWAY:  // 502
                case HttpURLConnection.HTTP_UNAVAILABLE:  // 503
                    error = Error.SERVICE_UNAVAILABLE;
                    break;
                default:
                    error = Error.UNKNOWN;
            }

            if (error != null) {
                return new Response(error, null);
            }

            StringBuilder responseBuilder = new StringBuilder();
            Scanner incoming = new Scanner(link.getInputStream());
            while (incoming.hasNext()) {
                responseBuilder.append(incoming.nextLine());
            }
            incoming.close();
            return new Response(null, responseBuilder.toString());
        } catch (MalformedURLException ex) {
            // This should never happen.
            throw new RuntimeException("Unexpected error while fetching host list: ", ex);
        } catch (IOException ex) {
            return new Response(Error.NETWORK_ERROR, null);
        } finally {
            if (link != null) {
                link.disconnect();
            }
        }
    }
}
