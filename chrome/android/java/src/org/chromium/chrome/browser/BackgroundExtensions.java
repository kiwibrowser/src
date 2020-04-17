package org.chromium.chrome.browser;

import android.annotation.TargetApi;
import android.app.Activity;
import android.content.Context;
import android.content.SharedPreferences;
import android.content.pm.ApplicationInfo;
import android.os.Build;
import android.os.Looper;
import android.util.Base64;
import android.util.JsonReader;
import android.util.JsonToken;
import android.webkit.JavascriptInterface;

import org.chromium.base.ContextUtils;
import org.chromium.base.ThreadUtils;
import org.chromium.content_public.browser.LoadUrlParams;
import org.chromium.ui.base.WindowAndroid;
import org.chromium.ui.base.ViewAndroidDelegate;
import org.chromium.ui.base.PageTransition;

import org.chromium.chrome.browser.WebContentsFactory;
import org.chromium.components.content_view.ContentView;
import org.chromium.content_public.browser.JavascriptInjector;
import org.chromium.content_public.browser.WebContents;

import org.json.JSONArray;
import org.json.JSONException;
import org.json.JSONObject;

import java.lang.IllegalArgumentException;
import java.lang.Runnable;
import java.util.ArrayList;
import java.util.Calendar;
import java.util.Collections;
import java.util.Iterator;
import java.util.List;
import java.util.Map;
import java.util.HashSet;
import java.util.HashMap;
import java.util.Random;
import java.util.Scanner;
import java.io.ByteArrayInputStream;
import java.io.ByteArrayOutputStream;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.io.IOException;
import java.io.ObjectInputStream;
import java.io.ObjectOutputStream;
import java.io.UnsupportedEncodingException;

import android.widget.Toast;
import android.util.Log;

public class BackgroundExtensions {
    public static final String TAG = "EXTENSIONS";
    private WebContents mWebContents;
    private Context mContext;

    public BackgroundExtensions(Context context) {
        mContext = context;

        Log.d(TAG, "Initializing Extension Engine");
//        InitExtensionEngine();
    }

    public void runJS(String code, String context) {
        Log.d(TAG, "RunExtension running: " + code + " in " + context);
        try {
            if (mWebContents != null) {
                String toLoad = "<script>" + code + "</script>";
                LoadUrlParams loadUrlParams = LoadUrlParams.createLoadDataParamsWithBaseUrl(toLoad, "text/html", false, context, null);
                loadUrlParams.setCanLoadLocalResources(true);
                mWebContents.getNavigationController().loadUrl(loadUrlParams);
            } else {
                Log.e(TAG, "RunExtension exception, WebContents is null");
            }
        } catch (Exception exc) {
            // Ignoring sync exception, we will try it on a next loop execution
            Log.e(TAG, "RunExtension exception: " + exc);
        }
    }

    private void RunExtension() {
        try {
            if (mWebContents == null) {
                mWebContents = WebContentsFactory.createWebContents(false, true);
                if (mWebContents != null) {
                    LoadUrlParams loadUrlParams = new LoadUrlParams("chrome-search://local-ntp/extensions.html", PageTransition.GENERATED);
                    loadUrlParams.setCanLoadLocalResources(true);
                    mWebContents.getNavigationController().loadUrl(loadUrlParams);
                }
            }
        } catch (Exception exc) {
            // Ignoring sync exception, we will try it on a next loop execution
            Log.e(TAG, "RunExtension exception: " + exc);
        }
    }

    public void InitExtensionEngine() {
        ThreadUtils.runOnUiThread(new Runnable() {
            @Override
            public void run() {
               RunExtension();
            }
        });
    }
}
