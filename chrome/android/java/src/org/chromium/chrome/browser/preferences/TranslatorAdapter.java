// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.preferences;

import android.content.Context;
import android.content.Intent;
import android.content.res.Resources;
import android.os.Build;
import android.os.Bundle;
import android.support.annotation.IntDef;
import android.support.annotation.StringRes;
import android.text.SpannableString;
import android.text.TextUtils;
import android.text.style.ForegroundColorSpan;
import android.view.LayoutInflater;
import android.view.View;
import android.view.View.AccessibilityDelegate;
import android.view.View.OnClickListener;
import android.view.ViewGroup;
import android.view.accessibility.AccessibilityEvent;
import android.view.accessibility.AccessibilityNodeInfo;
import android.widget.BaseAdapter;
import android.widget.RadioButton;
import android.widget.TextView;

import org.chromium.base.ApiCompatibilityUtils;
import org.chromium.base.Log;
import org.chromium.base.VisibleForTesting;
import org.chromium.base.metrics.RecordUserAction;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.ContentSettingsType;
import org.chromium.chrome.browser.locale.LocaleManager;
import org.chromium.chrome.browser.preferences.website.ContentSetting;
import org.chromium.chrome.browser.preferences.website.GeolocationInfo;
import org.chromium.chrome.browser.preferences.website.NotificationInfo;
import org.chromium.chrome.browser.preferences.website.SingleWebsitePreferences;
import org.chromium.chrome.browser.preferences.website.WebsitePreferenceBridge;
import org.chromium.chrome.browser.search_engines.TemplateUrl;
import org.chromium.chrome.browser.search_engines.TemplateUrlService;
import org.chromium.components.location.LocationUtils;
import org.chromium.ui.text.SpanApplier;
import org.chromium.ui.text.SpanApplier.SpanInfo;
import org.chromium.base.ContextUtils;
import org.chromium.chrome.browser.RestartWorker;

import java.lang.annotation.Retention;
import java.lang.annotation.RetentionPolicy;
import java.util.ArrayList;
import java.util.Collections;
import java.util.Comparator;
import java.util.Iterator;
import java.util.List;
import java.util.concurrent.TimeUnit;

import android.view.View;
import android.content.DialogInterface;
import android.support.v7.app.AlertDialog;
import android.content.SharedPreferences;
import android.graphics.Color;
import android.graphics.drawable.ColorDrawable;
import android.widget.ListView;

/**
* A custom adapter for listing search engines.
*/
public class TranslatorAdapter extends BaseAdapter implements OnClickListener {
    private static final String TAG = "cr_Translators";

    private static final int VIEW_TYPE_ITEM = 0;
    private static final int VIEW_TYPE_DIVIDER = 1;
    private static final int VIEW_TYPE_COUNT = 2;

    private List<String> mPrepopulatedTranslators = new ArrayList<>();
    private List<String> mPrepopulatedDescriptions = new ArrayList<>();

    private int mSelectedTranslatorPosition = -1;

    /** The position of the default search engine before user's action. */
    private int mInitialTranslatorPosition = -1;

    /** The current context. */
    private Context mContext;

    /** The layout inflater to use for the custom views. */
    private LayoutInflater mLayoutInflater;

    /**
     * Construct a TranslatorAdapter.
     * @param context The current context.
     */
    public TranslatorAdapter(Context context) {
        mContext = context;
        mLayoutInflater = (LayoutInflater) mContext.getSystemService(
                Context.LAYOUT_INFLATER_SERVICE);
    }

    /**
     * Start the adapter to gather the available search engines and listen for updates.
     */
    public void start() {
      refreshData();
    }

    /**
     * Stop the adapter from listening for future search engine updates.
     */
    public void stop() {
    }

    /**
     * Initialize the search engine list.
     */
    private void refreshData() {
        mPrepopulatedTranslators = new ArrayList<>();
        mPrepopulatedDescriptions = new ArrayList<>();
        mPrepopulatedTranslators.add("Default");
        mPrepopulatedDescriptions.add("Microsoft Translator");
        mPrepopulatedTranslators.add("Google");
        mPrepopulatedDescriptions.add("Google Translate");
        mPrepopulatedTranslators.add("Yandex");
        mPrepopulatedDescriptions.add("Yandex Translator");
        mPrepopulatedTranslators.add("Baidu");
        mPrepopulatedDescriptions.add("Baidu Fanyi");
        mSelectedTranslatorPosition = 0;
        String activeTheme = ContextUtils.getAppSharedPreferences().getString("active_translator", "");
        for (int i = 0; i < mPrepopulatedTranslators.size(); ++i) {
          if (mPrepopulatedTranslators.get(i).equals(activeTheme)) {
             mSelectedTranslatorPosition = i;
          }
        }
        notifyDataSetChanged();
    }

