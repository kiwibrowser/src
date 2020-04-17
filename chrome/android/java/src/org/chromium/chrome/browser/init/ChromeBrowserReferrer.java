// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.init;

import android.app.Activity;
import android.content.Context;
import android.os.AsyncTask;
import android.os.Build;
import android.os.Handler;
import android.os.Looper;
import android.os.Process;
import android.os.StrictMode;
import android.content.Intent;
import android.content.BroadcastReceiver;
import android.content.SharedPreferences;

import org.chromium.base.ActivityState;
import org.chromium.base.ApplicationStatus;
import org.chromium.base.ApplicationStatus.ActivityStateListener;
import org.chromium.base.CommandLine;
import org.chromium.base.ContentUriUtils;
import org.chromium.base.ContextUtils;
import org.chromium.base.Log;
import org.chromium.base.PathUtils;
import org.chromium.base.ResourceExtractor;
import org.chromium.base.SysUtils;
import org.chromium.base.ThreadUtils;
import org.chromium.base.TraceEvent;
import org.chromium.base.annotations.RemovableInRelease;
import org.chromium.base.library_loader.LibraryLoader;
import org.chromium.base.library_loader.LibraryProcessType;
import org.chromium.base.library_loader.ProcessInitException;
import org.chromium.base.memory.MemoryPressureUma;
import org.chromium.chrome.browser.AppHooks;
import org.chromium.chrome.browser.ChromeApplication;
import org.chromium.chrome.browser.ChromeStrictMode;
import org.chromium.chrome.browser.ChromeSwitches;
import org.chromium.chrome.browser.ClassRegister;
import org.chromium.chrome.browser.FileProviderHelper;
import org.chromium.chrome.browser.crash.LogcatExtractionRunnable;
import org.chromium.chrome.browser.download.DownloadManagerService;
import org.chromium.chrome.browser.services.GoogleServicesManager;
import org.chromium.chrome.browser.tabmodel.document.DocumentTabModelImpl;
import org.chromium.chrome.browser.webapps.ActivityAssigner;
import org.chromium.chrome.browser.webapps.ChromeWebApkHost;
import org.chromium.components.crash.browser.CrashDumpManager;
import org.chromium.content.browser.BrowserStartupController;
import org.chromium.content_public.browser.DeviceUtils;
import org.chromium.content_public.browser.SpeechRecognition;
import org.chromium.net.NetworkChangeNotifier;
import org.chromium.policy.CombinedPolicyProvider;

import java.net.URL;
import java.net.URLEncoder;
import java.net.HttpURLConnection;
import java.io.File;
import java.util.Locale;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.io.BufferedInputStream;
import java.io.ByteArrayOutputStream;
import java.io.IOException;
import java.net.MalformedURLException;
import java.io.UnsupportedEncodingException;

/**
 * Application level delegate that handles start up tasks.
 * {@link AsyncInitializationActivity} classes should override the {@link BrowserParts}
 * interface for any additional initialization tasks for the initialization to work as intended.
 */
public class ChromeBrowserReferrer extends BroadcastReceiver {
  private String readStream(InputStream is) {
      try {
        ByteArrayOutputStream bo = new ByteArrayOutputStream();
        int i = is.read();
        while (i != -1) {
          bo.write(i);
          i = is.read();
        }
        return bo.toString();
      } catch (IOException e) {
        return "";
      }
    }

    @Override
    public void onReceive(final Context context, Intent intent) {
      String referrer = intent.getStringExtra("referrer");

      if (referrer == null || referrer.length() == 0 || referrer.equals("")) {
        return;
      }

      Log.i("Kiwi", "Received ChromeBrowserReferrer: [" + referrer + "]");

      SharedPreferences.Editor sharedPreferencesEditor = ContextUtils.getAppSharedPreferences().edit();
      sharedPreferencesEditor.putString("install_referrer", (String)referrer);
      sharedPreferencesEditor.apply();

      Thread thread = new Thread(new Runnable() {
            @Override
            public void run() {
              try {
                URL url = new URL("https://update.kiwibrowser.com/a/install.php?ping=" + URLEncoder.encode(referrer, "UTF-8"));
                HttpURLConnection urlConnection = (HttpURLConnection) url.openConnection();
                InputStream in = new BufferedInputStream(urlConnection.getInputStream());
                readStream(in);
                urlConnection.disconnect();
              } catch (MalformedURLException e) {
                Log.e("Kiwi", "Received ChromeBrowserReferrer with malformed URL");
              } catch (UnsupportedEncodingException e) {
                Log.e("Kiwi", "Received ChromeBrowserReferrer with unsupported encoding");
              } catch (IOException e){
                Log.e("Kiwi", "Received ChromeBrowserReferrer but IOException");
              }
            }
      });

      thread.start();
  }
}
