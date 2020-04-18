/*
 * Copyright (C) 2015 Google Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package com.android.i18n.addressinput;

import com.google.i18n.addressinput.common.AsyncRequestApi;
import com.google.i18n.addressinput.common.JsoMap;
import java.io.BufferedReader;
import java.io.InputStreamReader;
import java.net.HttpURLConnection;
import java.net.MalformedURLException;
import java.net.URL;
import java.security.Provider;
import java.security.Security;
import javax.net.ssl.HttpsURLConnection;
import javax.net.ssl.SSLContext;
import javax.net.ssl.SSLSocketFactory;

/**
 * Android implementation of AsyncRequestApi.
 * <p>
 * Note that this class uses a thread-per-request approach to asynchronous connections. There are
 * likely to be better ways of doing this (and even if not, this implementation suffers from several
 * issues regarding interruption and cancellation). Ultimately this class should be revisited and
 * most likely rewritten.
 */
// TODO: Reimplement this class according to current best-practice for asynchronous requests.
public class AndroidAsyncRequestApi implements AsyncRequestApi {
  private static final String TAG = "AsyncRequestApi";

  /** Simple implementation of asynchronous HTTP GET. */
  private static class AsyncHttp extends Thread {
    private final URL requestUrl;
    private final AsyncCallback callback;
    private final int timeoutMillis;

    protected AsyncHttp(URL requestUrl, AsyncCallback callback, int timeoutMillis) {
      this.requestUrl = requestUrl;
      this.callback = callback;
      this.timeoutMillis = timeoutMillis;
    }

    @Override
    public void run() {
      try {
        // While MalformedURLException from URL's constructor is a different kind of error than
        // issues with the HTTP request, we're handling them the same way because the URLs are often
        // generated based on data returned by previous HTTP requests and we need robust, graceful
        // handling of any issues.
        HttpURLConnection connection = (HttpURLConnection) requestUrl.openConnection();
        connection.setConnectTimeout(timeoutMillis);
        connection.setReadTimeout(timeoutMillis);

        Provider[] providers = Security.getProviders();
        if (providers.length > 0 && providers[0].getName().equals("GmsCore_OpenSSL")) {
          // GMS security provider requires special handling of HTTPS connections. (b/29555362)
          if (connection instanceof HttpsURLConnection) {
            SSLContext context = SSLContext.getInstance("TLS");
            context.init(null /* KeyManager */, null /* TrustManager */, null /* SecureRandom */);
            SSLSocketFactory sslSocketFactory = context.getSocketFactory();
            if (sslSocketFactory != null) {
              ((HttpsURLConnection) connection).setSSLSocketFactory(sslSocketFactory);
            }
          }
        }

        if (connection.getResponseCode() == HttpURLConnection.HTTP_OK) {
          BufferedReader responseReader = new BufferedReader(
              new InputStreamReader(connection.getInputStream(), "UTF-8"));
          StringBuilder responseJson = new StringBuilder();
          String line;
          while ((line = responseReader.readLine()) != null) {
            responseJson.append(line);
          }
          responseReader.close();
          callback.onSuccess(JsoMap.buildJsoMap(responseJson.toString()));
        } else {
          callback.onFailure();
        }
        connection.disconnect();
      } catch (Exception e) {
        callback.onFailure();
      }
    }
  }

  @Override public void requestObject(String url, AsyncCallback callback, int timeoutMillis) {
    try {
      (new AsyncHttp(stringToUrl(url), callback, timeoutMillis)).start();
    } catch (MalformedURLException e) {
      callback.onFailure();
    }
  }

  protected URL stringToUrl(String url) throws MalformedURLException {
    return new URL(url);
  }
}