    // BaseAdapter:

    @Override
    public int getCount() {
        return mPrepopulatedTranslators.size();
    }

    @Override
    public int getViewTypeCount() {
        return VIEW_TYPE_COUNT;
    }

    @Override
    public Object getItem(int pos) {
        return mPrepopulatedTranslators.get(pos);
    }

    @Override
    public long getItemId(int position) {
        return position;
    }

    @Override
    public int getItemViewType(int position) {
        return VIEW_TYPE_ITEM;
    }

    @Override
    public View getView(int position, View convertView, ViewGroup parent) {
        View view = convertView;
        int itemViewType = getItemViewType(position);
        if (convertView == null) {
            view = mLayoutInflater.inflate(R.layout.search_engine, null);
        }
        if (itemViewType == VIEW_TYPE_DIVIDER) {
            return view;
        }

        view.setOnClickListener(this);
        view.setTag(position);

        // TODO(finnur): There's a tinting bug in the AppCompat lib (see http://crbug.com/474695),
        // which causes the first radiobox to always appear selected, even if it is not. It is being
        // addressed, but in the meantime we should use the native RadioButton instead.
        RadioButton radioButton = (RadioButton) view.findViewById(R.id.radiobutton);
        // On Lollipop this removes the redundant animation ring on selection but on older versions
        // it would cause the radio button to disappear.
        // TODO(finnur): Remove the encompassing if statement once we go back to using the AppCompat
        // control.
        final boolean selected = position == mSelectedTranslatorPosition;
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.LOLLIPOP) {
            radioButton.setBackgroundResource(0);
        }
        radioButton.setChecked(selected);

        TextView description = (TextView) view.findViewById(R.id.name);
        Resources resources = mContext.getResources();

        if (description != null) {
             if (ContextUtils.getAppSharedPreferences().getBoolean("user_night_mode_enabled", false) || ContextUtils.getAppSharedPreferences().getString("active_theme", "").equals("Diamond Black")) {
                 description.setTextColor(Color.WHITE);
             }
        }

        String themeName = (String) getItem(position);
        description.setText(themeName);

        TextView url = (TextView) view.findViewById(R.id.url);
        url.setText(mPrepopulatedDescriptions.get(position));

        radioButton.setImportantForAccessibility(View.IMPORTANT_FOR_ACCESSIBILITY_NO);
        description.setAccessibilityDelegate(new AccessibilityDelegate() {
            @Override
            public void onInitializeAccessibilityEvent(View host, AccessibilityEvent event) {
                super.onInitializeAccessibilityEvent(host, event);
                event.setChecked(selected);
            }

            @Override
            public void onInitializeAccessibilityNodeInfo(View host, AccessibilityNodeInfo info) {
                super.onInitializeAccessibilityNodeInfo(host, info);
                info.setCheckable(true);
                info.setChecked(selected);
            }
        });

        return view;
    }

    // OnClickListener:

    @Override
    public void onClick(View view) {
        TranslatorSelected((int) view.getTag());
    }

    private String TranslatorSelected(int position) {
        // Record the change in search engine.
        mSelectedTranslatorPosition = position;
        SharedPreferences.Editor sharedPreferencesEditor = ContextUtils.getAppSharedPreferences().edit();
        sharedPreferencesEditor.putString("active_translator", mPrepopulatedTranslators.get(position));
        sharedPreferencesEditor.apply();
        notifyDataSetChanged();
        return mPrepopulatedTranslators.get(position);
    }

    private void AskForRelaunch() {
        AlertDialog.Builder alertDialogBuilder = new AlertDialog.Builder(mContext);
         alertDialogBuilder
            .setMessage(R.string.preferences_restart_is_needed)
            .setCancelable(true)
            .setPositiveButton(R.string.preferences_restart_now, new DialogInterface.OnClickListener() {
              @Override
              public void onClick(DialogInterface dialog,int id) {
                  RestartWorker restartWorker = new RestartWorker();
                  restartWorker.Restart();
                  dialog.cancel();
              }
            })
            .setNegativeButton(R.string.preferences_restart_later,new DialogInterface.OnClickListener() {
              @Override
              public void onClick(DialogInterface dialog,int id) {
                  dialog.cancel();
              }
            });
            AlertDialog alertDialog = alertDialogBuilder.create();
            alertDialog.show();
    }
}
